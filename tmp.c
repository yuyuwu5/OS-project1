#include <stdio.h>
#define _GNU_SOURCE
#include <unistd.h>
#include <sys/syscall.h>

int main(){
	printf("%ld\n", syscall(332, 1, 2, 3));
}
