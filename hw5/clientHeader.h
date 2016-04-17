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

#define USAGE(name) do {                                                                  \
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
 *A function that creates a socket and connects it to the server
 *@param portNumber: portNumber of the Server
 *@param clientFd: socket to be initialized
 *@return: clientFD, -1 otherwise
 */
int createAndConnect(char* portNumber, int clientFd){
	bool error = false;
  	struct addrinfo *results, *resultsPtr;
  	if ((results = buildAddrInfoStructs(results, portNumber)) == NULL)
    	error = true;
  	for (resultsPtr = results; resultsPtr; resultsPtr->ai_next){    	if ((clientFd = socket(resultsPtr->ai_family, resultsPtr->ai_socktype, resultsPtr->ai_protocol)) < 0)
      		continue;
    	if (connect(clientFd, resultsPtr->ai_addr, resultsPtr->ai_addrlen)!=-1)
      		break;
    	if (close(clientFd) < 0){
      		fprintf(stderr,"close(): %s\n",strerror(errno));
      		error = true;
    	}
  	}
  	freeaddrinfo(results);
  	if (error)
  		return -1;
  	return clientFd;
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




