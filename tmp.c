#include <stdio.h>
#define _GNU_SOURCE
#include <unistd.h>
#include <sys/syscall.h>

int main(){
	//printf("%ld\n", syscall(333));
	//printf("%ld\n", syscall(334, 0, 2, 3));
	syscall(334,0,0,0);

}
