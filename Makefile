
all: scheduler

scheduler:
	gcc -O2 for_implement.c -o for_implement
	gcc -O2 queue_implement.c -o queue_implement

clean:
	@rm for_implement queue_implement

run:
	sudo ./for_implement
