#include <linux/module.h>
#include <linux/fs.h>
#include <linux/init.h>
#include <linux/cdev.h>
#include <linux/slab.h>
#include <linux/uaccess.h>
#include <linux/gfp.h>
#include <linux/types.h>
#include <asm/io.h>
#include <linux/errno.h>
#include <linux/mm.h>
#include <linux/sched.h>

#define GLOBALMEM_SIZE 0x1000
#define MEM_CLEAR 0x1
#define GLOBALMEM_MAJOR 230

static int globalmem_major = GLOBALMEM_MAJOR;
module_param(globalmem_major, int, S_IRUGO);

/*
 * 设备结构体,一般形式都是：
 * struct xxx_dev_t{
 *     struct cdev cdev;
 *     ....
 * }xxx_dev;
 *
 */
struct globalmem_dev {
	struct cdev cdev;//linux内核中使用cdev描述一个字符设备
	unsigned char mem[GLOBALMEM_SIZE];
};

struct globalmem_dev *globalmem_devp;

//从设备中读取数据
static ssize_t globalmem_read(struct file *filp, char __user *buf, size_t size,
		loff_t *ppos) {
	unsigned long p = *ppos;
	unsigned int count = size;
	int ret = 0;
	struct globalmem_dev *dev = filp->private_data;

	if (p >= GLOBALMEM_SIZE) {
		return count ? -ENXIO : 0;
	}
	if (count > GLOBALMEM_SIZE - p) {
		count = GLOBALMEM_SIZE - p;
	}
	//用户空间不能直接访问内核空间的内存，必须在read函数中借助copy_to_user()复制数据
	if (copy_to_user(buf, (void*) (dev->mem + p), count)) {
		ret = - EFAULT;
	} else {
		*ppos += count;
		ret = count;
		printk(KERN_INFO "read %u bytes(s) from %lu\n", count, p);
	}
	return ret;
}

//向设备发送数据
static ssize_t globalmem_write(struct file *filp, const char __user *buf,
		size_t size, loff_t *ppos) {
	unsigned long p = *ppos;
	unsigned int count = size;
	int ret = 0;
	struct globalmem_dev *dev = filp->private_data;

	if (p >= GLOBALMEM_SIZE)
		return count ? -ENXIO : 0;
	if (count >= GLOBALMEM_SIZE - p)
		count = GLOBALMEM_SIZE - p;

	if (copy_from_user(dev->mem + p, buf, count)) {
		ret = -EFAULT;
	} else {
		*ppos += count;
		ret = count;

		printk(KERN_INFO "written %u bytes(s) from %lu\n", count, p);
	}
	return ret;
}

/*
 * 修改文件当前读写位置，返回新位置，出错时返回负值
 * filp:内核规定的文件结构体指针
 * orig:对文件定位的起始地址，0 文件头，1当前位置，2文件尾
 * 假定globalmem支持0和1
 */
static loff_t globalmem_llseek(struct file *filp, loff_t offset, int orig) {
	printk(KERN_INFO "llseek %u bytes(s) from %u\n", offset, orig);
	loff_t ret = 0;
	switch (orig) {
	case 0:
		if (offset < 0) {
			ret = - EINVAL;
			break;
		}
		if ((unsigned int) offset > GLOBALMEM_SIZE) {
			ret = - EINVAL;
			break;
		}
		filp->f_pos = (unsigned int) offset;
		ret = filp->f_pos;
		break;
	case 1:
		if ((filp->f_pos + offset) > GLOBALMEM_SIZE) {
			ret = - EINVAL;
			break;
		}
		if ((filp->f_pos + offset) < 0) {
			ret = - EINVAL;
			break;
		}
		filp->f_pos += offset;
		ret = filp->f_pos;
		break;
	default:
		ret = - EINVAL;
		break;
	}
	return ret;
}

