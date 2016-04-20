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
  int argCounter; 
  bool verbose;
  bool newUser;
  char *username;
  char *portNumber;
  char * argArray[1024];
  char * flagArray[1024];
  memset(&argArray, 0, sizeof(argArray));
  memset(&flagArray, 0, sizeof(flagArray));
  argCounter = initArgArray(argc, argv, argArray);
  if (argCounter != 4){
    USAGE("./client");
    exit(EXIT_FAILURE);
  } 
  initFlagArray(argc, argv, flagArray);
  argCounter = 0;
  while (flagArray[argCounter] != NULL){
    if (strcmp(flagArray[argCounter], "-h")==0){
      USAGE("./client");
      exit(EXIT_SUCCESS);
    }
    if (strcmp(flagArray[argCounter], "-v")==0){
      verbose = true;
    }
    if (strcmp(flagArray[argCounter], "-c")==0){
      newUser = true;
    }
    argCounter++;
  }
  portNumber = argArray[2];
  username = argArray[1];
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
        fprintf(stderr,"poll():%s\n",strerror(errno));
        break;
      }
      int i;
      for(i=0;i<pollNum;i++){
        if(pollFds[i].revents==0){
          continue; 
        } 
        if(pollFds[i].revents!=POLLIN){
          fprintf(stderr,"poll.revents:%s\n",strerror(errno));
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
            if (checkVerb(PROTOCOL_EMIT, message)){
              char sessionLength[1024];
              memset(&sessionLength, 0, 1024);
              if (extractArgAndTest(message, sessionLength)){
                displayClientConnectedTime(sessionLength);
              }
            }
            else if (checkVerb(PROTOCOL_UTSIL, message)){
              int length = strlen(message) - 4;
              char *protocolTerminator = (void *)message + length;
              if (strcmp(protocolTerminator, "\r\n\r\n") == 0){
                  char *messagePtr = (void *)message + 6;
                  printf("CONNECTED USERS\n");
                  write(1, messagePtr, length-3);
              }
            }
            else if (checkVerb(PROTOCOL_BYE, message)){
              printf("RECEIVED BYE FROM SERVER\n");
              close(clientFd);
              exit(EXIT_SUCCESS);
            }
            //IF RECEIVED MSG BACK FROM SERVER
            else if (  ){
            	printf("Received MSG from server\nCreating Xterm\n");
            	createXterm();
            }

            //printf("Data received: %s\n",message);
            memset(&message,0,1024);   
          }
          if((serverBytes=read(clientFd,message,1))==0){
            printf("DETECTED SERVER CLOSED, CLOSING CLIENTFD\n");
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
            /*send time verb to server*/
            if(strcmp(stdinBuffer,"/time\n")==0){
              protocolMethod(clientFd, TIME, NULL, NULL, NULL);
            }  
            else if(strcmp(stdinBuffer,"/listu\n")==0){
              protocolMethod(clientFd, LISTU, NULL, NULL, NULL);
            }
            else if(strcmp(stdinBuffer,"/logout\n")==0){
              protocolMethod(clientFd, BYE, NULL, NULL, NULL);
              waitForByeAndClose(clientFd);
              exit(EXIT_SUCCESS);
            }
            else if(strstr(stdinBuffer,"/chat")!=NULL){ // CONTAINS "/chat"
            	processChatCommand(clientFd,stdinBuffer,username);
            }
            else{
            	/***********TEST COMMUNICATING WITH SERVER ****************/
            	send(clientFd,stdinBuffer,(strlen(stdinBuffer)),0);
            	printf("sent string :%s from client to server\n",stdinBuffer);
            	memset(&stdinBuffer,0,strlen(stdinBuffer));
            }
          }
        }
        /* MOVE ON TO NEXT POLL FD */
      }
    /* FOREVER RUNNING LOOP */ 
    }
  return 0;
}

