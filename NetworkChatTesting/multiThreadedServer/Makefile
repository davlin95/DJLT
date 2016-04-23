all: multiServer

multiServer: multiThreadedServer.c
	gcc -g -Wall -Werror multiThreadedServer.c -o multiServer -lcrypto  -pthread -l sqlite3

