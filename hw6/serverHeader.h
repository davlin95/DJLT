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
#include <openssl/rand.h>
#include <openssl/sha.h>
#include "WolfieProtocolVerbs.h"
#include <pthread.h> 
#include <sqlite3.h>
#include <semaphore.h> 

						/***********************************************************************/
						/*                    STRUCTS AND GLOBAL VARIABLES                     */
						/**********************************************************************/

#define MAX_INPUT 1024
struct pollfd pollFds[1024];
int pollNum=0;
int verbose = 0;
int serverFd=-1;
int globalSocket;
pthread_t commThreadId=-1;

//MUTEXES
pthread_mutex_t loginQueueMutex = PTHREAD_MUTEX_INITIALIZER;
pthread_mutex_t seenlist_mutex = PTHREAD_MUTEX_INITIALIZER;
pthread_mutex_t loginProcedureMutex = PTHREAD_MUTEX_INITIALIZER;
pthread_mutex_t stdoutMutex = PTHREAD_MUTEX_INITIALIZER;
pthread_rwlock_t clientList_RWLock = PTHREAD_RWLOCK_INITIALIZER;
pthread_rwlock_t accountList_RWLock = PTHREAD_RWLOCK_INITIALIZER;
pthread_rwlock_t seenlist_lock = PTHREAD_RWLOCK_INITIALIZER;

int loginQueue[128];
int queueCount =0;
sem_t items_semaphore;
sem_t accept_semaphore;
sem_t mainpoll_semaphore;

typedef struct sessionData{
  char* ipAddressString;
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
  char *salt;
  struct accountData *next;
  struct accountData *prev;
}Account;

typedef struct seenListData{
  char userName[1024];
  struct seenListData *next;
  struct seenListData *prev;
}SeenList;
                      

                      /********************************************/
                      /*    CLIENT / ACCOUNT STRUCT METHODS       */
                      /*******************************************/
Client* clientHead;
Account* accountHead;
SeenList* seenListHead=NULL;

void setSeenList(char* userBuffer){
  //pthread_rwlock_wrlock(&seenlist_lock);
  SeenList* node = malloc(sizeof(SeenList));
  memset(node->userName,0,1024);
  strcpy(node->userName,userBuffer);
  node->next = seenListHead;
  node->prev = NULL;
  seenListHead = node;
  //pthread_rwlock_unlock(&seenlist_lock);
}

bool inSeenList(char* userBuffer){
  pthread_mutex_lock(&seenlist_mutex);
  //pthread_rwlock_wrlock(&seenlist_lock);
  SeenList* ptr= seenListHead;
  while(ptr!=NULL){
    if(strcmp(userBuffer, ptr->userName)==0){
      SeenList* node = malloc(sizeof(SeenList));
      memset(node->userName,0,1024);
      strcpy(node->userName,userBuffer);
      node->next = seenListHead;
      node->prev = NULL;
      seenListHead = node;
      pthread_mutex_unlock(&seenlist_mutex);
      return true;
    }
  }
  //pthread_rwlock_unlock(&seenlist_lock);
  pthread_mutex_unlock(&seenlist_mutex);
  return false;
}


void destroySeenList(){
  pthread_rwlock_wrlock(&seenlist_lock);
  SeenList* head = seenListHead;
  SeenList* temp;
  while(head!=NULL){
    temp = head;
    head = head->next;
    free(temp);
  }
  pthread_rwlock_unlock(&seenlist_lock);
}

Client *createClient(char *username, int fd){
  Client *client = malloc(sizeof(struct clientData));
  Session *clientSession = malloc(sizeof(struct sessionData));
  struct in_addr *clientIPAddress= malloc(sizeof(struct in_addr));
  client->userName = malloc( (strlen(username)+1)*sizeof(char) );
  strcpy(client->userName, username);
  client->session = clientSession; 
  client->session->ipAddress = clientIPAddress;
  client->session->commSocket = fd;
  return client;
}

void destroyClientMemory(Client* user){
  if(user != NULL){
    //UNLINK THE CLIENT FROM THE REST OF LIST
    if(user->prev !=NULL){
      user->prev->next = user->next;
    }
    if(user->next !=NULL){
      user->next->prev = user->prev;
    }
    //RESET THE CLIENT HEAD IF MUST 
    if(user == clientHead && user->prev != NULL){
      clientHead = user->prev;
    }else if(user == clientHead && user->next != NULL){
      clientHead = user->next;
    }else if(user== clientHead){
      clientHead = NULL;
    }

    //ACTUAL FREEING PROCESS
    free(user->session->ipAddress);
    free(user->session->ipAddressString);
    free(user->session);
    free(user);
  }
}
/*
void setClientUserName(Client* client,char* name){
  client->userName = malloc( (strlen(name)+1)*sizeof(char) );
  strcpy(client->userName,name);
}
*/
void addClientToList(Client* client){
  pthread_rwlock_wrlock(&clientList_RWLock);
  client->next=clientHead;
  client->prev=NULL;
  clientHead=client;
  pthread_rwlock_unlock(&clientList_RWLock);
}


