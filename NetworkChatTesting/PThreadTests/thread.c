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
	struct addrinfo settings, *results;
  memset(&settings,0,sizeof(settings));
  settings.ai_family=AF_INET;
  settings.ai_socktype=SOCK_STREAM;
  //settings.ai_flags=AI_PASSIVE;

  /*Create linked list of socketaddresses */
  status = getaddrinfo(NULL,"1234",&settings,&results);
  if(status!=0){
    fprintf(stderr,"getaddrinfo():%s\n",gai_strerror(status));
    exit(1);
  }


  /*Build a socket */
  serverFd = socket(results->ai_family, results->ai_socktype, results->ai_protocol);
  if(serverFd==-1){
    fprintf(stderr,"socket(): %s\n", strerror(errno)); 
    exit(1);
  }
  /******************** OLD WAY OF GETTING ADDRESS *************/
  /*struct sockaddr serverAddr;
  struct sockaddr_storage serverStorage;
  socklen_t addr_size;*/

  /* Build an address 
  serverAddr.sin_family = AF_UNIX;
  serverAddr.sin_port = htons(1234);
  serverAddr.sin_addr.s_addr = inet_addr("127.0.0.1");
  memset(serverAddr.sin_zero, '\0', sizeof(serverAddr.sin_zero));  */
  /********************************************************************/

  /* Bind socket to address */
  status = bind(serverFd, results->ai_addr, results->ai_addrlen);
  if(status==-1){

    fprintf(stderr,"bind(): %s\n",strerror(errno));
    exit(1);
  }
  if(listen(serverFd,1024)<0){
    if(close(serverFd)<0){
      fprintf(stderr,"close(serverFd): %s\n",strerror(errno));
    }
    fprintf(stderr,"listen(): %s\n",strerror(errno)); // @todo: print errno
    exit(1);
  }
  else{
    printf("Listening\n");
  }

  struct sockaddr_storage serverStorage;
  socklen_t addr_size = sizeof(serverStorage);
  connfd = accept(serverFd, (struct sockaddr *) &serverStorage, &addr_size);
  if(connfd<0){
    fprintf(stderr,"accept(): %s\n",strerror(errno)); // @todo: print errno
  }else{
      printf("Accepted!\n");
   // printf("Accepted!:%s\n",serverStorage.sin_addr.s_addr);
  }


  strcpy(messageOfTheDay,"Hello World\n");
  send(connfd,messageOfTheDay,(strlen(messageOfTheDay)+1),0);

  //Cleanup
  freeaddrinfo(results);
  if(serverFd>0){
    close(serverFd);
    printf("closed serverFd\n");
  }
  if(connfd>0){
    close(connfd);
    printf("closed connfd\n");
  }
}
  
int main()
{
    pthread_t tid;
    printf("Before Thread\n");
    pthread_create(&tid, NULL, acceptThread, NULL);
   	pthread_join(tid, NULL);
    printf("After Thread\n");
    return 0;
}