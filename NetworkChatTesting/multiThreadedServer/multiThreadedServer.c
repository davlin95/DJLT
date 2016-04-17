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
#include <sys/fcntl.h>


/* Create file defining WOLFIE PROTOCOL */
#ifndef PROTOCOL_WOLFIE
#define PROTOCOL_WOLFIE "WOLFIE"
#endif 


/*******************************/
/*      ACCEPT THREAD         */
/*****************************/
void* acceptThread(void* args){
  char messageOfTheDay[1024];
  int serverFd, connfd, status=0;

  /****************************/
  /* Build addrinfo structs   */
  /***************************/
  struct addrinfo settings, *results;
  memset(&settings,0,sizeof(settings));
  settings.ai_family=AF_INET;
  settings.ai_socktype=SOCK_STREAM;

  /*****************************************/
  /* Create linked list of socketaddresses */
  /*****************************************/
  status = getaddrinfo(NULL,"1234",&settings,&results);
  if(status!=0){
    fprintf(stderr,"getaddrinfo():%s\n",gai_strerror(status));
    exit(1);
  }
  
  /***********************************/
  /*  BUILD THE SOCKET              */
  /*********************************/
  serverFd = socket(results->ai_family, results->ai_socktype, results->ai_protocol);
  if(serverFd==-1){
    fprintf(stderr,"socket(): %s\n", strerror(errno)); 
    exit(1);
  }


  /***********************************/
  /*  Make socket address reuseable */
  /*********************************/
  int val=1;
  status=setsockopt(serverFd, SOL_SOCKET,SO_REUSEADDR,&val,sizeof(val));
  if (status < 0)
  {
      fprintf(stderr,"setsockopt(): %s\n",strerror(errno)); 
      freeaddrinfo(results);
      close(serverFd);
      exit(-1);
  }
  fcntl(serverFd,F_SETFL,O_NONBLOCK); 

  /******************** COMMENT OUT: OLD WAY OF GETTING ADDRESS *************/
  /*struct sockaddr serverAddr;
  struct sockaddr_storage serverStorage;
  socklen_t addr_size;*/

  /* Build an address 
  serverAddr.sin_family = AF_UNIX;
  serverAddr.sin_port = htons(1234);
  serverAddr.sin_addr.s_addr = inet_addr("127.0.0.1");
  memset(serverAddr.sin_zero, '\0', sizeof(serverAddr.sin_zero));  */
  /********************************************************************/

  /***************************/
  /* Bind socket to address */
  /*************************/
  status = bind(serverFd, results->ai_addr, results->ai_addrlen);
  if(status==-1){
    fprintf(stderr,"bind(): %s\n",strerror(errno));
    exit(1);
  }

  /***************************/
  /* Listen in on the socket */
  /*************************/
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


    /******************************************/
    /*        IMPLEMENT EPOLLS               */
    /****************************************/

  /* Initialize Epolls Interface*/
  struct epoll_event epollEvent, * allEpollEvents;
  allEpollEvents = calloc(1024,sizeof(epollEvent));
  int epollFd,eStatus,numEpollFds;
  epollFd = epoll_create(1024);

  /* Set epollEvent for serverFd */
  epollEvent.data.fd = serverFd;
  epollEvent.events = EPOLLIN|EPOLLET;
  eStatus = epoll_ctl(epollFd,EPOLL_CTL_ADD,serverFd,&epollEvent);
  if(eStatus<0){
    fprintf(stderr,"epoll_ctl() serverFd :%s",strerror(errno));
  }

  /* Set epollEvent for server stdin */
  fcntl(0,F_SETFL,O_NONBLOCK); 
  epollEvent.data.fd = 0;
  epollEvent.events = EPOLLIN|EPOLLET;
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

     /********************* AN EVENT WAS DETECTED:  **********/
     int i;
     for(i=0;i<numEpollFds;i++){
       printf("numEpollFds = %d\n",numEpollFds);

       /************** ERROR EVENT ***************/
       if(allEpollEvents[i].events &epollErrors){
         fprintf(stderr,"epollErrors after waiting:%s\n",strerror(errno));

       }
       /***************  Server has incoming new client ************/
       else if(allEpollEvents[i].data.fd==serverFd){
          printf("Server is taking in a client\n");
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
          fcntl(connfd,F_SETFL,O_NONBLOCK); 
          epollEvent.data.fd = connfd;
          epollEvent.events = EPOLLIN|EPOLLET;
          eStatus = epoll_ctl(epollFd,EPOLL_CTL_ADD,connfd,&epollEvent);
          if(eStatus<0){
             fprintf(stderr,"epoll_ct() on new client: %s\n",strerror(errno));
          }

          // printf("Accepted!:%s\n",serverStorage.sin_addr.s_addr);
          printf("GREETING MESSAGE 1 to CLIENT: %d\n",epollEvent.data.fd);
          strcpy(messageOfTheDay,"Hello World ...");
          send(epollEvent.data.fd,messageOfTheDay,(strlen(messageOfTheDay)+1),0);
          
       }
       /******************* SERVER'S STDIN IS ACTIVE ***************/
       else if(allEpollEvents[i].data.fd==0){
         printf("STDIN has something to say\n");
         int bytes=0;
         char stdinBuffer[1024];
         memset(&stdinBuffer,0,1024);
         while( (bytes=read(0,&stdinBuffer,1024))>0){
            printf("reading from server's STDIN...\n");
            
            /************* SEND TO CLIENT 
            send(clientFd,stdinBuffer,(strlen(stdinBuffer)),0);
            printf("sent string :%s from client to server\n",stdinBuffer); */

            printf("outputting from server's STDIN %s",stdinBuffer);
            memset(&stdinBuffer,0,strlen(stdinBuffer));
         }

       }
       /*********************** CONNECTED CLIENT SENT MESSAGE *********/
       else{
         printf("One client has something to say %d\n",allEpollEvents[i].data.fd);
         int bytes,doneReading,writeStatus=-1;
         char clientMessage[1024];

         /***********************/
         /* READ FROM CLIENT   */
         /*********************/
         while(1){ 
            memset(&clientMessage,0,strlen(clientMessage));
            bytes = read(allEpollEvents[i].data.fd,clientMessage,sizeof(clientMessage));
            if(bytes<0){
              if(errno!=EAGAIN){
                doneReading=1;
                fprintf(stderr,"Error reading from client %d\n",allEpollEvents[i].data.fd);
              }
              break;
            }else if(bytes==0){
              break;
            }
            
            /*********************************/
            /* OUTPUT MESSAGE FROM CLIENT   */
            /*******************************/
            writeStatus = write(1,clientMessage,bytes);
            if(writeStatus<0){
              fprintf(stderr,"Error writing client message %d\n",allEpollEvents[i].data.fd);
            }
            /**************************************/
            /* SEND RESPONSE MESSAGE TO CLIENT   */
            /************************************/
            printf("RESPONSE MESSAGE 1 to CLIENT: %d\n",epollEvent.data.fd);
            strcpy(messageOfTheDay,"Dear Client ... from server\n");
            send(epollEvent.data.fd,messageOfTheDay,(strlen(messageOfTheDay)+1),0);
         }
         /*******************************/
         /*   EXIT READING FROM CLIENT */
         /******************************/
         if(doneReading){
           printf("closing client descriptor %d\n",allEpollEvents[i].data.fd);
           close(allEpollEvents[i].data.fd);
         }


       }
     }
  }

  /************************/
  /* FINAL EXIT CLEANUP  */
  /**********************/
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

