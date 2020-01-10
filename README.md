# golbalmen

make生成globalmem.ko，然后用insmod globalmem.ko加载模块
通过lsmod发现模块已经加载：
root@xue-virtual-machine:~/W/c_workspace/golbalmen# lsmod | grep global
globalmem              16384  0

通过cat /proc/devices ，发现多出主设备号为230的字符设备驱动:
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



接下来通过mknod /dev/globalmem c 230 0创建设备节点，在/dev/目录下看到多了globalmem设备:
root@xue-virtual-machine:/dev# ls -l | grep globalmem
crw-r--r-- 1 root root    230,   0 12月 19 10:15 globalmem
root@xue-virtual-machine:/dev# 


执行
echo 111 > globalmem
cat globalmem
来验证数据的读取和写入
