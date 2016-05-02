all: server client chat

server: server.c
	gcc -g -Wall -Werror server.c -o server -lcrypto  -pthread -l sqlite3

client: client.c clientHeader.h 
	gcc -Wall -Werror -g -o client client.c

chat: chat.c
	gcc -Wall -Werror -g -o chat chat.c

clean:
	rm -f *~ *.o server client chat
