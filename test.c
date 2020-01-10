#include <stdio.h>
#include <unistd.h>
#include <stdlib.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <sys/fcntl.h>
#include <sys/ioctl.h>

#define MEM_CLEAR 0x1

/**
 * 补\0
 */
void readStr(int fd, char *buf, size_t nbytes){
	//globalmem.c globalmem_read会执行
	read(fd, buf, nbytes);
	buf[nbytes] = '\0';
}
int main(void) {
	int devfd;
	int num = 0;
	char buf[64];

	//globalmem.c globalmem_open会执行
	devfd = open("/dev/globalmem", O_RDWR);
	if (devfd == -1) {
		printf("can't open /dev/globalmem\n");
		return -1;
	}
	printf("write abcde to globalmem...\n");
	//globalmem.c globalmem_write会执行
	write(devfd, "abcde", 5);
	//globalmem.c globalmem_release会执行
	close(devfd);
	devfd = open("/dev/globalmem", O_RDWR);
	readStr(devfd, buf, 3);
	printf("read 3 characters: %s\n", buf);//abc
	readStr(devfd, buf, 3);
	printf("read 3 characters: %s\n", buf);//de

	printf("clear data to zero\n");
	//globalmem.c globalmem_ioctl会执行
	ioctl(devfd,MEM_CLEAR);
	//globalmem.c globalmem_llseek会执行
	lseek(devfd,0,SEEK_SET);
	readStr(devfd, buf, 5);
	printf("read characters: %s\n", buf);

	close(devfd);
	return 0;
}
