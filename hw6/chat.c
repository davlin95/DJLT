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
  pthread_mutex_init(&lock, NULL);

	//INITIALIZE THE PASSED SOCKET PAIR
	  chatFd=0;
    if(argv[1]!=NULL && isAllDigits(argv[1]) ){
    	chatFd = atoi(argv[1]);
      makeNonBlocking(chatFd);
    }else{
      sfwrite(&lock, stderr, "chat main(): error with argv 1\n");
    	//fprintf(stderr,"chat main(): error with argv 1\n");
    }

    //INITIALIZE COMMUNICATION USER
	  char otherUser[1024];
	  memset(otherUser,0,1024);
    if(argc>=2 && argv[2]!=NULL){
    	strcpy(otherUser,argv[2]);
    }else{
      sfwrite(&lock, stderr, "chat main(): error loading other User's name\n");
    	//fprintf(stderr,"chat main(): error loading other User's name\n");
    }

    //INITIALIZE ORIGINAL USER 
	  char originalUser[1024];
	  memset(originalUser,0,1024);
    if(argc>=3 && argv[3]!=NULL){
    	strcpy(originalUser,argv[3]);
    }else{
      sfwrite(&lock, stderr, "chat main(): error loading original User's name\n");
    	//fprintf(stderr,"chat main(): error loading original User's name\n");
    }
    int auditFd = 0;
    if(argc>=4 && argv[4]!=NULL){
      auditFd = atoi(argv[4]);
    } else{
      sfwrite(&lock, stderr, "chat main(): error loading audit file descriptor\n");
    }





	 /****************************************/
    /*        IMPLEMENT POLL                */
   /****************************************/
 
    /* Initialize Polls Interface*/
    memset(chatPollFds,0,sizeof(chatPollFds));
    int pollStatus;
    chatPollNum=2;

    /* Set poll for chatFd */ 
    chatPollFds[0].fd = chatFd;
    chatPollFds[0].events = POLLIN; 
 
    /* Set poll for stdin */ 
    chatPollFds[1].fd = 0;
    chatPollFds[1].events = POLLIN; 
    if (makeNonBlocking(0)<0){ 
      sfwrite(&lock, stderr,"Error making stdin nonblocking.\n");
      //fprintf(stderr, "Error making stdin nonblocking.\n");
    }

    /**** ETERNAL POLL LOOP ***/
    while(1){
      pollStatus = poll(chatPollFds, chatPollNum, -1);
      if(pollStatus<0){
        sfwrite(&lock, stderr, "poll():%s\n",strerror(errno));
        //fprintf(stderr,"poll():%s\n",strerror(errno));
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
          sfwrite(&lock, stderr, "poll.revents:%s\n",strerror(errno));
          //fprintf(stderr,"poll.revents:%s\n",strerror(errno));
          break;  
        } 
        /***********************************/ 
        /*   POLLIN FROM CHATFD           */
        /*********************************/
        if(chatPollFds[i].fd == chatFd){
          int clientBytes =0;
          char message[1024];
          memset(&message,0,1024);   
          while( (clientBytes = recv(chatFd, message, 1024, 0))>0){
            write(1,VERBOSE,strlen(VERBOSE));
            char* inner = "\n> \0";
            write(1,inner,strlen(inner));
            write(1,message,strnlen(message,1023));
            write(1,ERROR,strlen(ERROR));
            char* outer = "\n< \0";
            write(1,outer,strlen(outer));
            memset(&message,0,1024);   
          }
          if((clientBytes=read(chatFd,message,1))==0){
            close(chatFd); 
            exit(0);
          }
        }
         
        /***********************************/
        /*   POLLIN FROM STDIN            */
        /*********************************/
        else if(chatPollFds[i].fd == 0){
          //printStarHeadline("STDIN INPUT",-1);
          int bytes=0; 
          char stdinBuffer[1024];  
          memset(&stdinBuffer,0,1024);
          while( (bytes=read(0,&stdinBuffer,1024))>0){
            if (strcmp(stdinBuffer, "/close\n")==0){
              char timeBuf[100];
              memset(timeBuf, 0, 100);
              time_t t;
              struct tm *tmp;
              t = time(NULL);
              tmp = localtime(&t);
              if (strftime(timeBuf, 100, "%D-%I:%M%P", tmp)==0){
                sfwrite(&lock, stderr, "strftime returned failed\n");
      //fprintf(stderr, "strftime returned failed\n");
                exit(1);
              }
              char closeBuf[1024];
              memset(&closeBuf, 0, 1024);
              sprintf(closeBuf, "%s, %s, %s", timeBuf, originalUser, "CMD, /close, success, chat\n");
              lockWriteUnlock(closeBuf, auditFd);
              exit(0);
            }
            send(chatFd,stdinBuffer,(strnlen(stdinBuffer,1023)),0);
            memset(&stdinBuffer,0,1024);
          }
          write(1,ERROR,strlen(ERROR));
          char* outer = "\n< \0";
          write(1,outer,strlen(outer));
        }

        /* MOVE ON TO NEXT POLL FD */
      }
    /* FOREVER RUNNING LOOP */ 
    }
    
  	return 0;
}