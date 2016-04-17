						/***********************************************************************/
						/*                    HEADER DETAILS                                  */
						/**********************************************************************/

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <stdbool.h>
#include <fcntl.h>
#include <netinet/in.h>
#include <pthread.h>
#include <time.h>
#include <arpa/inet.h>
#include <sys/fcntl.h>

#define _GNU_SOURCE

						/***********************************************************************/
						/*                    STRUCTS AND GLOBAL VARIABLES                     */
						/**********************************************************************/

#define MAX_INPUT 1024

typedef struct sessionData{
  struct in_addr *ipAddress;
  int commSocket;
  time_t start;
  time_t end;
}Session;

typedef struct clientData{
	char *userName;
  Session* session;
  struct clientData *next;
  struct clientData *prev;
}Client;

typedef struct accountData{
	char *userName;
	char *password;
  struct accountData *next;
  struct accountData *prev;
}Account;

Client* clientHead;
Account* accountHead;

Client *createClient(){
  Client *client = malloc(sizeof(struct clientData));
  Session *clientSession = malloc(sizeof(struct sessionData));
  struct in_addr *clientIPAddress= malloc(sizeof(struct in_addr));
  client->session = clientSession;
  ((Session *)client->session)->ipAddress = clientIPAddress;
  return client;
}

Account *createAccount(){
  Account *account = malloc(sizeof(struct accountData));
  return account;
}

#define USAGE(name) do {                                                                         \
        fprintf(stderr,                                                                                \
            "\n%s [-h|-v] PORT_NUMBER MOTD [ACCOUNTS_FILE]\n"                                          \
            "-h             Displays help menu & returns EXIT_SUCCESS.\n"                              \
            "-v             Verbose print all incoming and outgoing protocol verbs & content.\n"       \
            "PORT_NUMBER    Port number to listen on.\n"                                               \
            "MOTD           Message to display to the client when they connect.\n"                     \
            "ACCOUNTS_FILE  File containing username and password data to be loaded upon execution.\n" \
            ,(name)                                                                                    \
        );                                                                                             \
    } while(0)






char* serverHelpMenuStrings[]={"/users \t List users currently logged in.", "/help \t List available commands.", "/accts \t List all user accounts and information.", "/shutdown \t Shutdown the server", NULL};


						/***********************************************************************/
						/*                    SERVER PROGRAM FUNCTIONS                         */
						/**********************************************************************/

						/********** ACCOUNT FUNCTIONS *******/
void loadAccountFile();

Account* getAccount(int accountId);

void setAccount(Account* newAccount);

            /************ STDIN READING ***********/





						/******** THREAD PROGRAMMING ********/

/*
 * Spawns a communication thread for the client
 */
void spawnAcceptThread();

/*
 * Spawns a login thread for the client
 */
void spawnLoginThread();

/*
 * Spawns a communication thread for the client
 */
void spawnCommunicationThread();


						/******** CLIENT MANAGEMENT FUNCTIONS ******/

/*
 * A function that verifies a vaid user 
 */
bool verifyUser(int clientFd);

/*
 * A function that verifies a userPassword
 */
bool verifyPassword(char* password);

/*
 * A function that adds a user to the maintained list of active users 
 */
void addUser(int clientFd);

/*
 * A function that disconnects users
 */
 void disconnectUser(int clientFd);

/*
 * A function that disconnects all connected users. 
 */
 void disconnectAllUsers();
/*
 * A function that prints the account struct into string
 * @param accountData: account struct to be printed
 * @return : void
 */
void accountStructToString(Account* accountData){
  printf("Username: %-15s Password: %s\n", accountData->userName, accountData->password);
}
/*
 *A function that prints all accounts
 *@return: void
 */
void processAcctsRequest(){
  Account *accountPtr = accountHead;
  while (accountPtr != NULL){
    accountStructToString(accountPtr);
    accountPtr = accountPtr->next;
  }
}
/*
 * A function that prints the client struct into string
 * @param clientData: client struct to be printed
 * @return : void
 */
void clientStructToString(Client* clientData){
  char ipaddress[33];
  memset(ipaddress, 0, strlen(ipaddress));
  inet_ntop( AF_INET,(clientData->session->ipAddress), ipaddress, 33);
  printf("Username: %-15s IP Address: %s\n", clientData->userName, 
    ipaddress);
}
/*
 *A function that prints all connected users
 *@return: void
 */
void processUsersRequest(){
  Client *clientPtr = clientHead;
  while (clientPtr != NULL){
    clientStructToString(clientPtr);
    clientPtr = clientPtr->next;
  }
}


						
						/******** PROGRAM I/O AND LOGISTIC FUNCTIONS ****/

/* 
 * A function that creates a socketFd for the port currently listened on. 
 */
 int createSocket(int portNumber);

/* 
 * A method that handles detection of control-C keyboard input 
 */
 void controlC();

/*
 * A function that closes the connection socket
 */
 void closeSocket();

/*
 * A function that closes all the available socket connections
 */
 void closeAllSockets();

 					/*********** STDIN FUNCTIONS ***********/
 /* 
  * A function that parses the commandline 
  */
 void parseCommandLine();
 
 /*
  * A function that builds an argument array 
  */
 void buildArguments();

  /*
  * A function that builds executes the arguments
  */
 void executeArguments();


 					/********* BUILTIN COMMANDS ********/
 /* 
  * A function that returns the connected users on the server
  */
void processUsers();

  /* 
  * A help menu function for server
  */ 
 void processHelp(){
 }

  /* 
  * A function that shuts down the server
  */ 
 void processShutdown();

 void processAccounts();

						/****** SERVING CLIENT METHODS **/
