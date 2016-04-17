all: threadTest threading

threadTest: threadTest.c
	gcc -pthread threadTest.c -o threadTest

threading: thread.c
	gcc -Wall -Werror -o thread thread.c -lpthread