Account *createAccount(char *username, char* password, char* salt){
  Account *account = malloc(sizeof(struct accountData));
  account->userName = malloc((strlen(username)+1)*sizeof(char));
  strcpy(account->userName, username);
  account->password = malloc((strlen(password)+1)*sizeof(char));
  strcpy(account->password, password);
  account->salt = malloc((strlen(salt)+1)*sizeof(char));
  strcpy(account->salt, salt);
  return account;
}

void addAccountToList(Account* account){
  pthread_rwlock_wrlock(&accountList_RWLock);
  account->next=accountHead;
  account->prev=NULL;
  accountHead=account;
  pthread_rwlock_unlock(&accountList_RWLock);
}


void loadAccountFile();



						/***********************************************************************/
						/*                    SERVER PROGRAM FUNCTIONS                         */
						/**********************************************************************/
char* serverHelpMenuStrings[]={"/users \t List users currently logged in.", "/help \t List available commands.", "/accts \t List all user accounts and information.", "/shutdown \t Shutdown the server", NULL};

#define USAGE(name) do {                                                                         \
        sfwrite(&stdoutMutex,stderr,                                                                                \
            "\n%s [-hv] [-t THREAD_COUNT] PORT_NUMBER MOTD [ACCOUNTS_FILE]\n"                          \
            "-t THREAD_COUNT  The number of threads used for the login queue."                          \
            "-h               Displays help menu & returns EXIT_SUCCESS.\n"                              \
            "-v               Verbose print all incoming and outgoing protocol verbs & content.\n"       \
            "PORT_NUMBER      Port number to listen on.\n"                                               \
            "MOTD             Message to display to the client when they connect.\n"                     \
            "ACCOUNTS_FILE    File containing username and password data to be loaded upon execution.\n" \
            ,(name)                                                                                    \
        );                                                                                             \
    } while(0) 


            /************ STDIN READING ***********/
void compactPollDescriptors(){
  int i,j;
  for (i=0; i<pollNum; i++) 
  {
    // IF ENCOUNTER A CLOSED FD
    if (pollFds[i].fd == -1)
    {
      // SHIFT ALL SUBSEQUENT ELEMENTS LEFT BY ONE SLOT
      for(j = i; j < pollNum; j++)
      {
        pollFds[j].fd = pollFds[j+1].fd;
      }
      pollNum--;
    }
  }
}

			     /******** THREAD PROGRAMMING ********/

/*
 * Spawns a login thread for the client
 */
void* loginThread(void* args);

void* communicationThread(void* args);

						/******** CLIENT MANAGEMENT FUNCTIONS ******/

/*
 * A function that verifies a vaid user 
 */
bool verifyUser(int clientFd);

/*
 * A function that verifies a userPassword
 */
bool verifyPassword(char* password);


 Client* getClientByUsername(char* user);
 void printStarHeadline(char * string,int optionalOrNegative);
/*
 * A function that disconnects users
 */
 void disconnectUser(char* user){
  Client* loggedOffClient = getClientByUsername(user);
  if(loggedOffClient!=NULL){
    //SEND BYE BACK TO USER
    protocolMethod((loggedOffClient->session->commSocket),BYE,NULL,NULL,NULL, verbose, &stdoutMutex); 

    //CLOSING USER SOCKET
    close(loggedOffClient->session->commSocket); 

    //MUST BE LAST STEP 
    destroyClientMemory(loggedOffClient);
  }
 }

/* 
 * A function that disconnects all connected users. 
 */
 void disconnectAllUsers(){
   Client* clientPtr = clientHead;
   Client* logginOffUser;
   while(clientPtr!=NULL){
     logginOffUser= clientPtr;
     clientPtr= clientPtr->next;
     disconnectUser(logginOffUser->userName);
   }
 }

/*
 * A function that prints the account struct into string
 * @param accountData: account struct to be printed
 * @return : void
 */
