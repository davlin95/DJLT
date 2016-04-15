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

#define _GNU_SOURCE

						/***********************************************************************/
						/*                    STRUCTS AND GLOBAL VARIABLES                     */
						/**********************************************************************/
typedef struct clientData{
	char* userName;
  Session* session;
}Client;

typedef struct sessionData{
  clock_t start;
  clock_t end;
}Session;

typedef struct accountData{
	char* userName;
	char* password;
}Account;

client* clientList;
Account* accountList;

char* serverHelpMenuStrings[]={};
char* clientHelpMenuStrings[]={"logout"};

						/***********************************************************************/
						/*                    SERVER PROGRAM FUNCTIONS                         */
						/**********************************************************************/

						/********** ACCOUNT FUNCTIONS *******/
void loadAccountFile();

AccountData* getAccount(int accountId);

void setAccount(AccountData* newAccount);

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
 * A function that prints the client struct into string
 * @param clientData: client struct to be printed
 * @return : void
 */
void clientStructToString(clientData* clientData);


						
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
 void processHelp();

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
 clientData* returnClientData(int clientID);


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
 void startSession(Session* session);
 void endSession(Session* session);
 clock_t sessionLength(Session* session);

/*
 * Finds the client struct associated with the clientID 
 * @param clientID: ID of the client whose info to be searched for 
 * @return: clientData struct
 */
 clientData* returnClientData(int clientID);



 						/******* ACCESSORY METHODS ******/





						/***********************************************************************/
						/*                    CLIENT PROGRAM FUNCTIONS                         */
						/**********************************************************************/

 /*
 * A client side function that convert's the server's returned connection time into user-friendly printout format. 
 * @param time: time conneted on the server
 * @return: void
 */
  void displayClientConnectedTime(unsigned long long time);

 /* 
  * A help menu function for client
  */ 
 void displayHelpMenu();



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
bool atLeastFiveCharacters(char * password);
bool atLeastOneUpperCaseChar(char* password);
bool atLeastOneSymbol(char * password);
bool atLeastOneNumber(char* password);
bool validPassword(char* password);
bool storePassword(char* password);