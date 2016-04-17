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

#define USAGEserver(name) do {                                                                         \
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

#define USAGEclient(name) do {                                                                  \
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

#define USAGEchat(name) do {                                                                    \
        fprintf(stderr,                                                                         \
            "\n%s UNIX_SOCKET_FD\n"                                                             \
            "UNIX_SOCKET_FD       The Unix Domain File Descriptor number.\n"                  \
            ,(name)                                                                             \
        );                                                                                      \
    } while(0)



char* serverHelpMenuStrings[]={"/users \t List users currently logged in.", "/help \t List available commands.", "/accts \t List all user accounts and information.", "/shutdown \t Shutdown the server", NULL};
char* clientHelpMenuStrings[]={"/help \t List available commands.", "/listu \t List connected users.", "/time \t Display connection duration with Server.", "/chat <to> <msg> \t Send message (<msg>) to user (<to>).", "/logout \t Disconnect from server.", NULL};

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





						/***********************************************************************/
						/*                    CLIENT PROGRAM FUNCTIONS                         */
						/**********************************************************************/

 /*
 * A client side function that convert's the server's returned connection time into user-friendly printout format. 
 * @param time: time conneted on the server
 * @return: void
 */
  void displayClientConnectedTime(int time){
    int hour;
    int minute;
    int second;
    hour = time/3600;
    minute = (time%3600)/60;
    second = (time%3600)%60;
    printf("connected for %d hour(s), %d minute(s), and %d second(s)\n", hour, minute, second);
  }

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
						/*                    CHAT PROGRAM FUNCTIONS                         */
						/**********************************************************************/


void spawnChatWindow();

void openChatWindow();

void closeChatWindow();

void executeXterm();

void processGeometry(int width, int height);



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