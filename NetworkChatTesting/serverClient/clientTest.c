
#include <stdio.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <unistd.h>
#include <netdb.h>
#include <string.h>
#include <stdlib.h> 
#include <errno.h>

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

  /* NEW WAY: Build addrinfo structs */
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

 /*Build clientFd, to match the server's domain,socket type, and domain*/
  clientFd = socket(results->ai_family, results->ai_socktype, results->ai_protocol);
  if(clientFd<0){
    fprintf(stderr,"socket():%s\n",strerror(errno)); //@todo set and print errno
    exit(1);  
  }
  
  /*Reach out to server retrieved in results struct*/

  /******************* Test print
  struct sockaddr_in* sock = (struct sockaddr_in *)(results->ai_addr);
  printf("sinport: %d\n",(sock->sin_port));
  printf("Port: %d\n",ntohl(sock->sin_addr.s_addr)); 
  ***************/

  status = connect(clientFd, results->ai_addr, results->ai_addrlen);
  if(status<0){
    fprintf(stderr,"Connect():%s\n",strerror(errno)); //@todo set and print errno
    exit(1);
  }


  /*---- Read the message from the server into the buffer ----*/
  recv(clientFd, message, 1024, 0);
  printf("Data received: %s",message);   

  freeaddrinfo(results);

  return 0;
}