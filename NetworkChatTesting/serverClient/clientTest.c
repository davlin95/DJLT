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
#include "../../hw5/clientHeader.h"


int main(){
  int clientFd; 
  char *portNumber = "1234";
  char message[1024];
  if ((clientFd = createAndConnect(portNumber, clientFd)) < 0)
    printf("error createandconnect\n");

  /*********** NOTIFY SERVER OF CONNECTION *****/
  fcntl(clientFd,F_SETFL,O_NONBLOCK); 
  strcpy(message,"Hi, I am client and I've connected\n");
  send(clientFd,message,(strlen(message)),0);

   /******************************************/
   /*        IMPLEMENT POLL                 */
   /****************************************/
    /* Initialize Polls Interface*/
    struct pollfd pollFds[1024];
    memset(pollFds, 0 , sizeof(pollFds));
    int pollStatus,pollNum=2;

    /* Set poll for clientFd */
    pollFds[0].fd = clientFd;
    pollFds[0].events = POLLIN;

    /* Set poll for stdin */
    pollFds[1].fd = 0;
    pollFds[1].events = POLLIN;
    fcntl(serverFd,F_SETFL,O_NONBLOCK); 

    while(1){
      pollStatus = poll(pollFds, pollNum, -1);
      if(pollStatus<0){
        fprintf(stderr,"poll():%s",strerror(errno));
        break;
      }
      int i;
      for(i=0;i<pollNum;i++){
        if(pollFds[i].revents==0) continue;
        if(pollFds[i].revents!=POLLIN){
          fprintf(stderr,"poll.revents:%s",strerror(errno));
          break;
        }
        /***********************************/
        /*   POLLIN FROM SERVERFD         */
        /*********************************/
        if(pollFds[i].fd == serverFd){
          printf("\n/***********************************/\n");
          printf("/*   Server is taking in clients   */\n");
          printf("/***********************************/\n");
          while(1){

            /************* STORING ALL INCOMING CONNECTS ****/
            struct sockaddr_storage newClientStorage;
            socklen_t addr_size = sizeof(newClientStorage);

            connfd = accept(serverFd,(struct sockaddr *) &newClientStorage, &addr_size);
            if(connfd<0){
              if(errno!=EWOULDBLOCK){
                fprintf(stderr,"accept(): %s\n",strerror(errno));
                close(connfd); 
              }
              break;
            }
            printf("Accepted new client! %d\n", connfd);
            fcntl(connfd,F_SETFL,O_NONBLOCK); 
            pollFds[pollNum].fd = connfd;
            pollFds[pollNum].events = POLLIN;
            pollNum++;
            /***** SEND MESSAGE TO CLIENT ****/
            printf("connfd: %d   pollFds[pollNum]: %d\n",connfd,pollFds[pollNum-1].fd);
            printf("GREETING MESSAGE 1 to CLIENT: %d\n",pollFds[pollNum-1].fd);
            strcpy(messageOfTheDay,"Hello World ...");
            send(pollFds[pollNum].fd,messageOfTheDay,(strlen(messageOfTheDay)+1),0);
          } 
        }

        /***********************************/
        /*   POLLIN FROM STDIN            */
        /*********************************/
        else if(pollFds[i].fd == 0){

          printf("\n/***********************************/\n");
          printf("/*   STDIN INPUT :                  */\n");
          printf("/***********************************/\n");
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

        /**************************************/
        /*   POLLIN: PREVIOUS CLIENT         */
        /************************************/
        else{

          printf("\n/***********************************/\n");
          printf("/*   CLIENT NUMBER %d SAYS:        */\n",pollFds[i].fd);
          printf("/***********************************/\n");
          int bytes,doneReading,writeStatus=-1;
          char clientMessage[1024];

          /***********************/
          /* READ FROM CLIENT   */
          /*********************/
          while(1){ 
            memset(&clientMessage,0,strlen(clientMessage));
            bytes = read(pollFds[i].fd,clientMessage,sizeof(clientMessage));
            if(bytes<0){
              if(errno!=EAGAIN){
                doneReading=1;
                fprintf(stderr,"Error reading from client %d\n",pollFds[i].fd);
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
              fprintf(stderr,"Error writing client message %d\n",pollFds[i].fd);
            }
            /**************************************/
            /* SEND RESPONSE MESSAGE TO CLIENT   */
            /************************************/
            printf("RESPONSE MESSAGE 1 to CLIENT: %d\n",pollFds[i].fd);
            strcpy(messageOfTheDay,"Dear Client ... from server\n");
            send(pollFds[i].fd,messageOfTheDay,(strlen(messageOfTheDay)+1),0);

            if(strcmp(clientMessage,"close\n")==0){
              doneReading=1;
            }
         }
         /*******************************/
         /*   EXIT READING FROM CLIENT */
         /******************************/
         if(doneReading){
           printf("closing client descriptor %d\n",pollFds[i].fd);
           close(pollFds[i].fd);
         }
        }


        /* MOVE ON TO NEXT POLL FD */
      }

    /* FOREVER RUNNING LOOP */ 
    }



 

  return 0;
}