//提供设备的控制命令。对应用户空间程序调用的int fcntl(int fd,int cmd,.../*arg*/)和int ioctl(int d,int request,...)
static long globalmem_ioctl(struct file *filp, unsigned int cmd,
		unsigned long arg) {
	struct globalmem_dev *dev = filp->private_data;
	switch (cmd) {
	case MEM_CLEAR://接受到MEM_CLEAR命令，将数据清0
		memset(dev->mem, 0, GLOBALMEM_SIZE);
		printk(KERN_INFO "globalmem is set to zero\n");
		break;

	default:
		return - EINVAL;
	}
	return 0;
}

//用户空间调用Linux API的open()时，驱动的open函数最终被调用
static int globalmem_open(struct inode* inode, struct file *filp) {
	//大部分Linux驱动都遵循的规则，就是将文件的私有数据private_data指向设备结构体
	//然后在read()、write()、ioctl()、llseek()等函数中通过private_data访问设备结构体
	filp->private_data = globalmem_devp;
	printk(KERN_NOTICE"globalmem_open\n");
	return 0;
}

//与open向对应
static int globalmem_release(struct inode* inode, struct file *filp) {
	printk(KERN_NOTICE"globalmem_release\n");
	return 0;
}

//cdev->file_operations定义了字符设备驱动提供给虚拟文件系统的接口函数
static const struct file_operations globalmem_fops = {
		.owner = THIS_MODULE,
		.llseek = globalmem_llseek,
		.read = globalmem_read,
		.write = globalmem_write,
		.unlocked_ioctl = globalmem_ioctl,
		.open = globalmem_open,
		.release = globalmem_release
};

static void globalmem_setup_cdev(struct globalmem_dev *dev, int index) {
	//宏MKDEV通过主设备号和次设备号生成cdev->dev_t
	//反之，通过宏MAJOR，MINOR从dev_t获取主设备号和次设备号
	int err, devno = MKDEV(globalmem_major, index);

	//cdev_init函数用于初始化cdev，建立cdev和file_operations之间的连接
	cdev_init(&dev->cdev, &globalmem_fops);
	dev->cdev.owner = THIS_MODULE;
	//cdev_add向系统添加一个cdev，完成字符设备的注册
	//记得在卸载模块时调用cdev_del()注销此设备
	err = cdev_add(&dev->cdev, devno, 1);
	if (err)
		printk(KERN_NOTICE"Error %d adding globalmem%d\n", err, index);
}

static int __init globalmem_init(void) {
	int ret;
	dev_t devno = MKDEV(globalmem_major, 0);

	//申请设备号.记得注销设备时调用unregister_chrdev_region释放设备号
	if (globalmem_major) {
		//已知设备的设备号。向系统注册
		ret = register_chrdev_region(devno, 1, "globalmem");
	} else {
		//未知设备的设备号。由系统动态分配一个，并放入第一个参数中
		ret = alloc_chrdev_region(&devno, 0, 1, "globalmem");
		globalmem_major = MAJOR(devno);
	}
	if (ret < 0) {
		//一般是由于设备号冲突，有其他程序使用了同样的设备号，
		//或者自己退出时没有没有执行unregister_chrdev_region(devno,1);
		return ret;
	}
	globalmem_devp = kzalloc(sizeof(struct globalmem_dev), GFP_KERNEL);
	if (!globalmem_devp) {
		ret = -ENOMEM;
		goto fail_malloc;
	}
	globalmem_setup_cdev(globalmem_devp, 0);
	printk(KERN_NOTICE"globalmem_init success\n");
	return 0;

	fail_malloc:
	printk(KERN_NOTICE"globalmem_init fail\n");
	unregister_chrdev_region(devno, 1);
	return ret;
}
module_init(globalmem_init);

static void __exit globalmem_exit(void){
	cdev_del(&globalmem_devp->cdev);
	kfree(globalmem_devp);
	unregister_chrdev_region(MKDEV(globalmem_major, 0), 1);
	printk(KERN_NOTICE"globalmem_exit\n");
}
module_exit(globalmem_exit);

MODULE_AUTHOR("xue@163.com");
MODULE_LICENSE("GPL v2");
MODULE_DESCRIPTION("A simple character device Module!");
MODULE_ALIAS("A simple char!");

