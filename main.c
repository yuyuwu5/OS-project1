#define _GNU_SOURCE

#include<stdio.h>
#include<string.h>
#include<stdlib.h>
#include<errno.h>
#include<sched.h>
#include<unistd.h>
#include<sys/wait.h>
#include<sys/syscall.h>

#define ERR_EXIT(a){perror(a); exit(-1);}
#define MAX_PROCESS 32
#define UNIT_TIME() { volatile unsigned long i; for(i=0;i<1000000UL;i++);} 
#define PRIORITY_INIT 50
#define PRIORITY_HIGH 90
#define PRIORITY_LOW 1
#define CPU_PARENT 0
#define CPU_CHILD 1
#define FIFO 0
#define RR 1
#define SJF 2
#define PSJF 3
#define SYS_PRINTK 334
#define SYS_TIME 333

typedef struct process{
	char name[MAX_PROCESS];
	int ready, exec, start;
	pid_t pid;
}Process;


void useCpu(int pid, int core){
	cpu_set_t mask;
	CPU_ZERO(&mask);
	CPU_SET(core, &mask);
	if (sched_setaffinity(pid, sizeof(mask), &mask)< 0){
		ERR_EXIT("sched_setaffinity fail");
	}
}

void setPriority(int pid, int priority){
	struct sched_param param;
	param.sched_priority = priority;
	if (sched_setscheduler(pid, SCHED_FIFO, &param) < 0){
		ERR_EXIT("sched_setscheduler fail");
	}
}
int cmp(const void *p1, const void *p2){
	Process *s1 = (Process*)p1, *s2 = (Process*)p2;
	if(s1->ready > s2->ready){
		return 1;
	} else if (s1->ready < s2->ready){
		return -1;
	} return 0;
}
int createProcess(Process p){
	if((p.pid = fork()) < 0){
		ERR_EXIT("fork fail");
	} else if (p.pid == 0){
		int pid = getpid();
		printf("Hi I am child %d\n", pid);
		setPriority(pid, PRIORITY_LOW);
		long start_time = syscall(SYS_TIME);
		for (int i = 0; i < p.exec; i++){
			UNIT_TIME();
		}
		long end_time = syscall(SYS_TIME);
		printf("Child %d is done %ld %ld\n", pid, start_time, end_time);
		syscall(SYS_PRINTK, pid, start_time, end_time);
		syscall(878787);
		printf("%s %d\n", p.name, pid);
		exit(0);
	}
	useCpu(p.pid, CPU_CHILD);
	return p.pid;
}
int next(int N, int strategy, Process p[MAX_PROCESS], int run_process, int timer){
	if (strategy == FIFO){
		if (run_process > -1){
			return run_process;
		} for(int i = 0; i < N; i++){
			if (p[i].exec > 0 && p[i].pid > 0){
				//printf("%s run\n", p[i].name);
				return i;
			}
		}
	} else if (strategy == RR){
		if ((timer - p[run_process].start) % 500 == 0){
			do{
				run_process = (run_process+1)%N;
			} while(p[run_process].pid == -1 || p[run_process].exec == 0);
		} return run_process;
	} else if (strategy == SJF){
		if (run_process > -1){
			return run_process;
		} for(int i = 0; i < N; i++){
			if(p[i].pid > -1 && p[i].exec > 0 && p[i].exec < p[run_process].exec){
				run_process = i;
			}
		} return run_process;
	} else if (strategy == PSJF){
		for(int i = 0; i < N; i++){
			if(p[i].pid > -1 && p[i].exec > 0 && p[i].exec < p[run_process].exec){
				run_process = i;
			}
		} return run_process;
	} return -1;
}

void task(int strategy){
	int N;
	if(scanf("%d", &N) < 0){
		ERR_EXIT("scanf error");
	}
	Process p[MAX_PROCESS];
	for(int i = 0; i< N; i++){
		if(scanf("%s%d%d", p[i].name, &p[i].ready, &p[i].exec)<0){
			ERR_EXIT("scanf error");
		}
		p[i].pid = p[i].start = -1;
	}
	qsort(p, N, sizeof(Process),cmp);
	//for(int i = 0; i < N; i++)printf("%d\n", p[i].ready);
	int timer = 0, all_process = N, run_process = -1;
	while(all_process > 0){
		if(run_process != -1 && p[run_process].exec == 0){
			//waitpid(p[run_process].pid, NULL, 0);
			run_process = -1;
			all_process--;
		}
		for(int i = 0; i < N; i++){
			if(p[i].ready == timer){
				printf("Create new process at %d\n", timer);
				p[i].pid = createProcess(p[i]);
				printf("%d\n", p[i].pid);
			}
		}
		int todo = next(N, strategy, p, run_process, timer);
		printf("todo %d\n", todo);
		if (run_process != todo){
			setPriority(p[todo].pid, PRIORITY_HIGH);
			setPriority(p[run_process].pid, PRIORITY_LOW);
			p[run_process].start = -1;
			p[todo].start = timer;
			run_process = todo;
		}
		UNIT_TIME();
		if (run_process != -1){
			p[run_process].exec--;
		}
		timer++;
	}
}

int main(){
	char schdule_type[8];
	if(scanf("%s", schdule_type) < 0){
		ERR_EXIT("scanf error");
	}
	useCpu(getpid(), CPU_PARENT);
	setPriority(getpid(), PRIORITY_HIGH);
	int strategy;
	if (strcmp(schdule_type, "FIFO")==0){
		strategy = FIFO;
	} else if (strcmp(schdule_type, "RR") == 0){
		strategy = RR;
	} else if (strcmp(schdule_type, "SJF") == 0){
		strategy = SJF;
	} else if (strcmp(schdule_type, "PSJF") == 0){
		strategy = PSJF;
	} else{
		ERR_EXIT("Policy Not found!");
	}
	task(strategy);
	puts("Done!!!");
}
