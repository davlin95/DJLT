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
#include <sys/file.h>
#include <sys/stat.h>


                /************************************/
                /*  Global Structures               */
                /************************************/
int clientFd=-1; 
struct pollfd clientPollFds[1024];
int clientPollNum;
int verbose = 0; 
int globalSocket;
pthread_mutex_t lock;

#define USAGE(name) do {                                                                        \
        fprintf(stderr,                                                                         \
            "\n%s [-hcv] [-a FILE] NAME SERVER_IP SERVER_PORT\n"                                \
            "-a FILE      Path to the audit log file.\n"                                        \
            "-h           Displays help menu & returns EXIT_SUCCESS.\n"                         \
            "-c           Requests to server to create a new user.\n"                           \
            "-v           Verbose print all incoming and outgoing protocol verbs & content.\n"  \
            "NAME         This is the username to display when chatting.\n"                     \
            "SERVER_IP    The ipaddress of the server to connect to.\n"                         \
            "SERVER_PORT  The port to connect to.\n"                                            \
            ,(name)                                                                             \
        );                                                                                      \
    } while(0)

char* clientHelpMenuStrings[]={"/help \t List available commands.", "/listu \t List connected users.", "/time \t Display connection duration with Server.", "/chat <to> <msg> \t Send message (<msg>) to user (<to>).", "/audit \t Show contents of audit log file.", "/logout \t Disconnect from server.", NULL};

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
    sfwrite(&lock, stdout, "connected for %d hour(s), %d minute(s), and %d second(s)\n", hour, minute, second);
    //printf("connected for %d hour(s), %d minute(s), and %d second(s)\n", hour, minute, second);
  }

void compactClientPollDescriptors(){
  int i,j;
  for (i=0; i<clientPollNum; i++) 
  {
    // IF ENCOUNTER A CLOSED FD
    if (clientPollFds[i].fd == -1)
    {
      // SHIFT ALL SUBSEQUENT ELEMENTS LEFT BY ONE SLOT
      for(j = i; j < clientPollNum; j++)
      {
        clientPollFds[j].fd = clientPollFds[j+1].fd;
      }
      clientPollNum--;
    }
  }
}


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
    sfwrite(&lock, stderr,"getaddrinfo():%s\n",gai_strerror(status));
    //fprintf(stderr,"getaddrinfo():%s\n",gai_strerror(status));
    return NULL;
  }
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
    sfwrite(&lock, stderr, "setsockopt(): %s\n",strerror(errno));
      //fprintf(stderr,"setsockopt(): %s\n",strerror(errno)); 
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
    sfwrite(&lock, stderr, "createAndConnect(): unable to buildAddrInfoStructs()\n");
    //fprintf(stderr,"createAndConnect(): unable to buildAddrInfoStructs()\n");
    return -1;
  }
  if ((clientFd = socket(results->ai_family, results->ai_socktype, results->ai_protocol)) < 0){
    sfwrite(&lock, stderr, "createAndConnect(): unable to build socket\n");
    //fprintf(stderr,"createAndConnect(): unable to build socket\n");
    freeaddrinfo(results);
    return -1;
  }
  if (connect(clientFd, results->ai_addr, results->ai_addrlen)!=-1){
    //sfwrite(&lock, stderr, "createAndConnect(): Able to connect to socket\n");
    //fprintf(stderr,"createAndConnect(): Able to connect to socket\n");
    freeaddrinfo(results);
    return clientFd;
  }
  if (close(clientFd) < 0){
    sfwrite(&lock, stderr, "close(): %s\n",strerror(errno));
    //fprintf(stderr,"close(): %s\n",strerror(errno));
  }
  freeaddrinfo(results);
  return -1;
}


 // void printStarHeadline(char* headline,int optionalFd){

 //  printf("\n/***********************************/\n");
 //  printf("/*\t");
 //  if(optionalFd>=0){
 //    printf("%-40s : %d",headline,optionalFd);
 //  }
 //  else {
 //    printf("%-40s",headline);
 //  }
 //  printf("\t*/\n");
 //  printf("/***********************************/\n");
 // }

/*
 *A function that makes the socket non-blocking
 *@param fd: file descriptor of the socket
 *@return: 0 if success, -1 otherwise
 */
