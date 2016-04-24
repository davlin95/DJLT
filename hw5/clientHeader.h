#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <stdbool.h>
#include <netinet/in.h>
#include <pthread.h>
#include <time.h>
#include <sys/poll.h>
#include <arpa/inet.h>
#include <sys/fcntl.h>
#include "WolfieProtocolVerbs.h"
#include <sys/stat.h>
#include <sys/types.h>
#include <sys/socket.h>


                /************************************/
                /*  Global Structures               */
                /************************************/

struct pollfd clientPollFds[1024];
int clientPollNum;
int verbose = 0; 
int globalSocket;

#define USAGE(name) do {                                                                        \
        fprintf(stderr,                                                                         \
            "\n%s [-hcv] NAME SERVER_IP SERVER_PORT\n"                                          \
            "-h           Displays help menu & returns EXIT_SUCCESS.\n"                         \
            "-c           Requests to server to create a new user.\n"                           \
            "-v           Verbose print all incoming and outgoing protocol verbs & content.\n"  \
            "NAME         This is the username to display when chatting.\n"                     \
            "SERVER_IP    The ipaddress of the server to connect to.\n"                         \
            "SERVER_PORT  The port to connect to.\n"                                            \
            ,(name)                                                                             \
        );                                                                                      \
    } while(0)

char* clientHelpMenuStrings[]={"/help \t List available commands.", "/listu \t List connected users.", "/time \t Display connection duration with Server.", "/chat <to> <msg> \t Send message (<msg>) to user (<to>).", "/logout \t Disconnect from server.", NULL};

						/***********************************************************************/
						/*                    CLIENT PROGRAM FUNCTIONS                         */
						/**********************************************************************/

 /*
 * A client side function that convert's the server's returned connection time into user-friendly printout format. 
 * @param time: time conneted on the server
 * @return: void
 */
  void displayClientConnectedTime(char* sessionLength){
    int time = atoi(sessionLength);
    int hour;
    int minute;
    int second;
    hour = time/3600;
    minute = (time%3600)/60;
    second = (time%3600)%60;
    printf("connected for %d hour(s), %d minute(s), and %d second(s)\n", hour, minute, second);
  }


 /********** LOGISTICAL I/O FUNCTIONS *******/
 /* 
  * A function that disconnects the user from the server
  * 
  */ 
 void logout();

 							/************* BUILT-IN FUNCTIONS *********/


 							/************* ASK SERVER FUNCTIONS ******/
 /* 
  * A function that asks the server who has been connected
  * @param serverFd: the connected server file descriptor
  * 
  */ 
 void listU(int serverFd);


 			/***********************************************************************/
            /*                    CREATE SOCKET FUNCTIONS                         */
            /**********************************************************************/

/*
 *A function that creates necessary addrinfo structs and initializes the addrinfo list
 *@param results: addrinfo list to be initialized
 *@param portNumber: portNumber used for getaddrinfo method
 *@return: initialized addrinfo list results
 */
struct addrinfo * buildAddrInfoStructs(struct addrinfo *results, char* portNumber, char* ipAddress){
  int status;
  struct addrinfo settings;
  memset(&settings,0,sizeof(settings));
  settings.ai_family=AF_INET;
  settings.ai_socktype=SOCK_STREAM;
  status = getaddrinfo(ipAddress,portNumber,&settings,&results);
  if(status!=0){
    fprintf(stderr,"getaddrinfo():%s\n",gai_strerror(status));
    return NULL;
  }
  printf("getaddrinfo successful()\n");
  return results;
}
/*
 *A function that makes the socket reusable
 *@param fd: file descriptor of the socket
 *@return: 0 if success, -1 otherwise
 */
int makeReusable(int fd){
  int val=1;
  if ((setsockopt(fd, SOL_SOCKET,SO_REUSEADDR,&val,sizeof(val))) < 0){
      fprintf(stderr,"setsockopt(): %s\n",strerror(errno)); 
      return -1;
  }
  return 0;
}


/*
 *A function that creates a socket and connects it to the server
 *@param portNumber: portNumber of the Server
 *@param clientFd: socket to be initialized
 *@return: clientFD, -1 otherwise
 */
