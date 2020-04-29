#define _GNU_SOURCE

#include<stdio.h>
#include<string.h>
#include<stdlib.h>
#include<errno.h>
#include<sched.h>
#include<time.h>
#include<unistd.h>
#include<sys/wait.h>
#include<sys/syscall.h>
#include<sys/resource.h>

#define ERR_EXIT(a){perror(a); exit(-1);}
#define MAX_PROCESS 32
#define UNIT_TIME() { volatile unsigned long i; for(i=0;i<1000000UL;i++);} 
#define PRIORITY_HIGH 90
#define PRIORITY_LOW 50
#define CPU_PARENT 0
#define CPU_CHILD 1
#define FIFO 0
#define RR 1
#define SJF 2
#define PSJF 3
#define SYS_PRINTK 334
#define SYS_TIME 333
#define RR_CYCLE 500

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
		long start_time = syscall(SYS_TIME);
		//printf("Hi I am child %d\n", pid);
		setPriority(pid, PRIORITY_LOW);
		for (int i = 0; i < p.exec; i++){
			UNIT_TIME();
		}
		long end_time = syscall(SYS_TIME);
		//printf("Child %d is done %ld %ld\n", pid, start_time/1000000000, end_time/1000000000);
		syscall(SYS_PRINTK, pid, start_time, end_time);
		printf("%s %d\n", p.name, pid);
		exit(0);
	}
	useCpu(p.pid, CPU_CHILD);
	return p.pid;
}
int next(int N, int strategy, Process p[MAX_PROCESS], int run_process, int timer){
	if (strategy == FIFO){
		if (run_process != -1){
			return run_process;
		} for(int i = 0; i < N; i++){
			if (p[i].exec > 0 && p[i].pid > 0){
				//printf("%s run\n", p[i].name);
				return i;
			}
		}
	} else if (strategy == RR){
		if (run_process==-1 || (timer - p[run_process].start) % RR_CYCLE == 0){
			int flag = -1;
			for(int i = run_process+1; i < run_process+N+1; i++){
				if(p[i%N].pid != -1 && p[i%N].exec > 0){
					run_process = i%N;
					flag = i%N;
					break;
				}
			}
			if(flag == -1) run_process = -1;
		} return run_process;
	} else if (strategy == SJF){
		if (run_process != -1){
			return run_process;
		}
		else if (run_process ==-1){
			int ptr = 10000000;
			for(int i = 0; i < N; i++){
				if(p[i].pid > -1 && p[i].exec > 0 ){
					if(p[i].exec < ptr){
						ptr = p[i].exec;
						run_process = i;
					}
				}
			}
		} return run_process;
	} else if (strategy == PSJF){
	   	if(run_process ==-1){
			int ptr = 100000000;
			for(int i = 0; i < N; i++){
				if(p[i].pid > -1 && p[i].exec > 0 ){
					if(p[i].exec < ptr){
						ptr = p[i].exec;
						run_process = i;
					}
				}
			} return run_process;
		} else{
			for(int i = 0; i < N; i++){
				if(p[i].pid > -1 && p[i].exec > 0 && p[i].exec < p[run_process].exec){
					run_process = i;
				}
			} return run_process;
		}
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
	int timer = 0, all_process = N, run_process = -1;
	while(all_process > 0){
		if(run_process != -1 && p[run_process].exec == 0){
			waitpid(p[run_process].pid, NULL, 0);
			fprintf(stderr,"%s end at %d\n", p[run_process].name, timer);
			run_process = -1;
			all_process--;
		}
		for(int i = 0; i < N; i++){
			if(p[i].ready == timer){
				p[i].pid = createProcess(p[i]);
				fprintf(stderr,"Create new process %s %d at %d\n", p[i].name, p[i].pid, timer);
			}
		}
		int todo = next(N, strategy, p, run_process, timer);
		if (run_process != todo){
			p[todo].start = timer;
			if (run_process != -1){
				setPriority(p[run_process].pid, PRIORITY_LOW);
				fprintf(stderr,"%s be preempt at %d by %s\n", p[run_process].name, timer, p[todo].name);
				p[run_process].start = -1;
			}
			setPriority(p[todo].pid, PRIORITY_HIGH);
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
	struct rlimit old, new;
	new.rlim_cur = RLIM_INFINITY;
	new.rlim_max = RLIM_INFINITY;
	if(setrlimit(RLIMIT_CPU, &new) < 0){
		ERR_EXIT("set resorce error");
	}
	if(setrlimit(RLIMIT_RTTIME, &new) < 0){
		ERR_EXIT("set resorce error");
	}
	//set cpu usege to infinity
	//a = getrlimit(RLIMIT_CPU, &old);
	useCpu(getpid(), CPU_PARENT);
	setPriority(getpid(), PRIORITY_HIGH);
	fprintf(stderr,"Scheduler pid %d\n", getpid());

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
	syscall(SYS_PRINTK, 0, 0, 0);
	//puts("Done!!!");
}