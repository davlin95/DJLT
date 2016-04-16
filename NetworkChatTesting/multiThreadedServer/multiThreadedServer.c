/****************** SERVER CODE ****************/
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
#include <sys/epoll.h>



/* Create file defining WOLFIE PROTOCOL */
#ifndef PROTOCOL_WOLFIE
#define PROTOCOL_WOLFIE "WOLFIE"
#endif 

void* acceptThread(void* args){
  char messageOfTheDay[1024];
  int serverFd, connfd, status=0;

  /*Build addrinfo structs*/
  struct addrinfo settings, *results;
  memset(&settings,0,sizeof(settings));
  settings.ai_family=AF_INET;
  settings.ai_socktype=SOCK_STREAM;

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
   /* Make the socket address reuseable immediatley on closeure. */
  int val=1;
  status=setsockopt(serverFd, SOL_SOCKET,SO_REUSEADDR,&val,sizeof(val));
  if (status < 0)
  {
      fprintf(stderr,"setsockopt(): %s\n",strerror(errno)); 
      freeaddrinfo(results);
      close(serverFd);
      exit(-1);
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

  /* Listen in on the socket */
  if(listen(serverFd,1024)<0){
    if(close(serverFd)<0){
      fprintf(stderr,"close(serverFd): %s\n",strerror(errno));
    }
    fprintf(stderr,"listen(): %s\n",strerror(errno)); 
    exit(1);
  }
  else{
    printf("Listening\n");
  }


    /******************* IMPLEMENT EPOLLS ***************/

  /* Initialize Epolls Interface*/
  struct epoll_event epollEvent, * allEpollEvents;
  allEpollEvents = calloc(1024,sizeof(epollEvent));

  int epollFd,eStatus,numEpollFds;
  epollFd = epoll_create(1);

  /* Set epollEvent for serverFd */
  epollEvent.data.fd = serverFd;
  epollEvent.events = EPOLLIN;
  eStatus = epoll_ctl(epollFd,EPOLL_CTL_ADD,serverFd,&epollEvent);
  if(eStatus<0){
    fprintf(stderr,"epoll_ctl() serverFd :%s",strerror(errno));
  }

  /* Set epollEvent for server stdin */
  epollEvent.data.fd = 0;
  epollEvent.events = EPOLLIN;
  eStatus = epoll_ctl(epollFd,EPOLL_CTL_ADD,0,&epollEvent);
  if(eStatus<0){
    fprintf(stderr,"epoll_ctl() stdin :%s",strerror(errno));
  }

  /* Waiting for File Descriptors */
  uint32_t epollErrors = EPOLLERR;
  while(1){
     printf("epolls is waiting\n");
     numEpollFds= epoll_wait(epollFd,allEpollEvents,1024,-1);
     if(numEpollFds<0){
       fprintf(stderr,"epoll_wait():%s",strerror(errno));
     }

     int i;
     for(i=0;i<numEpollFds;i++){
       printf("numEpollFds = %d\n",numEpollFds);

       if(allEpollEvents[i].events &epollErrors){
         /* Check for erros */
         fprintf(stderr,"epollErrors after waiting:%s",strerror(errno));

       }else if(allEpollEvents[i].data.fd==serverFd){
         /*Server has incoming new client */

          struct sockaddr_storage newClientStorage;
          socklen_t addr_size = sizeof(newClientStorage);
          connfd = accept(serverFd, (struct sockaddr *) &newClientStorage, &addr_size);
          if(connfd<0){
             fprintf(stderr,"accept(): %s\n",strerror(errno));
             close(connfd); 
             continue;
          }             
          printf("Accepted new client! %d\n", connfd);
          /* Add the connfd into the listening set */
          epollEvent.data.fd = connfd;
          epollEvent.events = EPOLLIN;
          eStatus = epoll_ctl(epollFd,EPOLL_CTL_ADD,connfd,&epollEvent);
          if(eStatus<0){
             fprintf(stderr,"epoll_ct() on new client: %s\n",strerror(errno));
          }

          // printf("Accepted!:%s\n",serverStorage.sin_addr.s_addr);
          strcpy(messageOfTheDay,"Hello World\n");
          send(epollEvent.data.fd,messageOfTheDay,(strlen(messageOfTheDay)+1),0);
          
       }else if(allEpollEvents[i].data.fd==0){
         /*Server has incoming commands*/
         printf("STDIN has something to say\n");

       }else{
         /* Connected client has something to say */
         printf("One client has something to say %d\n",allEpollEvents[i].data.fd);
         int bytes,doneReading,writeStatus=-1;
         char clientMessage[1024];
         while(1){
            /*Read from client */
            memset(&clientMessage,0,strlen(clientMessage));
            bytes = read(allEpollEvents[i].data.fd,clientMessage,sizeof(clientMessage));
            if(bytes<0){
              doneReading=1;
              if(errno!=EAGAIN)
                fprintf(stderr,"Error reading from client %d\n",allEpollEvents[i].data.fd);
              break;
            }else if(bytes==0){
              doneReading=1;
              break;
            }
            //Output client message
            writeStatus = write(1,clientMessage,bytes);
            if(writeStatus<0){
              fprintf(stderr,"Error writing client message %d\n",allEpollEvents[i].data.fd);
            }
         }
         if(doneReading){
           printf("closing client descriptor %d\n",allEpollEvents[i].data.fd);
           close(allEpollEvents[i].data.fd);
         }


       }
     }
  }

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
  return NULL;
}


int main(){
  int threadStatus;
  pthread_t tid[1026];

  threadStatus = pthread_create(&(tid[0]), NULL, &acceptThread, NULL);

  /**************
  printf("main thread");
  char str[1024];
  int readStatus=0;
  int status = read(0,&str,1024);
  write(1,str,strlen(str));
  ***************/

  pthread_join(tid[0],NULL);
  return 0;

}

