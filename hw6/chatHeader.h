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
#include <sys/file.h>
#include "sfwrite.c"

#define VERBOSE "\x1b[1;34m"
#define ERROR "\x1b[1;31m"
#define DEFAULT "\x1b[0m"

        /************************************/
                /*  Global Structures               */
                /************************************/

struct pollfd chatPollFds[1024];
int chatPollNum=0;
int chatFd=-1;
pthread_mutex_t lock;



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


void lockWriteUnlock(char* text, int fd){
  flock(fd, LOCK_EX);
  write(fd, text, strlen(text));
  flock(fd, LOCK_UN);
}



