CC=gcc -fsanitize=address,undefined -Wall 

all:  My_Shell.o  utilities.o 
	$(CC)  My_Shell.o utilities.o -o  My_Shell 

My_Shell.o: My_Shell.c
	$(CC) -O0 -g -c My_Shell.c

utilities.o: utilities.c utilities.h
	$(CC) -O0 -g -c utilities.c

My_Shell:
	./My_Shell

clean:
	rm -rf *.o *.gcda *.gcno My_Shell utilities