/*
 * Finds the client struct associated with the clientID 
 * @param clientID: ID of the client whose info to be searched for 
 * @return: clientData struct
 */
 Client* returnClientData(int clientID);


 /*
  * A function that tells the client the message of the day 
  */
  void sendMessageOfTheDay(int clientFd);

  /*
   * A function that rejects the client from logging in 
   */
  void rejectClient(int clientFd);

  /*
   *  A function that services the TIME Verb between server and client. 
   */
  int findElapsedTimeDifference(int startingTime, int endingTime);



 						/***** USER INFO METHODS *******/
/*
 * A server side function that returns the client's connection time. 
 * @param clientID: ID of the client whose info to be searched for 
 * @return: value in seconds client was connected for.
 */
 unsigned long long returnClientConnectedtime(int clientID);

 void startSession(Session* session){
    session->start = time(0);
 }

 void endSession(Session* session){
    session->end = time(0);
 }

 int sessionLength(Session* session){
    time_t currentTime;
    currentTime = time(0);
    return ((int)(currentTime - session->start));
 }


/*
 * Finds the client struct associated with the clientID 
 * @param clientID: ID of the client whose info to be searched for 
 * @return: clientData struct
 */
 Client* returnClientData(int clientID);



 						/******* ACCESSORY METHODS ******/
 /* 
  * A help menu function for client
  */ 
 void displayHelpMenu(char **helpMenuStrings){
    char** str = helpMenuStrings;
    while(*str!=NULL){
      printf("%-30s\n",*str); 
      str++;
    }
 }


					/***** USER SECURITY **********/

					/******* VALID PASSWORD *****/
bool atLeastFiveCharacters(char * password){
  char* charPtr = password;
  int count = 0;
  while (*charPtr != '\0'){
    charPtr++;
    count++;
  }
  return (count > 4);
}

bool atLeastOneUpperCaseChar(char* password){
  bool oneUpper = false;
  char* charPtr = password;
  while (*charPtr != '\0'){
    if (*charPtr > 64 && *charPtr < 90)
      oneUpper = true;
    charPtr++;
  }
  return oneUpper;
}

bool atLeastOneSymbol(char * password){
  bool oneSymbol = false;
  char* charPtr = password;
  while (*charPtr != '\0'){
    if ((*charPtr > 32 && *charPtr < 48) || (*charPtr > 57 && *charPtr < 65) || (*charPtr > 90 && *charPtr < 97))
      oneSymbol = true;
    charPtr++;
  }
  return oneSymbol;
}
bool atLeastOneNumber(char* password){
  bool oneNumber = false;
  char* charPtr = password;
  while (*charPtr != '\0'){
    if (*charPtr > 47 && *charPtr < 58)
      oneNumber = true;
    charPtr++;
  }
  return oneNumber;
}
bool validPassword(char* password){
  return (atLeastFiveCharacters(password) && atLeastOneUpperCaseChar(password) && atLeastOneSymbol(password) && atLeastOneNumber(password));
}

bool storePassword(char* password);


            /***********************************************************************/
            /*                    CREATE SOCKET FUNCTIONS                         */
            /**********************************************************************/

/*
 *A function that creates necessary addrinfo structs and initializes the addrinfo list
 *@param results: addrinfo list to be initialized
 *@param portNumber: portNumber used for getaddrinfo method
 *@return: initialized addrinfo list results
 */
struct addrinfo * buildAddrInfoStructs(struct addrinfo *results, char* portNumber){
  int status;
  struct addrinfo settings;
  memset(&settings,0,sizeof(settings));
  settings.ai_family=AF_INET;
  settings.ai_socktype=SOCK_STREAM;
  status = getaddrinfo(NULL,portNumber,&settings,&results);
  if(status!=0){
    fprintf(stderr,"getaddrinfo():%s\n",gai_strerror(status));
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
      fprintf(stderr,"setsockopt(): %s\n",strerror(errno)); 
      return -1;
  }
  return 0;
}
/*
 *A function that walks through the results list and finds a socket to bind to
 *@param results: addrinfo list
 *@param serverFd: socket to be initialized
 *@return: serverFD, -1 otherwise
 */
int findSocketandBind(struct addrinfo *results, int serverFd){
  struct addrinfo *resultsPtr;
  for (resultsPtr = results; resultsPtr; resultsPtr->ai_next){
    if ((serverFd = socket(resultsPtr->ai_family, resultsPtr->ai_socktype, resultsPtr->ai_protocol)) < 0)
      continue;
    if ((makeReusable(serverFd)) < 0){
      close(serverFd);
      return -1;
    }
    if (bind(serverFd, resultsPtr->ai_addr, resultsPtr->ai_addrlen)==0)
      break;
    if (close(serverFd) < 0){
      fprintf(stderr,"close(): %s\n",strerror(errno));
      return -1;
    }
  }
  return serverFd;
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
/*
 *A function that creates and binds the server socket, and sets it to listening
 *@param portNumber: portNumber to create socket on
 *@param serverFd: socket to be initialized
 *@return: serverFd, -1 otherwise
 */
int createBindListen(char* portNumber, int serverFd){
  bool error = false;
  struct addrinfo *results;
  if ((results = buildAddrInfoStructs(results, portNumber)) == NULL)
    error = true;
  if ((serverFd = findSocketandBind(results, serverFd)) < 0)
    error = true;
  if ((makeNonBlocking(serverFd))<0)
    error = true;
  if(listen(serverFd,1024)<0){
    if(close(serverFd)<0){
      fprintf(stderr,"close(serverFd): %s\n",strerror(errno));
    }
    fprintf(stderr,"listen(): %s\n",strerror(errno));
    error = true;
  }
  freeaddrinfo(results);
  if (error)
    return -1;
  return serverFd;
}