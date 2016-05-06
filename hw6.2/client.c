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
 * 1) test the global socket for chat xterms, 
 * see @questions for wilson, must be -c -v, other way doesn't work for flags. 
 */
 
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
  }else{
    fprintf(stderr,"cleanUpXterm(): error xterm is NULL\n");
  }
}


void xtermReaperHandler(){ 
  pid_t pid;
  int reapStatus; 
  pid = waitpid(-1,&reapStatus,WUNTRACED);
  while(pid > 0){
    //NOTIFY THE ADMIN WHICH XTERM PROCESS DIED

    //CLEAN UP CONTENTS For the chat index corresponding to this process pid. 
    Xterm* deadXterm = getXtermByPid(pid);
    if(deadXterm!=NULL){
      cleanUpXterm(deadXterm);
    }else{
      fprintf(stderr,"xtermReaperHandler(): deadXterm is NULL and can't be cleaned up\n");
    }

    //MOVE THE POLL AHEAD TO STOP DISABILITY, AND ATTEMPT TO RE-REAP
    writeToGlobalSocket();
    pid = waitpid(-1,&reapStatus,WUNTRACED);
  }
}


void killClientProgramHandler(){
  printf("\n"); 
  protocolMethod(clientFd,BYE,NULL,NULL,NULL,verbose); 
  if(clientFd >0){  
    close(clientFd);
  }
  exit(0);
}