int makeNonBlocking(int fd){
  int flags;
  if ((flags = fcntl(fd, F_GETFL, 0)) < 0){
    sfwrite(&lock, stderr, "fcntl(): %s\n",strerror(errno));
    //fprintf(stderr,"fcntl(): %s\n",strerror(errno));
    return -1;
  }
  flags |= O_NONBLOCK;
  if ((fcntl(fd, F_SETFL, flags)) < 0){
    sfwrite(&lock, stderr, "fcntl(): %s\n",strerror(errno));
    //fprintf(stderr,"fcntl(): %s\n",strerror(errno));
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
      sfwrite(&lock, stderr, "Valid BYE FROM SERVER\n");
      //printf("Valid BYE FROM SERVER\n");
    } 
  }
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

void ipPortString(char *ip, char *port, char* strBuffer){
  strcat(strBuffer, ip);
  strcat(strBuffer, ":");
  strcat(strBuffer, port);
}


void createAuditEvent(char *username, char* event, char * info, char *info2, char *info3, char *strBuffer){
  memset(strBuffer, 0, 1024);
  time_t t;
  struct tm *tmp;
  t = time(NULL);
  tmp = localtime(&t);
  if (strftime(strBuffer, 100, "%D-%I:%M%P", tmp)==0){
    sfwrite(&lock, stderr, "strftime returned failed\n");
      //fprintf(stderr, "strftime returned failed\n");
  }
  strcat(strBuffer, ", ");
  strcat(strBuffer, username);
  strcat(strBuffer, ", ");
  strcat(strBuffer, event);
  strcat(strBuffer, ", ");
  strcat(strBuffer, info);
  if (info2 != NULL){
    strcat(strBuffer, ", ");
    strcat(strBuffer, info2);
    strcat(strBuffer, ", ");
    strcat(strBuffer, info3);
  }
  strcat(strBuffer, "\n");
}