int createAndConnect(char* portNumber, int clientFd, char* ipAddress){
  struct addrinfo resultsStruct;
  struct addrinfo* results=&resultsStruct;
  if((results = buildAddrInfoStructs(results, portNumber, ipAddress)) == NULL){
    fprintf(stderr,"createAndConnect(): unable to buildAddrInfoStructs()\n");
    return -1;
  }
  if ((clientFd = socket(results->ai_family, results->ai_socktype, results->ai_protocol)) < 0){
    fprintf(stderr,"createAndConnect(): unable to build socket\n");
    freeaddrinfo(results);
    return -1;
  }
  if (connect(clientFd, results->ai_addr, results->ai_addrlen)!=-1){
    fprintf(stderr,"createAndConnect(): Able to connect to socket\n");
    freeaddrinfo(results);
    return clientFd;
  }
  if (close(clientFd) < 0){
    fprintf(stderr,"close(): %s\n",strerror(errno));
  }
  freeaddrinfo(results);
  return -1;
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

int makeBlocking(int fd){
  int flags = fcntl(fd, F_GETFL);
  fcntl(fd, F_SETFL, flags & ~O_NONBLOCK);
  return 1;
}

void waitForByeAndClose(int clientFd){
  //WAITING FOR BYE FROM SERVER
  char byeBuffer[1024];
  memset(&byeBuffer,0,1024);
  makeBlocking(clientFd);
  int byeBytes=-1;  
  byeBytes = read(clientFd,&byeBuffer,1024);
  if(byeBytes > 0){
    if(checkVerb(PROTOCOL_BYE,byeBuffer)){
      printf("Valid BYE FROM SERVER\n");
    } 
  }
  printf("CLOSING SERVER\n");
  close(clientFd);
}

bool protocol_Login_Helper(char *verb, char* string, char *stringToMatch){
    char argBuffer[1024];
    memset(&argBuffer, 0, 1024);
    if (checkVerb(verb, string)){
      if (extractArgAndTest(string, argBuffer)){
        if(strcmp(argBuffer, stringToMatch) == 0)
          return true;
      }
    }
    return false;
}


bool performLoginProcedure(int fd,char* username, bool newUser){ 
  char protocolBuffer[1024];
  char *messageArray[1024];
  int noOfMessages;
  protocolMethod(fd, WOLFIE, NULL, NULL, NULL, verbose);  
  //----------------------------------||
  //     READ RESPONSE FROM CLIENT    ||                       
  //----------------------------------||
  /*
  if (Read(fd, protocolBuffer))
    return false;
  */
  memset(&protocolBuffer,0,1024);
  if (read(fd, &protocolBuffer,1024) < 0){
    fprintf(stderr,"Read(): bytes read negative\n");
    return false;
  }
  if (verbose)
      printf(VERBOSE "%s" DEFAULT, protocolBuffer);
  //----------------------------------------------|| 
  //      CHECK IF EXPECTED: EIFLOW \r\n\r\n      ||
  //----------------------------------------------||
  if(checkVerb(PROTOCOL_EIFLOW, protocolBuffer)){
    if (!newUser)
      protocolMethod(fd, IAM, username, NULL,NULL, verbose);
    else
      protocolMethod(fd, IAMNEW, username, NULL, NULL, verbose); 
  }
  else{
    printf("Expected protocol verb EIFLOW\n");
    return false;
  }
  //----------------------------------||
  //     READ RESPONSE FROM SERVER    ||                       
  //----------------------------------||
  /*
  if (Read(fd, protocolBuffer))
    return false;
    */
  memset(&protocolBuffer,0,1024);
  if (read(fd, &protocolBuffer,1024) < 0){
    fprintf(stderr,"Read(): bytes read negative\n");
    return false;
  }
  //----------------------------------------------------|| 
  //      CHECK IF EXPECTED: ERR0 or ERR1 and BYE       ||
  //----------------------------------------------------|| 
  if (checkVerb(PROTOCOL_ERR0, protocolBuffer) || checkVerb(PROTOCOL_ERR1, protocolBuffer)){
    memset(&messageArray, 0, 1024);
    if ((noOfMessages = getMessages(messageArray, protocolBuffer))>1){
      if (verbose)
        printf(ERROR "%s" DEFAULT, messageArray[0]);
      printf("Received Error and bye together\n");
      if (checkVerb(PROTOCOL_BYE, messageArray[1])){
        if (verbose)
          printf(VERBOSE "%s" DEFAULT, messageArray[1]);
        fprintf(stderr, "Error sending username, received ERR and BYE\n");
        return false;
      }
    }
    printf("Didn't receive Error and bye together\n");
    if (verbose)
        printf(ERROR "%s" DEFAULT, protocolBuffer);
    memset(&protocolBuffer,0,1024);
    if (read(fd, &protocolBuffer,1024) < 0){
      fprintf(stderr,"Read(): bytes read negative\n");
    return false;
    }
    if (verbose)
        printf(VERBOSE "%s" DEFAULT, protocolBuffer);
    if (checkVerb(PROTOCOL_BYE, protocolBuffer)){
        fprintf(stderr, "Error sending username, received ERR and BYE\n");
        return false;
    }
  }
  //---------------------------------------------|| 
  //      CHECK IF EXPECTED: HINEW or AUTH       ||
  //---------------------------------------------||
  if (verbose)
        printf(VERBOSE "%s" DEFAULT, protocolBuffer);
  if (protocol_Login_Helper(PROTOCOL_HINEW, protocolBuffer, username) || checkVerb(PROTOCOL_AUTH, protocolBuffer)){
    char password[1024];
    memset(&password,0,1024);
    strcpy(password, getpass("password: "));
    if (!newUser)
      protocolMethod(fd, PASS, password, NULL, NULL, verbose);
    else
      protocolMethod(fd, NEWPASS, password, NULL, NULL, verbose);
  }
  else {
    printf("Expected protocol verb AUTH or HINEW\n");
    return false;
  }
  //----------------------------------|| 
  //     READ RESPONSE FROM SERVER    ||                       
  //----------------------------------||
  memset(&protocolBuffer,0,1024);
  if (read(fd,&protocolBuffer,1024) < 0){
    fprintf(stderr,"performLoginProcedure(): bytes read negative\n");
    return false;
  }
  //--------------------------------------------|| 
  //      CHECK IF EXPECTED: ERR2 and BYE       ||
  //--------------------------------------------||
  if (checkVerb(PROTOCOL_ERR2, protocolBuffer)){
    memset(&messageArray, 0, 1024);
    if ((noOfMessages = getMessages(messageArray, protocolBuffer))>1){
      printf("Received Error and bye together\n");
      if (verbose)
        printf(ERROR "%s" DEFAULT, messageArray[0]);
      if (checkVerb(PROTOCOL_BYE, messageArray[1])){
        if (verbose)
          printf(VERBOSE "%s" DEFAULT, messageArray[1]);
        fprintf(stderr, "Error sending password, received ERR and BYE\n");
        return false;
      }
    }
    printf("Didn't receive Error and bye together\n");
    if (verbose)
        printf(ERROR "%s" DEFAULT, protocolBuffer);
    memset(&protocolBuffer,0,1024);
    if (read(fd, &protocolBuffer,1024) < 0){
      fprintf(stderr,"Read(): bytes read negative\n");
      return false;
    }
    if (verbose)
        printf(VERBOSE "%s" DEFAULT, protocolBuffer);
    if (checkVerb(PROTOCOL_BYE, protocolBuffer)){
        fprintf(stderr, "Error sending password, received ERR and BYE\n");
        return false;
    }
  }
  //-----------------------------------------------------------|| 
  //      CHECK IF EXPECTED: SSAPWEN or SSAP, HI and MOTD      ||
  //-----------------------------------------------------------||
  if (checkVerb(PROTOCOL_SSAPWEN, protocolBuffer) || checkVerb(PROTOCOL_SSAP, protocolBuffer)){
    char motd[1024];
    memset(&motd, 0, 1024);
    memset(&messageArray, 0, 1024);
    if ((noOfMessages = getMessages(messageArray, protocolBuffer))>1){
      if (verbose)
        printf(VERBOSE "%s" DEFAULT, messageArray[0]);
      if (checkVerb(PROTOCOL_HI, messageArray[1])){
        if (verbose)
          printf(VERBOSE "%s" DEFAULT, messageArray[1]);
        if (noOfMessages == 3){
          printf("Received SSAPWEN/SSAP, HI, and MOTD together.\n");
          if (checkVerb(PROTOCOL_MOTD, messageArray[2])){
            if (verbose)
              printf(VERBOSE "%s" DEFAULT, messageArray[2]);
            if (extractArgsAndTest(messageArray[2], motd)){
              printf("%s\n", motd);
              return true;
            } 
          }
          else{
            fprintf(stderr, "Didn't receive MOTD\n");
            return false;
          }
        }
        memset(&protocolBuffer, 0, 1024);
        read(fd,&protocolBuffer,1024);
        if (verbose)
          printf(VERBOSE "%s" DEFAULT, protocolBuffer);
        if (checkVerb(PROTOCOL_MOTD, protocolBuffer)){
            if (extractArgsAndTest(protocolBuffer, motd)){
              printf("%s\n", motd);
              return true;
            }
        }
        else{
            fprintf(stderr, "Didn't receive MOTD\n");
            return false;
        }
      }
      else{
        fprintf(stderr, "Didn't receive HI\n");
      }
    }
    printf("Didn't receive SSAPWEN/SSAP, HI, and MOTD together\n");
    memset(&protocolBuffer, 0, 1024);
    if (read(fd, &protocolBuffer,1024) < 0){
      fprintf(stderr,"Read(): bytes read negative\n");
    return false;
    }
    memset(&messageArray, 0, 1024);
    if ((noOfMessages = getMessages(messageArray, protocolBuffer))>1){
      if (verbose)
        printf(VERBOSE "%s" DEFAULT, messageArray[0]);
      if (checkVerb(PROTOCOL_HI, messageArray[0])){
        if (verbose)
          printf(VERBOSE "%s" DEFAULT, messageArray[0]);
        if (checkVerb(PROTOCOL_MOTD, messageArray[1])){
            if (verbose)
              printf(VERBOSE "%s" DEFAULT, messageArray[1]);
            if (extractArgsAndTest(messageArray[1], motd)){
              printf("%s\n", motd);
              return true;
            }
          }
        }
      }
      printf("noOfMessages: %d\n", noOfMessages);
    if (protocol_Login_Helper(PROTOCOL_HI, protocolBuffer, username)){
      if (verbose)
          printf(VERBOSE "%s" DEFAULT, protocolBuffer);
      memset(&protocolBuffer, 0, 1024);
      if (read(fd, &protocolBuffer,1024) < 0){
        fprintf(stderr,"Read(): bytes read negative\n");
      return false;
      }
      if (verbose)
          printf(VERBOSE "%s" DEFAULT, protocolBuffer);
      if (checkVerb(PROTOCOL_MOTD, protocolBuffer)){
          if (extractArgsAndTest(protocolBuffer, motd)){
              printf("%s\n", motd);
              return true;
          }
        else
          fprintf(stderr, "Expected verb MOTD");
      }
      else 
        printf("Expected MOTD");
    }
    else
      printf("protocol_Login_Helper failed\n");
  }
  else 
    printf("Expected SSAP or SSAPWEN\n");
  return false; 
}

void recognizeAndExecuteStdin(char* userTypedIn){
  printf("user typed in: %s", userTypedIn);
  if(strcmp(userTypedIn,"/users\n")==0){ 
    //PRINT OUT USERS
    //processUsersRequest();
  }else if(strcmp(userTypedIn,"/help\n")==0){
    //PRINT OUT HELP
    displayHelpMenu(clientHelpMenuStrings);
  }else if(strcmp(userTypedIn,"/shutdown\n")==0){
    //SHUTDOWN
    //processShutdown();
  }
}




                  