int main(int argc, char* argv[]){ 

  /* Attach Signal handlers */
  signal(SIGINT,killClientProgramHandler);  
  signal(SIGCHLD,xtermReaperHandler);

  bool newUser;
  char *username = NULL;
  char *portNumber = NULL; 
  char *ipAddress = NULL; 
  char *filePtr;
  int c;
  char message[1024];
  //check flags in command line
  while ((c = getopt(argc, argv, "hcva:")) != -1){
    switch(c){
      case 'h':
        USAGE("./client");
        exit(EXIT_SUCCESS);
      case 'c':
        newUser = true;
        printf("new user\n");
        break;
      case 'v':
        printf("verbose\n");
        verbose = true;
        break;
      case 'a':
        if (optarg != NULL){
          filePtr = optarg;
          break;
        }
        else
      case '?': 
      default:
        USAGE("./client");
        exit(EXIT_FAILURE);
    }
  }
  //get username, ip, and address from command line
  if (optind < argc && (argc - optind) == 3){
    username = argv[optind++];
    ipAddress = argv[optind++];
    portNumber = argv[optind++];
  }
  else{
    USAGE("./client");
    exit(EXIT_FAILURE);
  }
  //initialize audit file
  FILE* auditFile = NULL;
  int auditFD = 0;
  auditFile = initAudit(filePtr);
  auditFD = fileno(auditFile);
  printf("auditFd is %d\n", auditFD);
  fprintf(auditFile, "%s\n", "yeahhhh boy");
  fclose(auditFile);

  char ipPort[1024];
  memset(&ipPort, 0, 1024);
  ipPortString(ipAddress, portNumber, ipPort); 
  char auditEvent[1024];
  // TRY CONNECTING TO SOCKET   
  if ((clientFd = createAndConnect(portNumber, clientFd,ipAddress)) < 0){
    createAuditEvent(username, "ERR, ", "ERR # message", NULL, NULL, auditEvent);
    printf("%s\n", auditEvent);
    exit(0); 
  }
  char loginMSG[1024];
  memset(&loginMSG, 0, 1024);
  /*********** NOTIFY SERVER OF CONNECTION *****/
  if (performLoginProcedure(clientFd, username, newUser, loginMSG) == 0){
      close(clientFd);
      createAuditEvent(username, "LOGIN", ipPort, "fail", loginMSG, auditEvent);
      printf("%s\n", auditEvent);
      exit(0); 
  } 
  createAuditEvent(username, "LOGIN", ipPort, "success", loginMSG, auditEvent);
  printf("%s\n", auditEvent);

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
              close(clientFd);
              exit(EXIT_SUCCESS);
            }
            else if (checkVerb(PROTOCOL_UOFF, message)){ 
              printf("received UOFF\n");
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

                //IF NO OPEN CHAT FROM PREVIOUS CONTACT, AND MESSAGE ADDRESSED TO THIS CLIENT
                if(getXtermByUsername(fromUser)==NULL && strcmp(toUser,username)==0){
                    //CREATE CHAT BOX
                    int child = createXterm(fromUser,username);
                    send(child,messageFromUser,strnlen(messageFromUser,1023),0);
                    createAuditEvent(username, "MSG", "from", fromUser, messageFromUser, auditEvent);
                    printf("%s\n", auditEvent);

                    /*//CONTINUE TO SEND CHAT  
                    Xterm* xterm = getXtermByUsername(fromUser);
                    if(xterm!=NULL){
                      send(xterm->chatFd,messageFromUser,strnlen(messageFromUser,1023),0);
                    }else{
                      fprintf(stderr,"createXterm() in poll: not initialized \n");
                    }*/
                }
                //IF CHAT EXISTS FROM PREVIOUS CONTACT, AND MESSAGE ADDRESSED TO THIS CLIENT
                else if(getXtermByUsername(fromUser)!=NULL && strcmp(toUser,username)==0){ 
                    Xterm* xterm = getXtermByUsername(fromUser);

                    //SAFE SEND 
                    if(xterm!=NULL){
                      send(xterm->chatFd,messageFromUser,strnlen(messageFromUser,1023),0);
                      createAuditEvent(username, "MSG", "from", fromUser, messageFromUser, auditEvent);
                      printf("%s\n", auditEvent);
                    }else{
                      fprintf(stderr,"poll() loop: receiving MSG from server from pre-existing chat user, but no chatbox found\n");
                    }
                }
                //IF CHATBOX DOESN'T EXIST FROM PREVIOUS CONTACT, BUT THIS CLIENT IS THE SENDER
                else if(getXtermByUsername(toUser)==NULL && strcmp(fromUser,username)==0){
                  //CREATE CHAT BOX
                  int child = createXterm(toUser,username);
                  send(child,messageFromUser,strnlen(messageFromUser,1023),0);
                  createAuditEvent(username, "CMD", "/chat", "success", "client", auditEvent);
                  printf("%s\n", auditEvent);
                  createAuditEvent(username, "MSG", "to", toUser, messageFromUser, auditEvent);
                  printf("%s\n", auditEvent);

                  /*//SAFE-SEND CHAT  
                  Xterm* xterm = getXtermByUsername(toUser);
                  if(xterm!=NULL){
                    send(xterm->chatFd,messageFromUser,strnlen(messageFromUser,1023),0);
                  }else{
                    fprintf(stderr,"poll() loop: receiving MSG from server from pre-existing chat user, but no chatbox found\n");
                  }*/

                }
            	} 
            } 
            else{
              createAuditEvent(username, "CMD", "/chat", "failure", "client", auditEvent);
              printf("%s\n", auditEvent);

            }
            memset(&message,0,1024);   
          
          }
          if((serverBytes=read(clientFd,message,1))==0){
            close(clientFd);  
            exit(0);  
          }
        }
 
        /***********************************/
        /*   POLLIN FROM STDIN            */
        /*********************************/
        else if(clientPollFds[i].fd == 0){
          int bytes=0;
          char stdinBuffer[1024];   
          memset(&stdinBuffer,0,1024); 
          while( (bytes=read(0,&stdinBuffer,1024))>0){
            stdinBuffer[strlen(stdinBuffer)-1] = 0;
            /*send time verb to server*/ 
            if(strcmp(stdinBuffer,"/time")==0){
              protocolMethod(clientFd, TIME, NULL, NULL, NULL, verbose);
              createAuditEvent(username, "CMD", stdinBuffer, "success", "client", auditEvent);
              printf("%s\n", auditEvent);
            }  
            else if(strcmp(stdinBuffer,"/listu")==0){  
              protocolMethod(clientFd, LISTU, NULL, NULL, NULL, verbose); 
              createAuditEvent(username, "CMD", stdinBuffer, "success", "client", auditEvent);
              printf("%s\n", auditEvent);
            }  
            else if(strcmp(stdinBuffer,"/logout")==0){
              protocolMethod(clientFd, BYE, NULL, NULL, NULL, verbose);
              waitForByeAndClose(clientFd); 
              createAuditEvent(username, "LOGOUT", "intentional", NULL, NULL, auditEvent);
              printf("%s\n", auditEvent);
              exit(EXIT_SUCCESS);   
            }  
            else if(strstr(stdinBuffer,"/chat")!=NULL){ 
              // CONTAINS "/chat"
              char userMsgBuffer[1024];
              memset(&userMsgBuffer, 0, 1024);
            	processChatCommand(clientFd,stdinBuffer,username, verbose);
            }
            else if(strcmp(stdinBuffer,"/help")==0){  
            	displayHelpMenu(clientHelpMenuStrings);
              createAuditEvent(username, "CMD", stdinBuffer, "success", "client", auditEvent);
              printf("%s\n", auditEvent);
            } 
            else if(strcmp(stdinBuffer,"/audit")==0){
              createAuditEvent(username, "CMD", stdinBuffer, "success", "client", auditEvent);
              printf("%s\n", auditEvent);
              //todo: print audit log
            }
            else{
              createAuditEvent(username, "CMD", stdinBuffer, "failure", "client", auditEvent);
              printf("%s\n", auditEvent);
              //todo:not a valid command
            }
            
          } 
        }
        /*******************************/
        /*  Catch Global Socket Write */
        /*****************************/
        else if(clientPollFds[i].fd == clientPollFds[2].fd){
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

          //IF CHILD XTERM DIED
          if(chatBytes==0){
            cleanUpXterm(getXtermByChatFd(clientPollFds[i].fd));
          }else{
              //BUILD MESSAGE PROTOCOL TO SEND TO SERVER/ TO BE RELAYED TO PERSON
              Xterm* xterm = getXtermByChatFd(clientPollFds[i].fd);
              if(xterm==NULL){
                  fprintf(stderr,"error in poll loop: getXtermByChatFd() returned NULL\n");
                  continue;//continue searching the rest of for loop for events
              }
              char userMsgBuffer[1024];
              memset(&userMsgBuffer, 0, 1024);
              char relayMessage[1024];
              memset(&relayMessage,0,1024);
              if(buildMSGProtocol(relayMessage ,xterm->toUser, username, chatBuffer)==false){
                fprintf(stderr,"error in poll loop: buildMSGProtocol() unable to build relay message to server\n");
                continue; //continue searching the rest of for loop for events
              }
              chatBytes = send(clientFd,relayMessage,strnlen(relayMessage,1023),0);
              createAuditEvent(username, "MSG", "to", xterm->toUser, chatBuffer, auditEvent);
              printf("%s\n", auditEvent);

          }
        }
         
      }/* MOVE ON TO NEXT POLL FD */ 

    }/* FOREVER RUNNING LOOP */ 
        
    return 0;
}

