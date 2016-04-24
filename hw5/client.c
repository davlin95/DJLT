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
#include <sys/wait.h>
#include "xtermHeader.h"     


/*
 * A function that removes the dependent datastructures for the xterm, by clearing up its memory 
 * and refleting the changes in the clientPollFds structure. 
 */
void cleanUpXterm(Xterm* xterm){
  if(xterm!=NULL){
    int chatFd = xterm->chatFd;
    destroyXtermMemory(xterm);
    //REMOVE THE CHATFD FROM THE POLL STRUCTURE
    int i;
    for(i=0;i<clientPollNum;i++){
      if(clientPollFds[i].fd == chatFd){
        close(clientPollFds[i].fd);
        clientPollFds[i].fd = -1;
      }
    }
    compactClientPollDescriptors();
  }
}

void xtermReaperHandler(){
  pid_t pid;
  int reapStatus;
  while( (pid = waitpid(-1,&reapStatus,WUNTRACED) ) > 0 ){
    //NOTIFY THE ADMIN WHICH XTERM PROCESS DIED
    char deadChild[1024];
    memset(deadChild,0,1024); 
    sprintf(deadChild,"pid: %d died\n",pid);
    write(1,deadChild,1023);
  }

  //CLEAN UP CONTENTS For the chat index corresponding to this process pid. 
  Xterm* deadXterm = getXtermByPid(pid);
  if(deadXterm!=NULL){
    cleanUpXterm(deadXterm);
  }else{
    fprintf(stderr,"xtermReaperHandler(): deadXterm is NULL and can't be cleaned up\n");
  }
  //MOVE THE POLL AHEAD 
  writeToGlobalSocket();
}


void killClientProgramHandler(){
  printf("\n"); 
  protocolMethod(clientFd,BYE,NULL,NULL,NULL,verbose); 
  if(clientFd >0){  
    printf("closing clientFd cleanly\n");
    close(clientFd);
  }
  exit(0);
}


