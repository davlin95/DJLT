#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdbool.h>
#include <unistd.h>
#include <fcntl.h>
#include <errno.h>
#include <sys/poll.h>
#include <sys/types.h>
#include <sys/socket.h>


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

void printUserIn(char* user){
  if(user!=NULL){
  	char promptArrow[1024];
  	memset(&promptArrow,0,1024);
  	strcpy(promptArrow,user);
  	strcat(promptArrow," > ");
  	printf("%s",promptArrow);
  }
}

void printUserOut(char* user){
  if(user!=NULL){
  	char promptArrow[1024];
  	memset(&promptArrow,0,1024);
  	strcat(promptArrow," < ");
  	strcpy(promptArrow,user);
  	printf("%s",promptArrow);
  }
}

                /************************************/
                /*  Global Structures               */
                /************************************/

struct pollfd chatPollFds[1024];
int chatPollNum=0;

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


int main(int argc, char ** argv) {
	//INITIALIZE THE PASSED SOCKET PAIR
	int chatFd=0;
    if(argv[1]!=NULL && isAllDigits(argv[1]) ){
    	chatFd = atoi(argv[1]);
    	printf("chatFd is %d",chatFd);
    }else{
    	fprintf(stderr,"chat main(): error with argv 1\n");
    }

    //INITIALIZE COMMUNICATION USER
	char otherUser[1024];
	memset(otherUser,0,1024);
    if(argc>=2 && argv[2]!=NULL){
    	strcpy(otherUser,argv[2]);
    }else{
    	fprintf(stderr,"chat main(): error loading other User's name\n");
    }

    //INITIALIZE ORIGINAL USER
	char originalUser[1024];
	memset(originalUser,0,1024);
    if(argc>=3 && argv[3]!=NULL){
    	strcpy(originalUser,argv[3]);
    }else{
    	fprintf(stderr,"chat main(): error loading original User's name\n");
    }


    if(makeNonBlocking(chatFd)<0){
    	fprintf(stderr,"chat.c: error with makeNonBlockingForChat\n");
    }

    printUserIn(otherUser); 

	 /****************************************/
    /*        IMPLEMENT POLL                */
   /****************************************/

    /* Initialize Polls Interface*/
    memset(chatPollFds,0,sizeof(chatPollFds));
    char message[1024];
    int pollStatus;
    chatPollNum=2;

    /* Set poll for chatFd */
    chatPollFds[0].fd = chatFd;
    chatPollFds[0].events = POLLIN;

    /* Set poll for stdin */ 
    chatPollFds[1].fd = 0;
    chatPollFds[1].events = POLLIN; 
    if (makeNonBlocking(0)<0){ 
      fprintf(stderr, "Error making stdin nonblocking.\n");
    }

    /**** ETERNAL POLL LOOP ***/
    while(1){
      pollStatus = poll(chatPollFds, chatPollNum, -1);
      if(pollStatus<0){
        fprintf(stderr,"poll():%s\n",strerror(errno));
        break;
      } 

      /************************/
      /*   CHECK EVENTS LOOP */
      /**********************/
      int i; 
      for(i=0;i<chatPollNum;i++){
        if(chatPollFds[i].revents==0){
          continue; 
        } 
        if(chatPollFds[i].revents!=POLLIN){
          fprintf(stderr,"poll.revents:%s\n",strerror(errno));
          break;
        } 
        /***********************************/
        /*   POLLIN FROM CHATFD           */
        /*********************************/
        if(chatPollFds[i].fd == chatFd){
          //printStarHeadline("CLIENT TALKING TO THIS CHAT",-1);
          int clientBytes =0;
          while( (clientBytes = recv(chatFd, message, 1024, 0))>0){
            printUserIn(otherUser);
            printf("%s\n",message);
            printUserOut(originalUser);
            memset(&message,0,1024);   
          }
          if((clientBytes=read(chatFd,message,1))==0){
            printf("DETECTED client CLOSED, CLOSING chatFd\n");
            close(chatFd); 
            exit(0);
          }
        }
 
        /***********************************/
        /*   POLLIN FROM STDIN            */
        /*********************************/
        else if(chatPollFds[i].fd == 0){
          printStarHeadline("STDIN INPUT",-1);
        
          int bytes=0;
          char stdinBuffer[1024];  
          memset(&stdinBuffer,0,1024);
          while( (bytes=read(0,&stdinBuffer,1024))>0){
            send(chatFd,stdinBuffer,(strnlen(stdinBuffer,1023)),0);
            printUserOut(originalUser);
            memset(&stdinBuffer,0,1024);
          } 
        }

        /* MOVE ON TO NEXT POLL FD */
      }
    /* FOREVER RUNNING LOOP */ 
    }

  	return 0;
}