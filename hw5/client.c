#include <stdio.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <unistd.h>
#include <netdb.h>
#include <string.h> 
#include <stdlib.h> 
#include <errno.h>
#include <sys/fcntl.h>
#include <signal.h>
#include "xtermHeader.h"    

int clientFd=-1; 
bool existsChat=false;
 
void killClientProgramHandler(int fd){  
    if(fd >0){ 
      close(fd);
    }
    printf("Clean exit on clientFd\n"); 
    exit(0);
}

void handleKilledChat(){
	existsChat = false;
	printf("chat died\n");
}
 
int main(int argc, char* argv[]){ 
  signal(SIGCHLD,handleKilledChat);
  initializeChatGlobals();
  int argCounter; 
  //bool verbose;
  //bool newUser;
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
      //verbose = true;
    }
    if (strcmp(flagArray[argCounter], "-c")==0){
      //newUser = true;
    }
    argCounter++;
  }
  portNumber = argArray[2];
  username = argArray[1];
  char message[1024]; 
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
    memset(clientPollFds, 0 , sizeof(clientPollFds));
    int pollStatus;
    clientPollNum=2;

    /* Set poll for clientFd */
    clientPollFds[0].fd = clientFd;
    clientPollFds[0].events = POLLIN;

    /* Set poll for stdin */ 
    clientPollFds[1].fd = 0;
    clientPollFds[1].events = POLLIN; 
 
    if (makeNonBlocking(0)<0){ 
      fprintf(stderr, "Error making stdin nonblocking.\n");
    }
    while(1){
      pollStatus = poll(clientPollFds, clientPollNum, -1);
      if(pollStatus<0){
        fprintf(stderr,"poll(): %s\n",strerror(errno));
        break;
      } 
      int i; 
      for(i=0;i<clientPollNum;i++){
        if(clientPollFds[i].revents==0){
          continue; 
        } 
        if(clientPollFds[i].revents!=POLLIN){
          fprintf(stderr,"poll.revents:%s\n",strerror(errno));
          break;
        }  
        /***********************************/
        /*   POLLIN FROM CLIENTFD         */
        /*********************************/
        if(clientPollFds[i].fd == clientFd){
          printStarHeadline("SERVER TALKING TO THIS CLIENT",-1);
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
            else if ( extractArgAndTestMSG(message,NULL,NULL,NULL) ){
            	char toUser[1024];
            	char fromUser[1024]; 
            	char messageFromUser[1024];
 
              //CLEAR BUFFERS BEFORE FILLING MESSAGE CONTACT INFO
            	memset(&toUser,0,strnlen(toUser,1024));
            	memset(&fromUser,0,strnlen(fromUser,1024));
            	memset(&messageFromUser,0,strnlen(messageFromUser,1023));
 
              // EXTRACT THE ARGS AND SEE IF IT'S VALID
            	if(extractArgAndTestMSG(message,toUser,fromUser,messageFromUser)){
                printStarHeadline("MESSAGE ADDRESS",-1);
            		printf("TO: %s, FROM: %s MESSAGE: %s\n",toUser,fromUser,messageFromUser);

                //IF NO OPEN CHAT FROM PREVIOUS CONTACT, AND MESSAGE ADDRESSED TO THIS CLIENT
                if(getChatFdFromUsername(fromUser)<0 && strcmp(toUser,username)==0){
                    //CREATE CHAT BOX
                    printf("Creating Xterm for user: %s\n",fromUser);
                    createXterm(fromUser,username);
                    //SEND CHAT  
                    int chatBox = getChatFdFromUsername(fromUser);
                    send(chatBox,messageFromUser,strnlen(messageFromUser,1023),0);


                }else{ //A CHAT ALREADY EXISTS
                    //SEND CHAT 
                    int chatBox = getChatFdFromUsername(fromUser);
                    send(chatBox,messageFromUser,strnlen(messageFromUser,1023),0);
                }

            	} 
            }

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
        else if(clientPollFds[i].fd == 0){
          printStarHeadline("STDIN INPUT",-1);
         
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
            	memset(&stdinBuffer,0,strnlen(stdinBuffer,1024));
            }

          } 
        }
        else{
          /****************************************/
          /*       USER TYPED INTO CHAT XTERM    */
          /**************************************/  
          char chatBuffer[1024];
          memset(chatBuffer,0,1024);  

          //READ BYTES FROM CHAT BOX CHILD PROCESS
          int chatBytes =-1;
          chatBytes = read(clientPollFds[i].fd,chatBuffer,1024);
          if(chatBytes>0){
            printStarHeadline("READ FROM CHATBOX: ",-1);
            printf("%s",chatBuffer);
          }

          //BUILD MESSAGE PROTOCOL TO SEND TO SERVER/ TO BE RELAYED TO PERSON
          char* toPerson = getChatUsernameFromChatFd(clientPollFds[i].fd);
          if(toPerson==NULL){
            fprintf(stderr,"error in poll loop: getChatUsernameFromChatFd() returned NULL person string\n");
          }
          char relayMessage[1024];
          memset(&relayMessage,0,1024);
          if(buildMSGProtocol(relayMessage,toPerson,username, chatBuffer)==false){
              fprintf(stderr,"error in poll loop: buildMSGProtocol() unable to build relay message to server\n");
          }
          chatBytes = send(clientFd,relayMessage,strnlen(relayMessage,1024),0);
        }



        /* MOVE ON TO NEXT POLL FD */
      }

    /* FOREVER RUNNING LOOP */ 
    }
  return 0;
}

