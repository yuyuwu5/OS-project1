#define _GNU_SOURCE

#include<stdio.h>
#include<string.h>
#include<stdlib.h>
#include<errno.h>
#include<sched.h>
#include<unistd.h>
#include"process.h"

#define ERR_EXIT(a){perror(a); exit(1);}
void useCpu(int pid, int core){
	cpu_set_t mask;
	CPU_ZERO(&mask);
	CPU_SET(core, &mask);
	if (sched_setaffinity(pid, sizeof(mask), &mask)< 0){
		ERR_EXIT("sched_setaffinity fail");
	}
}

int main(){
	char schdule_type[8];
	int N, ready[32], execute[32];
	char name[32][32];
	scanf("%s%d", schdule_type, &N);
	for(int i = 0; i < N; i++){
		scanf("%s%d%d", name[i], &ready[i], &execute[i]);
	}
	useCpu(getpid(), 0);

}
