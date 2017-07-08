#define __NR_sys_andrewgatessyscall 7

#include <sys/types.h>
#include <sys/syscall.h>
#include <linux/kernel.h>
#include <signal.h>
#include <unistd.h>
#include <stdio.h>

int main(int argc, char *argv[])
{
	printf("Testing\n");
	syscall(__NR_sys_andrewgatessyscall);
	return 0;
}