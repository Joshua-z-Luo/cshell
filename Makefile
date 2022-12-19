all: cshell

cshell:
	gcc -Wall -o cshell cshell.c

clean:
	rm -f *.o cshell