bool performLoginProcedure(int fd,char* username, bool newUser, char *loginMSG){ 
  char protocolBuffer[1024];
  char *messageArray[1024];
  int noOfMessages;

  //SEND WOLFIE TO SERVER
  protocolMethod(fd, WOLFIE, NULL, NULL, NULL, verbose, &lock);  

  //----------------------------------||
  //     READ RESPONSE FROM FD        ||                       
  //----------------------------------||
  memset(&protocolBuffer,0,1024);
  if (read(fd, &protocolBuffer,1024) < 0){
    sfwrite(&lock, stderr, "Read(): bytes read negative\n");
    //fprintf(stderr,"Read(): bytes read negative\n");
    return false;
  }
  if (verbose){
    sfwrite(&lock, stdout, VERBOSE "%s", protocolBuffer);
    sfwrite(&lock, stdout, DEFAULT "");
      //printf(VERBOSE "%s" DEFAULT, protocolBuffer);
    }
  //----------------------------------------------|| 
  //      CHECK IF EXPECTED: EIFLOW \r\n\r\n      ||
  //----------------------------------------------||
  if(checkVerb(PROTOCOL_EIFLOW, protocolBuffer)){
    if (!newUser)
      protocolMethod(fd, IAM, username, NULL,NULL, verbose, &lock);
    else
      protocolMethod(fd, IAMNEW, username, NULL, NULL, verbose, &lock); 
  }
  else{
    sfwrite(&lock, stderr, "Expected protocol verb EIFLOW\n");
    //printf("Expected protocol verb EIFLOW\n");
    return false;
  }
  //----------------------------------||
  //     READ RESPONSE FROM SERVER    ||                       
  //----------------------------------||

  memset(&protocolBuffer,0,1024);
  if (read(fd, &protocolBuffer,1024) < 0){
    sfwrite(&lock, stderr, "Read(): bytes read negative\n");
    //fprintf(stderr,"Read(): bytes read negative\n");
    return false;
  }
  //----------------------------------------------------|| 
  //      CHECK IF EXPECTED: ERR0 or ERR1 and BYE       ||
  //----------------------------------------------------|| 
  if (checkVerb(PROTOCOL_ERR0, protocolBuffer) || checkVerb(PROTOCOL_ERR1, protocolBuffer)){
    memset(&messageArray, 0, 1024);
    if ((noOfMessages = getMessages(messageArray, protocolBuffer))>1){
      if (verbose){
        sfwrite(&lock, stdout, ERROR "%s", messageArray[0]);
        sfwrite(&lock, stdout, DEFAULT "");
        //printf(ERROR "%s" DEFAULT, messageArray[0]);
      }
      if (checkVerb(PROTOCOL_BYE, messageArray[1])){
        if (verbose){
          sfwrite(&lock, stdout, ERROR "%s", messageArray[1]);
          sfwrite(&lock, stdout, DEFAULT "");
          //printf(VERBOSE "%s" DEFAULT, messageArray[1]);
        }
        if (checkVerb(PROTOCOL_ERR0, messageArray[0]))
          strncat(loginMSG, messageArray[0], 22);
        else
          strncat(loginMSG, messageArray[1], 25);
        return false;
      }
    }
    if(verbose){
      sfwrite(&lock, stdout, ERROR "%s", protocolBuffer);
      sfwrite(&lock, stdout, DEFAULT "");
      //  printf(ERROR "%s" DEFAULT, protocolBuffer);
    }
    if (checkVerb(PROTOCOL_ERR0, messageArray[0]))
          strncat(loginMSG, protocolBuffer, 22);
    else
          strncat(loginMSG, protocolBuffer, 25);
    memset(&protocolBuffer,0,1024);
    if (read(fd, &protocolBuffer,1024) < 0){
      sfwrite(&lock, stderr, "Read(): bytes read negative\n");
      //fprintf(stderr,"Read(): bytes read negative\n");
      return false;
    }
    if(verbose){
      sfwrite(&lock, stdout, VERBOSE "%s", protocolBuffer);
      sfwrite(&lock, stdout, DEFAULT "");
       // printf(VERBOSE "%s" DEFAULT, protocolBuffer);
    }
    if (checkVerb(PROTOCOL_BYE, protocolBuffer)){
      sfwrite(&lock, stderr, "Error sending username, received ERR and BYE\n");
      //  fprintf(stderr, "Error sending username, received ERR and BYE\n");
        return false;
    }
  }
  //---------------------------------------------|| 
  //      CHECK IF EXPECTED: HINEW or AUTH       ||
  //---------------------------------------------||
  if(verbose){
    sfwrite(&lock, stdout, VERBOSE "%s", protocolBuffer);
    sfwrite(&lock, stdout, DEFAULT "");
    //printf(VERBOSE "%s" DEFAULT, protocolBuffer);
  }
  if (protocol_Login_Helper(PROTOCOL_HINEW, protocolBuffer, username) || checkVerb(PROTOCOL_AUTH, protocolBuffer)){
    char password[1024];
    memset(&password,0,1024);
    sfwrite(&lock, stdout,"%s", "\npassword:");
    strcpy(password, getpass(""));


    //SEND MESSAGE DEPENDING ON WHETHER IS A NEW USER OR NOT
    if (!newUser){
      protocolMethod(fd, PASS, password, NULL, NULL, verbose, &lock);
    }
    else{
      protocolMethod(fd, NEWPASS, password, NULL, NULL, verbose, &lock);
    }
  }
  else {
    sfwrite(&lock, stderr, "Expected protocol verb AUTH or HINEW\n");
    //printf("Expected protocol verb AUTH or HINEW\n");
    return false;
  }
  //----------------------------------|| 
  //     READ RESPONSE FROM SERVER    ||                       
  //----------------------------------||
  memset(&protocolBuffer,0,1024);
  if (read(fd,&protocolBuffer,1024) < 0){
    sfwrite(&lock, stderr, "performLoginProcedure(): bytes read negative\n");
    //fprintf(stderr,"performLoginProcedure(): bytes read negative\n");
    return false;
  }
  //--------------------------------------------|| 
  //      CHECK IF EXPECTED: ERR2 and BYE       ||
  //--------------------------------------------||
  if (checkVerb(PROTOCOL_ERR2, protocolBuffer)){
    memset(&messageArray, 0, 1024);
    if ((noOfMessages = getMessages(messageArray, protocolBuffer))>1){
      if (verbose){
        sfwrite(&lock, stdout, ERROR "%s", messageArray[0]);
        sfwrite(&lock, stdout, DEFAULT "");
        //printf(ERROR "%s" DEFAULT, messageArray[0]);
      }
      if (checkVerb(PROTOCOL_BYE, messageArray[1])){
        if (verbose){
          sfwrite(&lock, stdout, VERBOSE "%s", messageArray[1]);
          sfwrite(&lock, stdout, DEFAULT "");
          //printf(VERBOSE "%s" DEFAULT, messageArray[1]);
        }
        sfwrite(&lock, stderr, "Error sending password, received ERR and BYE\n");
        //fprintf(stderr, "Error sending password, received ERR and BYE\n");
        strncat(loginMSG, messageArray[0], 19);
        return false;
      }
    }
    if (verbose){
      sfwrite(&lock, stdout, ERROR "%s", protocolBuffer);
      sfwrite(&lock, stdout, DEFAULT "");
      //printf(ERROR "%s" DEFAULT, protocolBuffer);
    }
    strcat(loginMSG, protocolBuffer);
    memset(&protocolBuffer,0,1024);
    if (read(fd, &protocolBuffer,1024) < 0){
      sfwrite(&lock, stderr, "Read(): bytes read negative\n");
      //fprintf(stderr,"Read(): bytes read negative\n");
      return false;
    }
    if (verbose){
      sfwrite(&lock, stdout, VERBOSE "%s", protocolBuffer);
      sfwrite(&lock, stdout, DEFAULT "");
      //printf(VERBOSE "%s" DEFAULT, protocolBuffer);
    }
    if (checkVerb(PROTOCOL_BYE, protocolBuffer)){
      sfwrite(&lock, stderr, "Error sending password, received ERR and BYE\n");
        //fprintf(stderr, "Error sending password, received ERR and BYE\n");
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
      if (verbose){
        sfwrite(&lock, stdout, VERBOSE "%s", messageArray[0]);
        sfwrite(&lock, stdout, DEFAULT "");
        printf(VERBOSE "%s" DEFAULT, messageArray[0]);
      }
      if (checkVerb(PROTOCOL_HI, messageArray[1])){
        if (verbose){
          sfwrite(&lock, stdout, VERBOSE "%s", messageArray[1]);
          sfwrite(&lock, stdout, DEFAULT "");
          //printf(VERBOSE "%s" DEFAULT, messageArray[1]);
        }
        if (noOfMessages == 3){
          if (checkVerb(PROTOCOL_MOTD, messageArray[2])){
            if (verbose){
              sfwrite(&lock, stdout, VERBOSE "%s", messageArray[2]);
              sfwrite(&lock, stdout, DEFAULT "");
              //printf(VERBOSE "%s" DEFAULT, messageArray[2]);
            }
            if (extractArgsAndTest(messageArray[2], motd)){
              sfwrite(&lock, stdout, "%s\n", motd);
              //printf("%s\n", motd);
              strcat(loginMSG, motd);
              return true;
            } 
          }
          else{
            sfwrite(&lock, stderr, "Didn't receive MOTD\n");
            //fprintf(stderr, "Didn't receive MOTD\n");
            return false;
          }
        }
        memset(&protocolBuffer, 0, 1024);
        read(fd,&protocolBuffer,1024);
        if (verbose){
          sfwrite(&lock, stdout, VERBOSE "%s", protocolBuffer);
          sfwrite(&lock, stdout, DEFAULT "");
          //printf(VERBOSE "%s" DEFAULT, protocolBuffer);
        }
        if (checkVerb(PROTOCOL_MOTD, protocolBuffer)){
            if (extractArgsAndTest(protocolBuffer, motd)){
              sfwrite(&lock, stdout, "%s\n", motd);
              //printf("%s\n", motd);
              strcat(loginMSG, motd);
              return true;
            }
        }
        else{
          sfwrite(&lock, stderr, "Didn't receive MOTD\n");
            //fprintf(stderr, "Didn't receive MOTD\n");
            return false;
        }
      }
      else{
        sfwrite(&lock, stderr, "Didn't receive HI\n");
        //fprintf(stderr, "Didn't receive HI\n");
      }
    }
    memset(&protocolBuffer, 0, 1024);
    if (read(fd, &protocolBuffer,1024) < 0){
      sfwrite(&lock, stderr, "Read(): bytes read negative\n");
      //fprintf(stderr,"Read(): bytes read negative\n");
      return false;
    }
    memset(&messageArray, 0, 1024);
    if ((noOfMessages = getMessages(messageArray, protocolBuffer))>1){
      if (verbose){
        sfwrite(&lock, stdout, VERBOSE "%s", messageArray[0]);
        sfwrite(&lock, stdout, DEFAULT "");
        //printf(VERBOSE "%s" DEFAULT, messageArray[0]);
      }
      if (checkVerb(PROTOCOL_HI, messageArray[0])){
        if (verbose){
          sfwrite(&lock, stdout, VERBOSE "%s", messageArray[0]);
          sfwrite(&lock, stdout, DEFAULT "");
          //printf(VERBOSE "%s" DEFAULT, messageArray[0]);
        }
        if (checkVerb(PROTOCOL_MOTD, messageArray[1])){
            if (verbose){
              sfwrite(&lock, stdout, VERBOSE "%s", messageArray[1]);
              sfwrite(&lock, stdout, DEFAULT "");
              //printf(VERBOSE "%s" DEFAULT, messageArray[1]);
            }
            if (extractArgsAndTest(messageArray[1], motd)){
              sfwrite(&lock, stdout, "%s\n", motd);
              //printf("%s\n", motd);
              strcat(loginMSG, motd);
              return true;
            }
          }
        }
      }
    if (protocol_Login_Helper(PROTOCOL_HI, protocolBuffer, username)){
      if (verbose){
          sfwrite(&lock, stdout, VERBOSE "%s", protocolBuffer);
          sfwrite(&lock, stdout, DEFAULT "");
          //printf(VERBOSE "%s" DEFAULT, protocolBuffer);
        }
      memset(&protocolBuffer, 0, 1024);
      if (read(fd, &protocolBuffer,1024) < 0){
        sfwrite(&lock, stderr,"Read(): bytes read negative\n");
        //fprintf(stderr,"Read(): bytes read negative\n");
        return false;
      }
      if (verbose){
          sfwrite(&lock, stdout, VERBOSE "%s", protocolBuffer);
          sfwrite(&lock, stdout, DEFAULT "");
          //printf(VERBOSE "%s" DEFAULT, protocolBuffer);
        }
      if (checkVerb(PROTOCOL_MOTD, protocolBuffer)){
          if (extractArgsAndTest(protocolBuffer, motd)){
              sfwrite(&lock, stdout, "%s\n", motd);
              //printf("%s\n", motd);
              strcat(loginMSG, motd);
              return true;
          }
        else{
          sfwrite(&lock, stderr, "Expected verb MOTD");
          //fprintf(stderr, "Expected verb MOTD");
        }
      }
      else {
        sfwrite(&lock, stderr, "Expected MOTD");
        //printf("Expected MOTD");
      }
    }
    else{
      sfwrite(&lock, stderr, "protocol_Login_Helper failed\n");
      //printf("protocol_Login_Helper failed\n");
    }
  }
  else {
    sfwrite(&lock, stderr, "Expected SSAP or SSAPWEN\n");
    //printf("Expected SSAP or SSAPWEN\n");
  }
  return false; 
}


void writeToGlobalSocket(){
  if(globalSocket>0){
      write(globalSocket," ",1);
    }
}

void recognizeAndExecuteStdin(char* userTypedIn){
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

FILE *initAudit(char *file){
  char *fileName = "audit.log";
  FILE *filePtr; 
  if (file != NULL){
    fileName = file;
  }
  filePtr = fopen(fileName, "a+");
  return filePtr;
}

void lockWriteUnlock(char* text,  int fd){
  //flock(fd, LOCK_EX);
  write(fd, text, strlen(text));
  //flock(fd, LOCK_UN);
}

void processAudit(int fd){
  char buf[1];
  memset(&buf, 0, 1);
  lseek(fd, 0, SEEK_SET);
  flock(fd, LOCK_EX);
  while (read(fd, buf, 1) == 1){
    sfwrite(&lock, stdout, buf);
  }
  flock(fd, LOCK_UN);
}




                  