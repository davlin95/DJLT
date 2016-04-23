all: server client chat

server: server.c
	gcc -Wall -Werror -g -o server server.c

client: client.c clientHeader.h 
	gcc -Wall -Werror -g -o client client.c

chat: chat.c
	gcc -Wall -Werror -g -o chat chat.c

clean:
	rm -f *~ *.o server client chat
