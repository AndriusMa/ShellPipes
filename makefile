all: shell

shell: shell.o
	gcc shell.o -l readline -o shell

shell.o: shell.c
	gcc -c shell.c -Wall

clean:
	rm *.o	
