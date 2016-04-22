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
      printf("waiting at chatPoll, chatPollNum = %d\n", chatPollNum);
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
          printStarHeadline("CLIENT TALKING TO THIS CHAT",-1);
          int clientBytes =0;
          while( (clientBytes = recv(chatFd, message, 1024, 0))>0){

 /*           if (checkVerb(PROTOCOL_EMIT, message)){
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
              printf("RECEIVED BYE FROM client\n");
              close(chatFd);
              exit(EXIT_SUCCESS);
            }*/

            //IF RECEIVED MSG BACK FROM CLIENT
          /*  else if ( extractArgAndTestMSG(message,NULL,NULL,NULL) ){
            	printf("TESTING RESULTS OF MSG PROTOCOL FROM client");
            	char toUser[1024];
            	char fromUser[1024]; 
            	char messageFromUser[1024];
 
            	memset(&toUser,0,strlen(toUser));
            	memset(&fromUser,0,strlen(fromUser));
            	memset(&messageFromUser,0,strlen(messageFromUser));
 
            	if(extractArgAndTestMSG(message,toUser,fromUser,messageFromUser)){
            		printf("TO: %s, FROM: %s MESSAGE: %s\n",toUser,fromUser,messageFromUser);
            	} 
            	printf("Creating Xterm, pollNum is %d\n", chatPollNum);
            	createXterm(toUser); 
            }  */

            printf("%s\n",message);
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
            printf("reading from CHAT STDIN...\n"); 
            /*send time verb to client*/

            /*
            if(strcmp(stdinBuffer,"/time\n")==0){
              protocolMethod(chatFd, TIME, NULL, NULL, NULL);
            }  
            else if(strcmp(stdinBuffer,"/listu\n")==0){
              protocolMethod(chatFd, LISTU, NULL, NULL, NULL);
            }
            else if(strcmp(stdinBuffer,"/logout\n")==0){
              protocolMethod(chatFd, BYE, NULL, NULL, NULL);
              waitForByeAndClose(chatFd);
              exit(EXIT_SUCCESS);
            } 
            else if(strstr(stdinBuffer,"/chat")!=NULL){ // CONTAINS "/chat"
            	processChatCommand(chatFd,stdinBuffer,username);
            }
            else{  */

            	/***********TEST COMMUNICATING WITH client ****************/
            	send(chatFd,stdinBuffer,(strnlen(stdinBuffer,1023)),0);
            	printf("sent string :%s from client to client\n",stdinBuffer);
            	memset(&stdinBuffer,0,strlen(stdinBuffer));
           // }

          } 
        }
        else{
          /*************** USER TYPED INTO CHAT XTERM **********/
        /*  printf("Just received write from chat xterm window fd at i= %d: %d\n",chatPollFds[i].fd,i);
          char chatBuffer[1024];
          memset(chatBuffer,0,1024);

          int chatBytes =-1;
          chatBytes = read(chatPollFds[i].fd,chatBuffer,1024);
          if(chatBytes>0){
            printf("Read: %s",chatBuffer);
          }
          char * helloChatBox = "Hello chatbox\n\0";
          chatBytes = send(chatPollFds[i].fd,helloChatBox,strlen(helloChatBox),0);
          printf("Just wrote into chat xterm window %d\n", chatBytes);*/
        }
        /* MOVE ON TO NEXT POLL FD */
      }
    /* FOREVER RUNNING LOOP */ 
    }

  	return 0;
}