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
#define PRIORITY_OCCUPY 95
#define PRIORITY_LOW 0
#define CPU_PARENT 0
#define CPU_CHILD 1
#define RR_CYCLE 500
#define FIFO 0
#define RR 1
#define SJF 2
#define PSJF 3
#define SYS_TIME 333
#define SYS_PRINTK 334

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

void setPriority(int pid, int type,int priority){
	struct sched_param param;
	param.sched_priority = priority;
	if (sched_setscheduler(pid, type, &param) < 0){
		ERR_EXIT("sched_setscheduler fail");
	}
	/*
	if(type == SCHED_IDLE){
		setpriority(PRIO_PROCESS, pid, 39);
	}
	else{
		setpriority(PRIO_PROCESS, pid, 0);

	}
	*/
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
		for (int i = 0; i < p.exec; i++){
			UNIT_TIME();
		}
		long end_time = syscall(SYS_TIME);
		//printf("Child %d is done %ld %ld\n", pid, start_time, end_time);
		syscall(SYS_PRINTK, pid, start_time, end_time);
		printf("%s %d\n", p.name, pid);
		exit(0);
	}
	setPriority(p.pid, SCHED_IDLE, PRIORITY_LOW);
	useCpu(p.pid, CPU_CHILD);
	return p.pid;
}

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
			if ((*head)!=(*tail) && (timer-((*head)->start))%RR_CYCLE == 0){
				if((*head)->next){
					(*tail)->next = (*head);
					(*tail) = (*head);
					(*head) = (*head)->next;
					(*tail)->next = NULL;
				}
			}
			return (*head)->id;
		}
	}
}


void task(int strategy){
	int N;
	FILE *perfect = fopen("Vperfect.txt", "w+");
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
	int timer = 0, all_process = N, run_process = -1, has_job=-1;
	while(all_process > 0){
		if(run_process != -1 && p[run_process].exec == 0){
			fprintf(perfect, "%d end at %d\n", p[run_process].pid, timer);
			fflush(perfect);
			//fprintf(stderr, "%d end at %d\n", p[run_process].pid, timer);
			waitpid(p[run_process].pid, NULL, 0);
			run_process = -1;
			all_process--;
			head = head->next;
		}
		for(int i = 0; i < N; i++){
			if(p[i].ready == timer){
				p[i].pid = createProcess(p[i]);
				//setPriority(p[i].pid, SCHED_IDLE, PRIORITY_LOW);
				insert(strategy, &head, &tail, &p[i], run_process);
				if(has_job==-1){
					has_job = timer;
				}
				fprintf(perfect,"Create new process %d at %d\n", p[i].pid, timer);
				//fprintf(stderr, "Create new process %d at %d\n", p[i].pid, timer);
				fflush(perfect);
			}
		}
		int todo = get(&head, &tail, timer, strategy);
		if (run_process != todo){
			if(todo != -1){
				setPriority(p[todo].pid, SCHED_OTHER, PRIORITY_LOW);
			}
			p[todo].start = timer;
			//printf("%s %d run\n", p[todo].name, p[todo].pid);
			if (run_process != -1){
				setPriority(p[run_process].pid, SCHED_IDLE,PRIORITY_LOW);
				//ity(p[run_process].pid, SCHED_FIFO,PRIORITY_LOW);
				//fprintf(perfect,"%s be preempt at %d by %s\n", p[run_process].name, timer, p[todo].name);
				p[run_process].start = -1;
			}
			run_process = todo;
		}
		UNIT_TIME();
		//printf("time: %d run %s\n", timer, p[run_process].name);
		if (run_process != -1){
			p[run_process].exec--;
		}
		timer++;
	}
	fprintf(perfect, "Real start: %d\n", has_job);
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
	setPriority(getpid(), SCHED_FIFO, PRIORITY_HIGH);
	//fprintf(stderr,"Scheduler pid %d\n", getpid());

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
	syscall(SYS_PRINTK,0,0,0);
	//puts("Done!!!");
}
