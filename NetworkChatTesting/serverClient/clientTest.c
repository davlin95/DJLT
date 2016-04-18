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
#include <sys/poll.h>
#include <signal.h>
#include "../../hw5/clientHeader.h"
  
int clientFd=-1; 
void killClientProgramHandler(int fd){  
    if(fd >0){ 
      close(fd);
    }
    printf("Clean exit on clientFd\n"); 
    exit(0);
}

int main(int argc, char* argv[]){ 
  char* username = "ELCHAPO1";
  char *portNumber = "1234"; 
  char message[1024]; 
  int step = 0;
  signal(SIGINT,killClientProgramHandler);

  if ((clientFd = createAndConnect(portNumber, clientFd)) < 0){
    fprintf(stderr, "Error creating socket and connecting to server. \n");
    exit(0);
  }
 
  /*********** NOTIFY SERVER OF CONNECTION *****/
  if (performLoginProcedure(clientFd, username) == 0){
      printf("Failed to login properly\n");
      close(clientFd);
      exit(0);
   }

  if (makeNonBlocking(clientFd)<0){   
    fprintf(stderr, "Error making socket nonblocking.\n");
  }

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
    
    if (makeNonBlocking(0)<0){
      fprintf(stderr, "Error making stdin nonblocking.\n");
    }
    int t=0;
    while(1){
      pollStatus = poll(pollFds, pollNum, -1);
      if(pollStatus<0){
        fprintf(stderr,"poll():%s",strerror(errno));
        break;
      }
      int i;
      for(i=0;i<pollNum;i++){
        if(pollFds[i].revents==0){
          continue; 
        } 
        if(pollFds[i].revents!=POLLIN){
          fprintf(stderr,"poll.revents:%s",strerror(errno));
          break;
        }
        /***********************************/
        /*   POLLIN FROM CLIENTFD         */
        /*********************************/
        if(pollFds[i].fd == clientFd){
          printf("\n/***********************************/\n");
          printf("/*   SERVER TALKING TO THIS CLIENT   */\n");
          printf("/***********************************/\n");
          int serverBytes =0;
          while( (serverBytes = recv(clientFd, message, 1024, 0))>0){
            printf("Data received: %s\n",message);
            memset(&message,0,1024);   
          }
          if((serverBytes=read(clientFd,message,1))==0){
            printf("CLOSING CLIENTFD\n");
            close(clientFd); 
            exit(0);
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
            printf("reading from client STDIN...\n");

            /****** @TODO Make logout connected to BYE\r\n\r\n **********/
            if(strcmp(stdinBuffer,"/logout\n")==0){
              close(clientFd);
              exit(EXIT_SUCCESS);
              break;
            }
            send(clientFd,stdinBuffer,(strlen(stdinBuffer)),0);
            printf("sent string :%s from client to server\n",stdinBuffer);
            memset(&stdinBuffer,0,strlen(stdinBuffer));
          }
        }
        /* MOVE ON TO NEXT POLL FD */
      }
    /* FOREVER RUNNING LOOP */ 
    }
  return 0;
}

