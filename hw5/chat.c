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
#include "chatHeader.h"

               

int main(int argc, char ** argv) {
  signal(SIGKILL, endProcessHandler);
  signal(SIGINT,endProcessHandler);
	//INITIALIZE THE PASSED SOCKET PAIR
	  chatFd=0;
    if(argv[1]!=NULL && isAllDigits(argv[1]) ){
    	chatFd = atoi(argv[1]);
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