int main(int argc, char* argv[]){ 

  /* Attach Signal handlers */
  signal(SIGINT,killClientProgramHandler);  
  signal(SIGCHLD,xtermReaperHandler);

  //Program Startup Vars 
  int argCounter;   
  bool newUser;
  char *username;
  char *portNumber; 
  char* ipAddress;
  char message[1024];  
  char * argArray[1024];
  char * flagArray[1024];
  memset(&argArray, 0, sizeof(argArray));
  memset(&flagArray, 0, sizeof(flagArray));
  argCounter = initArgArray(argc, argv, argArray);

  //USER NEEDS HELP 
  if (argCounter != 5){
    USAGE("./client");
    exit(EXIT_FAILURE);
  } 

  // BUILD FLAG ARRAY 
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

  //INIT USER PASSED IN ARGS
  username = argArray[1];
  portNumber = argArray[2];
  ipAddress = argArray[3];
 
  // TRY CONNECTING TO SOCKET 
  if ((clientFd = createAndConnect(portNumber, clientFd,ipAddress)) < 0){
    fprintf(stderr, "Error creating socket and connecting to server. \n");
    exit(0);
  }
 
  /*********** NOTIFY SERVER OF CONNECTION *****/
  if (performLoginProcedure(clientFd, username, newUser) == 0){
      printf("Failed to login properly\n");
      close(clientFd);
      exit(0); 
  } 

   /******************************************/
   /*        IMPLEMENT POLL                 */
   /****************************************/
    /* Initialize Polls Interface*/
    memset(clientPollFds, 0 , sizeof(clientPollFds));
    int pollStatus;
    clientPollNum=3;

    // CREATE GLOBAL SOCKET PAIR 
    int globalSocketPair[2]={0,0};
    createSocketPair(globalSocketPair,2);
    globalSocket= globalSocketPair[0]; // assign to global var 

    /* Set poll for clientFd */ 
    clientPollFds[0].fd = clientFd; 
    clientPollFds[0].events = POLLIN;
    if (makeNonBlocking(clientFd)<0){   
      fprintf(stderr, "Error making socket nonblocking.\n");
    }  
    /* Set poll for stdin */ 
    clientPollFds[1].fd = 0;
    clientPollFds[1].events = POLLIN; 
    if (makeNonBlocking(0)<0){ 
      fprintf(stderr, "Error making stdin nonblocking.\n");
    }

    //UNBLOCK THE GLOBAL SOCKET/READ AND ADD THE GLOBAL READ SOCKET ON POLL WATCH
    if(makeNonBlocking(globalSocketPair[0])<0){
      fprintf(stderr, "Error making global socket 1 nonblocking.\n");
    }
    if(makeNonBlocking(globalSocketPair[1])<0){
      fprintf(stderr, "Error making global socket 2 nonblocking.\n");
    }
    clientPollFds[2].fd = globalSocketPair[1]; // hold onto the read pipe for the global var. 
    clientPollFds[2].events = POLLIN;


    /************************/
    /*   ETERNAL POLL       */
    /***********************/
    while(1){
      printf("waiting at poll()\n");
      pollStatus = poll(clientPollFds, clientPollNum, -1);
      if(pollStatus<0){
        fprintf(stderr,"poll(): %s\n",strerror(errno));
        continue;
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
          	if (verbose)
          		printf(VERBOSE "%s" DEFAULT, message);
            if (checkVerb(PROTOCOL_EMIT, message)){
              char sessionLength[1024]; 
              memset(&sessionLength, 0, 1024);
              if (extractArgAndTest(message, sessionLength)){
                displayClientConnectedTime(sessionLength);
              }   
            }  
            else if (checkVerb(PROTOCOL_UTSIL, message)){
            	char listOfUsers[1024];
            	memset(&listOfUsers, 0, 1024);
              	if (extractArgsAndTest(message, listOfUsers))
              		printf("%s\n", listOfUsers);
              	else
              		fprintf(stderr, "Error with UTSIL response\n");
            }
            else if (checkVerb(PROTOCOL_BYE, message)){ 
              printf("RECEIVED BYE FROM SERVER\n");
              close(clientFd);
              exit(EXIT_SUCCESS);
            }
            /***********************************/
            /* RECEIVED MSG BACK FROM SERVER  */
            /*********************************/
            else if (extractArgAndTestMSG(message,NULL,NULL,NULL) ){
            	char toUser[1024];
            	char fromUser[1024]; 
            	char messageFromUser[1024];
 
              //CLEAR BUFFERS BEFORE FILLING MESSAGE CONTACT INFO
            	memset(&toUser,0,strnlen(toUser,1024));
            	memset(&fromUser,0,strnlen(fromUser,1024));
            	memset(&messageFromUser,0,1024);
 
              // EXTRACT THE ARGS AND SEE IF IT'S VALID
            	if(extractArgAndTestMSG(message,toUser,fromUser,messageFromUser)){
                printStarHeadline("MESSAGE ADDRESS",-1);
            		printf("TO: %s, FROM: %s MESSAGE: %s\n",toUser,fromUser,messageFromUser);

                //IF NO OPEN CHAT FROM PREVIOUS CONTACT, AND MESSAGE ADDRESSED TO THIS CLIENT
                if(getXtermByUsername(fromUser)==NULL && strcmp(toUser,username)==0){
                    //CREATE CHAT BOX
                    printf("Creating Xterm for user: %s\n",fromUser);
                    createXterm(fromUser,username);

                    //SEND CHAT  
                    Xterm* xterm = getXtermByUsername(fromUser);
                    send(xterm->chatFd,messageFromUser,strnlen(messageFromUser,1023),0);
                }else{ 
                    //A CHAT ALREADY EXISTS, SEND TO IT
                    Xterm* xterm = getXtermByUsername(fromUser);
                    send(xterm->chatFd,messageFromUser,strnlen(messageFromUser,1023),0);
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
            /*send time verb to server*/
            if(strcmp(stdinBuffer,"/time\n")==0){
              protocolMethod(clientFd, TIME, NULL, NULL, NULL, verbose);
            }  
            else if(strcmp(stdinBuffer,"/listu\n")==0){
              protocolMethod(clientFd, LISTU, NULL, NULL, NULL, verbose); 
            }
            else if(strcmp(stdinBuffer,"/logout\n")==0){
              protocolMethod(clientFd, BYE, NULL, NULL, NULL, verbose);
              waitForByeAndClose(clientFd); 
              exit(EXIT_SUCCESS);   
            }  
            else if(strstr(stdinBuffer,"/chat")!=NULL){ 
              // CONTAINS "/chat"
            	processChatCommand(clientFd,stdinBuffer,username, verbose);
            }
            else if(strcmp(stdinBuffer,"/help\n")==0){  
            	displayHelpMenu(clientHelpMenuStrings);
            } 
            
          } 
        }
        /*******************************/
        /*  Catch Global Socket Write */
        /*****************************/
        else if(clientPollFds[i].fd == clientPollFds[2].fd){
          printf("catched read into global socket\n");
          char byte;
          read(clientPollFds[2].fd,&byte,1);
        }
        else{
          /****************************************/
          /*       USER TYPED INTO CHAT XTERM    */
          /**************************************/  

          //READ BYTES FROM CHAT BOX CHILD PROCESS
          char chatBuffer[1024];
          memset(chatBuffer,0,1024);  
          int chatBytes =-1;
          chatBytes = read(clientPollFds[i].fd,chatBuffer,1024);

          //IF XTERM DIED
          if(chatBytes==0){
            printf("xterm died detected in poll loop read\n");
            cleanUpXterm(getXtermByChatFd(clientPollFds[i].fd));
          }else{

              //BUILD MESSAGE PROTOCOL TO SEND TO SERVER/ TO BE RELAYED TO PERSON
              Xterm* xterm = getXtermByChatFd(clientPollFds[i].fd);
              if(xterm==NULL){
                  fprintf(stderr,"error in poll loop: getXtermByChatFd() returned NULL\n");
                  continue;//continue searching the rest of for loop for events
              }
              char relayMessage[1024];
              memset(&relayMessage,0,1024);
              if(buildMSGProtocol(relayMessage ,xterm->toUser, username, chatBuffer)==false){
                fprintf(stderr,"error in poll loop: buildMSGProtocol() unable to build relay message to server\n");
                continue; //continue searching the rest of for loop for events
              }
              chatBytes = send(clientFd,relayMessage,strnlen(relayMessage,1023),0);
          }
        }
        
      }/* MOVE ON TO NEXT POLL FD */
        
    }/* FOREVER RUNNING LOOP */ 


    return 0;
}

