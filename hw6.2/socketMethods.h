#include <sys/socket.h>
#include <sys/types.h>
#include <netdb.h>

int createSocket(int portNumber){
	socket(AF_UNIX,SOCK_STREAM,0);
}