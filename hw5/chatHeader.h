#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <stdbool.h>
#include <fcntl.h>
#include <netinet/in.h>
#include <pthread.h>
#include <time.h>
#include <signal.h>
#include <arpa/inet.h>
#include <sys/fcntl.h>

#define VERBOSE "\x1b[1;34m"
#define ERROR "\x1b[1;31m"
#define DEFAULT "\x1b[0m"

#ifndef PROTOCOL_MSG
#define PROTOCOL_MSG "MSG"
#endif 

 				/************************************/
                /*  Global Structures               */
                /************************************/

struct pollfd chatPollFds[1024];
int chatPollNum=0;
int chatFd=-1;



#define USAGE(name) do {                                                                    \
        fprintf(stderr,                                                                         \
            "\n%s UNIX_SOCKET_FD\n"                                                             \
            "UNIX_SOCKET_FD       The Unix Domain File Descriptor number.\n"                  \
            ,(name)                                                                             \
        );                                                                                      \
    } while(0)


						/***********************************************************************/
						/*                    CHAT PROGRAM FUNCTIONS                         */
						/**********************************************************************/


/*
 *A function that makes the socket non-blocking
 *@param fd: file descriptor of the socket
 *@return: 0 if success, -1 otherwise
 */
int makeNonBlocking(int fd){
  int flags;
  if ((flags = fcntl(fd, F_GETFL, 0)) < 0){
    fprintf(stderr,"fcntl(): %s\n",strerror(errno));
    return -1;
  }
  flags |= O_NONBLOCK;
  if ((fcntl(fd, F_SETFL, flags)) < 0){
    fprintf(stderr,"fcntl(): %s\n",strerror(errno));
    return -1;
  }
  return 0;
}


bool isAllDigits(char* string){
  int i;
  char* checkPtr=string;
  for(i=0;i<strlen(string);i++){
    if(*checkPtr>'9' || *checkPtr < '0' ){
    	return false;
    }
  }
  return true;
}
  void endProcessHandler(){
    if(chatFd>0){
      close(chatFd);
      exit(0);
    }
  }

 void printStarHeadline(char* headline,int optionalFd){
  printf("\n/***********************************/\n");
  printf("/*\t");
  if(optionalFd>=0){
    printf("%-60s : %d",headline,optionalFd);
  }
  else {
    printf("%-60s",headline);
  }
  printf("\t*/\n");
  printf("/***********************************/\n");

 }

void printUserIn(){
  char rightPromptArrow[1024];
  memset(&rightPromptArrow,0,1024);
  strcat(rightPromptArrow," >");
  printf(VERBOSE"\n%s",rightPromptArrow);
}
 
void printUserOut(){
  	char promptArrow[1024];
  	memset(&promptArrow,0,1024);
  	strcat(promptArrow,"< ");
  	printf(ERROR "\n%s",promptArrow);
}

/*
 *  A function that takes a string and tests it for the structure of "MSG TOPERSON FROMPERSON MSG \r\n\r\n"
 *  returns true if passes format test. copies the message into the buffer.
 */
bool extractArgAndTestMSG(char *string, char* toBuffer, char* fromBuffer, char* messageBuffer){
    char * savePtr;
    char *token;
    int arrayIndex = 0;
    char tempString[1024];
    char * protocolArray[1024];

    //CLEAR BUFFERS
    memset(&tempString, 0, 1024);
    memset(&protocolArray, 0, 1024);

    //CHOP UP THE STRING
    strcpy(tempString, string);
    token = strtok_r(tempString, " ", &savePtr);
    while (token != NULL){
      protocolArray[arrayIndex++] = token;
      token = strtok_r(NULL," ",&savePtr);
    }

    //ERROR CHECK THE INPUT, 
    if ( arrayIndex < 5 ){
      //fprintf(stderr,"extractArgAndTestMSG() error: not enough arguments in buildProtocolMSGString");
      return false;
    }else if(strcmp(protocolArray[arrayIndex-1], "\r\n\r\n")!=0 ){
      //fprintf(stderr,"extractArgAndTestMSG() error: buildProtocolString() not ended with protocol ending");
      return false;
    }else if(strcmp(protocolArray[0], PROTOCOL_MSG)!=0){
     // fprintf(stderr,"extractArgAndTestMSG() error: buildProtocolString() not started with protocol string");
      return false;
    }

    //COPY THE TO PERSON IF POSSIBLE
    if(toBuffer!=NULL){
      strcpy(toBuffer,protocolArray[1]);
     // printf("extractArgAndTestMSG() copied TO:%s\n",toBuffer);
    }

    //COPY THE FROM PERSON IF POSSIBLE 
    if(fromBuffer!=NULL){
      strcpy(fromBuffer,protocolArray[2]);
     // printf("extractArgAndTestMSG() copied FROM:%s\n",fromBuffer);
    }
    //COPY ALL ARGS AFTER FROM PERSON, UP TO BEFORE THE PROTOCOL FOOTER
    int i;
    for(i=3; i<arrayIndex-1 ;i++){
      if(messageBuffer!=NULL){
          strcat(messageBuffer, protocolArray[i]);
          //DON"T ADD SPACE AFTER LAST MESSAGE WORD
          if(i!=arrayIndex-2){
            strcat(messageBuffer," ");
          }
      }
    }
    if(messageBuffer!=NULL){
      //printf("extractArgAndTestMSG() copied MESSAGE:%s",messageBuffer);
    }
    return true;
}


