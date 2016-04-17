#include <stdio.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <unistd.h>
#include <netdb.h>
#include <string.h>
#include <stdlib.h> 
#include <errno.h>
#include <sys/fcntl.h>
#include <sys/epoll.h>


int main(){
  int clientFd,status;
  char message[1024];
  socklen_t addr_size;

  /*************************OLD WAY ***************************/
  /* Build the Address Of server being reached
  struct sockaddr_in serverAddr;
  serverAddr.sin_family = AF_INET;
  serverAddr.sin_port = htons(1234);
  serverAddr.sin_addr.s_addr = inet_addr("127.0.0.1");
  memset(serverAddr.sin_zero, '\0', sizeof serverAddr.sin_zero);  
  addr_size = sizeof serverAddr;*/
  /**************************************************************/

  /***************** NEW WAY: Build addrinfo structs ******/
 struct addrinfo settings, * results;
 memset(&settings,0,sizeof(settings));
 settings.ai_family=AF_INET;
 settings.ai_socktype=SOCK_STREAM;
 //settings.ai_flags=AI_PASSIVE;
 settings.ai_protocol=0;
 status = getaddrinfo("127.0.0.1","1234",&settings,&results);
 if(status!=0){
    fprintf(stderr,"getaddrinfo():%s\n",strerror(errno)); //@todo set and print errno
    exit(1);
 }

 /*************************************************************************/
 /* Build clientFd, to match the server's domain,socket type, and domain */
 /***********************************************************************/
  clientFd = socket(results->ai_family, results->ai_socktype, results->ai_protocol);
  if(clientFd<0){
    fprintf(stderr,"socket():%s\n",strerror(errno)); //@todo set and print errno
    exit(1);  
  }

  /******************* Test print
  struct sockaddr_in* sock = (struct sockaddr_in *)(results->ai_addr);
  printf("sinport: %d\n",(sock->sin_port));
  printf("Port: %d\n",ntohl(sock->sin_addr.s_addr)); 
  ***************/

  /*Reach out to server retrieved in results struct*/
  status = connect(clientFd, results->ai_addr, results->ai_addrlen);
  if(status<0){
    fprintf(stderr,"Connect():%s\n",strerror(errno)); 
    exit(1);
  }

  /*********** NOTIFY SERVER OF CONNECTION *****/
  fcntl(clientFd,F_SETFL,O_NONBLOCK); 
  strcpy(message,"Hi, I am client and I've connected\n");
  send(clientFd,message,(strlen(message)),0);

  /*****************************************/
  /*        Initialize Epolls Interface   */
  /***************************************/
  struct epoll_event epollEvent, * allEpollEvents;
  allEpollEvents = calloc(1024,sizeof(epollEvent));

  int epollFd,eStatus,numEpollFds;
  epollFd = epoll_create(1024);

  /* Set epollEvent for serverFd */
  epollEvent.data.fd = clientFd;
  epollEvent.events = EPOLLIN|EPOLLET;
  eStatus = epoll_ctl(epollFd,EPOLL_CTL_ADD,clientFd,&epollEvent);
  if(eStatus<0){
    fprintf(stderr,"epoll_ctl() clientFd :%s",strerror(errno));
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

     int i;
     for(i=0;i<numEpollFds;i++){
       printf("numEpollFds = %d\n",numEpollFds);

       /****************** IF ERROR WAS DETECTED **********************/
       if(allEpollEvents[i].events & epollErrors){
         /* Check for erros */
         fprintf(stderr,"epollErrors after waiting:%s\n",strerror(errno));

       /*************** WHEN SERVER TALKS TO THIS CLIENT ************/
       }else if(allEpollEvents[i].data.fd==clientFd){
         printf("Server is talking to this client\n");
         int serverBytes =0;
         while( (serverBytes = recv(clientFd, message, 1024, 0))>0){
            printf("Data received: %s\n",message);
            memset(&message,0,1024);   
         }
       /************** WHEN CLIENT STDIN IS ACTIVE ********************/
       }else if(allEpollEvents[i].data.fd==0){
         printf("STDIN has something to say\n");

         int bytes=0;
         char stdinBuffer[1024];
         memset(&stdinBuffer,0,1024);
         while( (bytes=read(0,&stdinBuffer,1024))>0){
            printf("reading from client STDIN...\n");

            /****** @TODO Make logout connected to BYE\r\n\r\n **********/
            if(strcmp(stdinBuffer,"/logout\n")==0){
              close(clientFd);
              freeaddrinfo(results);
              exit(EXIT_SUCCESS);
              break;
            }
            send(clientFd,stdinBuffer,(strlen(stdinBuffer)),0);
            printf("sent string :%s from client to server\n",stdinBuffer);
            memset(&stdinBuffer,0,strlen(stdinBuffer));
         }
       /**************** UNPREDICTABLE BEHAVIOR *****************/
       }else{
         /* Connected client has something to say */
         printf("This shouldn't be happening descriptor: %d\n",allEpollEvents[i].data.fd);

       }
     }
  }
  
  /******** CLEANUP *******/
  freeaddrinfo(results);

  return 0;
}