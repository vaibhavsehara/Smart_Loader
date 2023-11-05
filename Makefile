all: sum fib smartloader

sum: sum.c
	gcc -g -m32 -no-pie -nostdlib -o sum sum.c

fib: fib.c
	gcc -m32 -no-pie -nostdlib -o fib fib.c

smartloader: smart_loader.c
	gcc -g -m32 -o smartloader smart_loader.c

clean:
	rm -f sum fib smartloader
