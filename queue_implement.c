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
#define PRIORITY_INIT 50
#define PRIORITY_HIGH 90
#define PRIORITY_LOW 1
#define CPU_PARENT 0
#define CPU_CHILD 1
#define RR_CYCLE 50
#define FIFO 0
#define RR 1
#define SJF 2
#define PSJF 3
#define SYS_PRINTK 334
#define SYS_TIME 333

typedef struct process{
	char name[MAX_PROCESS];
	int ready, exec, start, id;
	struct process *next;
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
/*
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
*/

void insert(int strategy, Process **head, Process **tail, Process *p, int run_process){
	if(!(*head)){
		*head = p;
		*tail = p;
		return;
	}
	switch(strategy){
		case FIFO: case RR:{
			(*tail)->next = p;
			*tail = p;
			break;
						   }
		case SJF:{
			if(run_process==-1){
				if(p->exec < (*head)->exec){
					p->next = *head;
					*head = p;
					if(!p->next){
						*tail = p;
					}
					return;
				}
			}
			Process *h;
			h = *head;
			while(h->next && p->exec >= h->next->exec){
				h = h->next;
			}
			p->next = h->next;
			h->next = p;
			if(!p->next){
				*tail = p;
			}
			break;
				 }
		case PSJF:{
			if(p->exec < (*head)->exec){
				p->next = *head;
				*head = p;
			} else{
				Process *h = *head;
				while(h->next && p->exec >= h->next->exec){
					h = h->next;
				}
				p->next = h->next;
				h->next = p;
			}
			if(!p->next){
				*tail = p;
			}
			break;
				  }
		default:{
			ERR_EXIT("No such strategy !");
			break;
				}
	}
}
int get(Process **head, Process **tail, int timer, int strategy){
	switch (strategy){
		case FIFO: case SJF: case PSJF:{
			if(*head) return (*head)->id;
			return -1;
			break;
		} 
		case RR:{
			if(!(*head)) return -1;
			//printf("%d %d\n", timer, (*head)->start);
			if ((*head)!=(*tail) && (timer-((*head)->start))%RR_CYCLE == 0){
				if((*head)->next){
					(*tail)->next = (*head);
					(*tail) = (*head);
					(*head) = (*head)->next;
					(*tail)->next = NULL;
					//Process *t = (*head);
					//t->next = NULL;
				}
			}
			return (*head)->id;
		}
	}
}


void task(int strategy){
	int N;
	Process *head = NULL, *tail = NULL;
	if(scanf("%d", &N) < 0){
		ERR_EXIT("scanf error");
	}
	Process p[MAX_PROCESS];
	for(int i = 0; i< N; i++){
		if(scanf("%s%d%d", p[i].name, &p[i].ready, &p[i].exec)<0){
			ERR_EXIT("scanf error");
		}
		p[i].pid = p[i].start = -1;
		p[i].next = NULL;
		p[i].id = i;
	}
	qsort(p, N, sizeof(Process),cmp);
	int timer = 0, all_process = N, run_process = -1;
	while(all_process > 0){
		if(run_process != -1 && p[run_process].exec == 0){
			printf("%s end at %d\n", p[run_process].name, timer);
			//waitpid(p[run_process].pid, NULL, 0);
			run_process = -1;
			all_process--;
			head = head->next;
		}
		for(int i = 0; i < N; i++){
			if(p[i].ready == timer){
				printf("Create new process %s at %d\n", p[i].name, timer);
				//p[i].pid = createProcess(p[i]);
				insert(strategy, &head, &tail, &p[i], run_process);
				//printf("%s %d create\n", p[i].name, p[i].pid);
			}
		}
		int todo = get(&head, &tail, timer, strategy);
		//next(N, strategy, p, run_process, timer);
		//printf("todo %d\n", todo);
		if (run_process != todo){
			//setPriority(p[todo].pid, PRIORITY_HIGH);
			p[todo].start = timer;
			//printf("%s %d run\n", p[todo].name, p[todo].pid);
			if (run_process != -1){
				//setPriority(p[run_process].pid, PRIORITY_LOW);
				printf("%s be preempt at %d by %s\n", p[run_process].name, timer, p[todo].name);
				p[run_process].start = -1;
			}
			run_process = todo;
		}
		//UNIT_TIME();
		//printf("time: %d run %s\n", timer, p[run_process].name);
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
	/*
	struct rlimit old, new;
	new.rlim_cur = RLIM_INFINITY;
	new.rlim_max = RLIM_INFINITY;
	if(setrlimit(RLIMIT_CPU, &new) < 0){
		ERR_EXIT("set resorce error");
	}
	if(setrlimit(RLIMIT_RTTIME, &new) < 0){
		ERR_EXIT("set resorce error");
	}
	*/
	//set cpu usege to infinity
	//a = getrlimit(RLIMIT_CPU, &old);
	//useCpu(getpid(), CPU_PARENT);
	//setPriority(getpid(), PRIORITY_HIGH);
	//long long c = 10;
	//while(c--)printf("%d\n", getpid());
	
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
	
	//puts("Done!!!");
}