void accountStructToString(Account* accountData){
  sfwrite(&stdoutMutex,stdout,"Username: %-15s Password: %s\n", accountData->userName, accountData->password);
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
  /*OUTDATED CODE: SINCE NEW FUNCTIONALITY GETS THE STRING DIRECTLY 
  char ipaddress[33];
  memset(ipaddress, 0, strlen(ipaddress)); 
  inet_ntop( AF_INET,(clientData->session->ipAddress), ipaddress, 33); */

  sfwrite(&stdoutMutex,stdout,"Username: %-15s IP Address: %s\n", clientData->userName, 
    clientData->session->ipAddressString);
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
void createSocketPair(int socketsArray[], int size){
  if(size <2){
    sfwrite(&stdoutMutex,stderr,"CreateSocketPair(): insufficient size of array\n");
  }
  int status=-1;
  status = socketpair(AF_UNIX,SOCK_STREAM,0,socketsArray);
  if(status<0){
    sfwrite(&stdoutMutex,stderr,"CreateSocketPair(): error with socketpair()\n");
  }
}


 					/********* BUILTIN COMMANDS ********/
 /* 
  * A help menu function for server
  */ 
 void processHelp(){
    sfwrite(&stdoutMutex,stdout,                                                                             
            "[-h|-v] PORT_NUMBER MOTD [ACCOUNTS_FILE]\n"                                          
            "-h             Displays help menu & returns EXIT_SUCCESS.\n"                              
            "-v             Verbose print all incoming and outgoing protocol verbs & content.\n"       
            "PORT_NUMBER    Port number to listen on.\n"                                               
            "MOTD           Message to display to the client when they connect.\n"                     
            "ACCOUNTS_FILE  File containing username and password data to be loaded upon execution.\n" 
                                                                                              
    ); 
 }

 /* 
  * A function that returns the connected users on the server
  */
void processUsers();
  
  /* 
  * A function that shuts down the server
  */ 
 void processShutdown(){
   //sfwrite(&stdoutMutex,stdout,"SHUTTING DOWN()\n");
   disconnectAllUsers();
   if(serverFd>0){
     close(serverFd);
   }
   destroySeenList();
   // printf("\nEXITING\n");
   exit(0);
 }

void killServerHandler(){
    disconnectAllUsers();
    sfwrite(&stdoutMutex,stdout,"disconnected All users\n"); 
    exit(EXIT_SUCCESS);
}


 void processAccounts();

						/****** SERVING CLIENT METHODS **/
  /*  
   * A function that clears the message of the day
   */
  void clearMessageOfTheDay(){
    memset(messageOfTheDay,0,1024);
  }

  void sendMessage(int fd, char *message){
    write(fd,message,strlen(message)); 
  }

 /*
  * A function that tells the client the message of the day 
  */
  void sendMessageOfTheDay(int clientFd){
    sendMessage(clientFd, messageOfTheDay);
  }

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

 int sessionLength(Client* client, char* sessionLengthBuffer){
    if (sessionLengthBuffer == NULL)
      return -1;
    int length;
    time_t currentTime;
    currentTime = time(0);
    length = ((int)(currentTime - client->session->start));
    sprintf(sessionLengthBuffer, "%d", length);
    return length;
 }


/*
 * Finds the client struct associated with the clientID 
 * @param clientID: ID of the client whose info to be searched for 
 * @return: clientData struct
 */
 Client* getClientByUsername(char* username){
    Client* clientPtr;
    for(clientPtr = clientHead; clientPtr; clientPtr = clientPtr->next){
      if (strcmp(username, clientPtr->userName) == 0)
        return clientPtr;
      }
    return NULL;
  }

  Client* getClientByFd(int fd){
    Client* clientPtr;
    for(clientPtr = clientHead; clientPtr; clientPtr = clientPtr->next){
      if (clientPtr->session->commSocket == fd)
        return clientPtr;
    }
    return NULL;
  }

  Account* getAccountByUsername(char* username){
    Account* accountPtr;
    for(accountPtr = accountHead; accountPtr; accountPtr = accountPtr->next){
      if (strcmp(username, accountPtr->userName) == 0)
        return accountPtr;
    }
    return NULL;
  } 

 

 void printStarHeadline(char* headline,int optionalFd){
  printf("\n/***********************************/\n");
  printf("/*\t");
  if(optionalFd>=0){
    printf("%-30s : %d",headline,optionalFd);
  }
  else {
    printf("%-30s",headline);
  }
  printf("\t*/\n");
  printf("/***********************************/\n");

 }

  /*********** STDIN FUNCTIONS ***********/
void recognizeAndExecuteStdin(char* userTypedIn){
  // printf("user typed in: %s", userTypedIn);
  if(strcmp(userTypedIn,"/users\n")==0){
    //PRINT OUT USERS
    processUsersRequest();
  }else if(strcmp(userTypedIn,"/help\n")==0){
    //PRINT OUT HELP
    displayHelpMenu(serverHelpMenuStrings);
  }else if(strcmp(userTypedIn,"/accts\n")==0){
    processAcctsRequest();
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
    sfwrite(&stdoutMutex,stderr,"getaddrinfo():%s\n",gai_strerror(status));
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
      sfwrite(&stdoutMutex,stderr,"setsockopt(): %s\n",strerror(errno)); 
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
  if ((serverFd = socket(results->ai_family, results->ai_socktype, results->ai_protocol)) < 0){
    sfwrite(&stdoutMutex,stderr,"socket(): %s\n",strerror(errno));
    return -1;
  }
  if ((makeReusable(serverFd)) < 0){
    if (close(serverFd) < 0){
      sfwrite(&stdoutMutex,stderr,"close(): %s\n",strerror(errno));
      return -1;
    }
  }
  if (bind(serverFd, results->ai_addr, results->ai_addrlen)<0){
    if (close(serverFd) < 0){
      sfwrite(&stdoutMutex,stderr,"close(): %s\n",strerror(errno));
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
    sfwrite(&stdoutMutex,stderr,"fcntl(): %s\n",strerror(errno));
    return -1;
  }
  flags |= O_NONBLOCK;
  if ((fcntl(fd, F_SETFL, flags)) < 0){
    sfwrite(&stdoutMutex,stderr,"fcntl(): %s\n",strerror(errno));
    return -1;
  }
  return 0;
}

int makeBlocking(int fd){
  int flags = fcntl(fd, F_GETFL);
  fcntl(fd, F_SETFL, flags & ~O_NONBLOCK);
  return 1;
}
/*
 *A function that creates and binds the server socket, and sets it to listening
 *@param portNumber: portNumber to create socket on
 *@param serverFd: socket to be initialized
 *@return: serverFd, -1 otherwise
 */
int createBindListen(char* portNumber, int serverFd){
  struct addrinfo resultsStruct;
  struct addrinfo *results=&resultsStruct;
  if ((results = buildAddrInfoStructs(results, portNumber)) == NULL){
    return -1;
  }
  if ((serverFd = findSocketandBind(results, serverFd)) < 0){
    freeaddrinfo(results);
    return -1;
  }
  if ((makeNonBlocking(serverFd))<0){
    sfwrite(&stdoutMutex,stderr,"createBindListen(): error makeNonBlocking\n");
    freeaddrinfo(results);
    return -1;
  }
  if(listen(serverFd,128)<0){
    if(close(serverFd)<0){
      sfwrite(&stdoutMutex,stderr,"close(serverFd): %s\n",strerror(errno));
    }
    sfwrite(&stdoutMutex,stderr,"listen(): %s\n",strerror(errno));
    freeaddrinfo(results);
    return -1;
  }
  freeaddrinfo(results);
  return serverFd;
}

/*
 * Takes in a socket fd, and a char buffer, returns the ip address associated with the socket.
 */
char* getIpAddressFromSocketFd(int socketFd,char ipAddress[]){
  int status=0;
  struct sockaddr address;
  socklen_t addressLength=sizeof(address);
  status = getsockname(socketFd,&address,&addressLength);
  if(status<0){
    sfwrite(&stdoutMutex,stderr,"getIpAddressFromSocketFd():%s",strerror(errno));
  }
  memset(ipAddress,0,1024);
  struct sockaddr_in * ipInet = ((struct sockaddr_in *) &address);
  char* ipString = inet_ntoa(ipInet->sin_addr );
  strcpy(ipAddress,ipString);
  return ipAddress;
}


            /***********************************************************************/
            /*                    BUILD LIST OF CLIENTS FOR UTSIL                        */
            /**********************************************************************/

bool buildUtsilArg(char *argBuffer){
  // CHECK FOR ERRORS
    if(argBuffer==NULL) {
      sfwrite(&stdoutMutex,stderr,"buildUtsilArg(): argBuffer is null\n");
      return 0;
    } 

  // LOOP THRU USERS
    char clientUsername[1024];
    Client* clientPtr;
    for(clientPtr = clientHead; clientPtr; clientPtr = clientPtr->next){
        memset(&clientUsername, 0, 1024);
        strcpy(clientUsername, clientPtr->userName);

        //SHOULD WE 
        if (clientPtr->next != NULL)
            strcat(clientUsername, "\r\n");
        strcat(argBuffer, clientUsername);
    }
    sfwrite(&stdoutMutex,stdout,"arg is %s\n", argBuffer);
    return true;
}

void sha256(char* password, unsigned char *output){
  SHA256_CTX sha256;
  SHA256_Init(&sha256);
  SHA256_Update(&sha256, password, strlen(password));
  SHA256_Final(output, &sha256);
}

void processValidClient(char* clientUserName, int fd){
  Client* newClient = createClient(clientUserName, fd);

  //GET IPADDRESS STRING FOR THE CLIENT STRUCT
  char ipAddressBuffer[1024];
  memset(ipAddressBuffer,0,1024);
  char* ipString;
  ipString = getIpAddressFromSocketFd(fd,ipAddressBuffer); // result is ipString

  //ADD IP ADDRESS TO THE CLIENT'S SESSION;
  char* mallocIpString = malloc((strlen(ipString)+1)*sizeof(char));
  strcpy(mallocIpString,ipString);
  newClient->session->ipAddressString = mallocIpString;

  //START SESSION TIME FOR CLIENT
  startSession(newClient->session);
  addClientToList(newClient);
}

void processValidAccount(char *username, char *password, char *salt){
  Account* newAccount = createAccount(username, password, salt);
  addAccountToList(newAccount);
}

int callback(void* NotUsed, int argc, char **argv, char**azColName){
  int i;
  for (i = 0; i < argc; i+=3){
    processValidAccount( argv[i], argv[i+1], argv[i+2]);
  }
  return 0;
}
char * createSQLInsert(Account *account, char* sqlBuffer){
  char* sqlPtr = sqlBuffer;
  strcat(sqlPtr, "INSERT INTO ACCOUNTS (username, password, salt) VALUES ('");
  strcat(sqlPtr, account->userName);
  strcat(sqlPtr, "', '");
  strcat(sqlPtr, account->password);
  strcat(sqlPtr, "', '");
  strcat(sqlPtr, account->salt);
  strcat(sqlPtr, "');");
  return sqlPtr;
}

void notifyAllUsersUOFF(char* username){
  Client* clientPtr;
  for (clientPtr = clientHead; clientPtr!=NULL; clientPtr = clientPtr->next){
    protocolMethod(getClientByUsername(clientPtr->userName)->session->commSocket, UOFF, username, NULL, NULL, verbose, &stdoutMutex);
  }
}

void writeToGlobalSocket(){
  if(globalSocket>0){
      write(globalSocket," ",1);
      // printf("\nwrote to globalSocket\n");
    }
}

void getAccounts(char * accounts){
      FILE *accountsFile;
      size_t size = 1024;
      char username[1024];
      char password[1024];
      char salt[1024];
      char *usernamePtr = username;
      char *passwordPtr = password;
      char *saltPtr = salt;
      char *lastChar;
      memset(&username, 0, 1024);
      memset(&password, 0, 1024);
      memset(&salt, 0, 1024);
      accountsFile = fopen(accounts, "r");
      if (accountsFile != NULL){
        while(getline(&usernamePtr, &size, accountsFile) > 0){
          lastChar = strchr(usernamePtr, '\n');
          *lastChar = '\0';
          if (getline(&passwordPtr, &size, accountsFile) > 0){
            lastChar = strchr(passwordPtr, '\n');
            *lastChar = '\0';
          } else{
            sfwrite(&stdoutMutex,stderr, "Invalid Accounts File\n");
            exit(0);
          }

          if (getline(&saltPtr, &size, accountsFile) > 0){
            lastChar = strchr(saltPtr, '\n');
            *lastChar = '\0';
          } else{
            sfwrite(&stdoutMutex,stderr, "Invalid Accounts File\n");
            exit(0);
          }
          processValidAccount( usernamePtr, passwordPtr, saltPtr);
          memset(&username, 0, 1024);
          memset(&password, 0, 1024);
          memset(&salt, 0, 1024);
        }
        fclose(accountsFile);
      }
  }

  void saveAccounts(char * accounts){
    FILE *accountsFile;
    Account *accountPtr;
    accountsFile = fopen(accounts, "w");
    if (accountsFile != NULL){
        for (accountPtr = accountHead; accountPtr!=NULL; accountPtr = accountPtr->next){
            sfwrite(&stdoutMutex,accountsFile, "%s\n", accountPtr->userName);
            sfwrite(&stdoutMutex,accountsFile, "%s\n", accountPtr->password);
            sfwrite(&stdoutMutex,accountsFile, "%s\n", accountPtr->salt);
        }
      fclose(accountsFile);
    }
  }