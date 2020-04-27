
all: main

main:
	gcc -O2 main.c -o main

clean:
	@rm main

run:
	sudo ./main
