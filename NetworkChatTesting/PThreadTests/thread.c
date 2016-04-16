#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <string.h>
#include <errno.h>
#include <netdb.h>
#include <sys/types.h>
#include <pthread.h>
 
// A normal C function that is executed as a thread when its name
// is specified in pthread_create()
void *loginThread(void *vargp){
	printf("login thread \n");
}
void *acceptThread(void *vargp)
{
	//int i = 1;
	//while(1)
    printf("accept thread spawns login thread\n");
    //return NULL;
}
  
int main()
{
    pthread_t tid;
    printf("Before Thread\n");
    pthread_create(&tid, NULL, acceptThread, NULL);
   	//pthread_join(tid, NULL);
    //pthread_exit(NULL);
    printf("After Thread\n");
    while(1);
    return 0;
}