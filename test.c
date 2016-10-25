/*
 * =====================================================================================
 *
 *       Filename:  test.c
 *
 *    Description:  
 *
 *        Version:  1.0
 *        Created:  2016年10月09日 14时49分21秒
 *       Revision:  none
 *       Compiler:  gcc
 *
 *         Author:  YOUR NAME (), 
 *   Organization:  
 *
 * =====================================================================================
 */

#include<stdio.h>
#include<sys/types.h>
#include<sys/stat.h>
#include<fcntl.h>
#include<string.h>

#define FILE_NAME "/dev/shn_cdev"

int main(int argc, char *argv[])
{
	int fd = 0;
	char wrbuf[] = "Hello Kernel";
	//int wr_size = strlen(wrbuf) + 1;
	int wr_size = sizeof(wrbuf);

	if (argc != 2) {
		printf("usage: ./a.out read_buf_size\n");
		return -1;
	}
	int rd_size = atoi(argv[1]);
	char rdbuf[rd_size];
	int size = 0;

	fd = open(FILE_NAME, O_RDWR, 0666);
	if (fd < 0) {
		perror("open file failed");
		return -1;
	}

#if 1
	if (write(fd, wrbuf, wr_size) != wr_size) {
		perror("write file failed");
		return -1;
	}
#endif

#if 1
	printf("rd_size:%d\n", rd_size);
	if ((size = read(fd, rdbuf, rd_size)) != rd_size) {
		perror("read file failed");
		return -1;
	}
	printf("rdbuf:%s, size:%d\n", rdbuf, size);
#endif

	close(fd);

	return 0;
}
