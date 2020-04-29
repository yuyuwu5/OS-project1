
all: scheduler

scheduler:
	gcc -O2 queue_implement_scheduler.c -o queue_implement_scheduler

clean:
	@rm queue_implement_scheduler

