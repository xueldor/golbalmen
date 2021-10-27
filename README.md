# golbalmen

实现一个简单的字符设备驱动

globalmem.c： 驱动实现

test.c： 测试对驱动的打开、读、写、控制。

## 操作步骤

在linux环境下，make命令生成globalmem.ko:

```shell
root@xue-virtual-machine:~/W/golbalmen# make
make -C /lib/modules/4.15.0-123-generic/build M=/home/xue/W/golbalmen modules
make[1]: Entering directory '/usr/src/linux-headers-4.15.0-123-generic'
  CC [M]  /home/xue/W/golbalmen/globalmem.o
In file included from ./include/linux/printk.h:7:0,
                 from ./include/linux/kernel.h:14,
                 from ./include/linux/list.h:9,
                 from ./include/linux/module.h:9,
                 from /home/xue/W/golbalmen/globalmem.c:1:
/home/xue/W/golbalmen/globalmem.c: In function ‘globalmem_llseek’:
./include/linux/kern_levels.h:5:18: warning: format ‘%u’ expects argument of type ‘unsigned int’, but argument 2 has type ‘loff_t {aka long long int}’ [-Wformat=]
 #define KERN_SOH "\001"  /* ASCII Start Of Header */
                  ^
./include/linux/kern_levels.h:14:19: note: in expansion of macro ‘KERN_SOH’
 #define KERN_INFO KERN_SOH "6" /* informational */
                   ^
/home/xue/W/golbalmen/globalmem.c:92:9: note: in expansion of macro ‘KERN_INFO’
/home/xue/W/golbalmen/globalmem.c:93:2: warning: ISO C90 forbids mixed declarations and code [-Wdeclaration-after-statement]
  Building modules, stage 2.
  MODPOST 1 modules
  CC      /home/xue/W/golbalmen/globalmem.mod.o
  LD [M]  /home/xue/W/golbalmen/globalmem.ko
make[1]: Leaving directory '/usr/src/linux-headers-4.15.0-123-generic'
root@xue-virtual-machine:~/W/golbalmen#
```

会生成许多中间文件，我们只需要globalmem.ko这个文件。

> `.ko`文件是kernel object文件（内核模块），意义就是把内核中那些不是经常使用的功能移动到内核外边， 需要的时候插入内核，不需要时卸载。就像U盘一样，要用时就插到电脑上，不用就拔下来。

然后用`insmod globalmem.ko`命令加载模块:

```shell
root@xue-virtual-machine:~/W/golbalmen# insmod globalmem.ko 
root@xue-virtual-machine:~/W/golbalmen# 
```

然后用lsmod命令查看模块是不是已经加载：

```shell
root@xue-virtual-machine:~/W/c_workspace/golbalmen# lsmod | grep global
globalmem              16384  0
```

把grep去掉，看看我的系统有哪些已载入系统的模块：

```shell
root@xue-virtual-machine:~/W/golbalmen# lsmod
Module                  Size  Used by
globalmem              16384  0
vmw_vsock_vmci_transport    32768  2
vsock                  36864  3 vmw_vsock_vmci_transport
binfmt_misc            20480  1
snd_ens1371            28672  0
snd_ac97_codec        131072  1 snd_ens1371
gameport               16384  1 snd_ens1371
ac97_bus               16384  1 snd_ac97_codec
snd_pcm                98304  2 snd_ac97_codec,snd_ens1371
```

通过`cat /proc/devices` ，发现多出主设备号为230的字符设备驱动:

```shell
root@xue-virtual-machine:~/W/c_workspace/golbalmen# cat /proc/devices
Character devices:
  1 mem
  4 /dev/vc/0
  4 tty
  4 ttyS
.........(省略)
230 globalmem
.........(省略)
Block devices:
  7 loop
  8 sd
.........(省略)
```



接下来, 通过`mknod /dev/globalmem c 230 0`创建设备节点:

```shell
root@xue-virtual-machine:~/W/golbalmen# mknod /dev/globalmem c 230 0
root@xue-virtual-machine:~/W/golbalmen# 
```

执行过后，在/dev/目录下看到多了globalmem设备:

```shell
root@xue-virtual-machine:~/W/golbalmen# ls -l /dev | grep globalmem
crw-r--r-- 1 root root    230,   0 10月 27 18:12 globalmem
```

可以看出，globalmem是一个字符设备。

执行
echo 111 > globalmem
cat globalmem
来验证数据的读取和写入：

```shell
root@xue-virtual-machine:~/W/golbalmen# echo 1112 > /dev/globalmem 
root@xue-virtual-machine:~/W/golbalmen# cat /dev/globalmem
1112
cat: /dev/globalmem: 没有那个设备或地址
root@xue-virtual-machine:~/W/golbalmen# 
```

最后，用`rmmod globalmem`命令卸载设备:

```shell
root@xue-virtual-machine:~/W/golbalmen# lsmod | grep globalmem
globalmem              16384  0
root@xue-virtual-machine:~/W/golbalmen# rmmod globalmem
root@xue-virtual-machine:~/W/golbalmen# lsmod | grep globalmem
root@xue-virtual-machine:~/W/golbalmen# 
```

如要删除/dev/目录下的字符设备，用`rm`即可：

```shell
root@xue-virtual-machine:~/W/golbalmen# rm /dev/globalmem 
root@xue-virtual-machine:~/W/golbalmen# ls -l /dev | grep globalmem
root@xue-virtual-machine:~/W/golbalmen# 
```

