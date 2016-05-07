#include "clientHeader.h"
		

				/****************************************/
				/* Accessory method needed at this spot */
				/****************************************/
/*
 *A function that makes the socket non-blocking
 *@param fd: file descriptor of the socket
 *@return: 0 if success, -1 otherwise
 */  
int makeNonBlockingForChat(int fd){
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
  return 1;
}

					/************************/
					/*  SIGNALS            */
					/**********************/
/*
 * Send SIGINT to process 
 */
void killXterm(pid_t xtermProcess){
	if(xtermProcess>=0){
	  kill(xtermProcess,SIGINT);
	}
}

					/*************************/
					/*     GLOBALS CHAT     */
					/***********************/

typedef struct xtermStruct{
  //NODE DATA
  int chatFd;
  pid_t xtermProcess;
  char* toUser;

  //LINKED LIST FEATURES
  struct xtermStruct *next;
  struct xtermStruct *prev;
}Xterm;

//HEAD OF LINKED LIST STRUCT PTR
Xterm* xtermHead;

		
						/***********************/
                        /* XTERM STRUCT METHODS */
						/***********************/
Xterm* createXtermStruct(char* username,int fd, pid_t process){
  Xterm* newXterm = malloc(sizeof(Xterm));

  //GET MALLOCED USERNAME STRING
  char* newUser = malloc((strlen(username)+1)*sizeof(char));
  strcpy(newUser,username);

  //MAKE THE XTERM CHAT FD NONBLOCKING
  if (makeNonBlockingForChat(fd)<0){ 
    sfwrite(&lock, stderr, "createXtermStruct(): Error making fd nonblocking\n");
    //fprintf(stderr,"createXtermStruct(): Error making fd nonblocking\n");
  }

  //ADD VALUES TO STRUCT
  newXterm->toUser = newUser;
  newXterm->chatFd = fd;
  newXterm->xtermProcess = process;

  newXterm->next = NULL;
  newXterm->prev = NULL;

  return newXterm;
}

void destroyXtermMemory(Xterm* xterm){
  if(xterm != NULL){
    //UNLINK THE XTERM FROM THE REST OF LIST
    if(xterm->prev !=NULL){
      xterm->prev->next = xterm->next;
    }
    if(xterm->next !=NULL){
      xterm->next->prev = xterm->prev;
    }
    //RESET THE XTERM HEAD IF MUST 
    if(xterm == xtermHead && xterm->prev != NULL){
      xtermHead = xterm->prev;
    }else if(xterm == xtermHead && xterm->next != NULL){
      xtermHead = xterm->next;
    }else if(xterm== xtermHead){
      xtermHead = NULL;
    }

    //ACTUAL FREEING PROCESS
    free(xterm->toUser);
    free(xterm);
  }
}

	
 					/************************************************/
                    /*           XTERM CHAT PROGRAM                */
                    /***********************************************/

void setChatUser(char* username, int fd, pid_t xtermProcess){
  //Create new xterm
  Xterm* newXterm = createXtermStruct(username,fd,xtermProcess);

  //ADD TO GLOBAL DATA STRUCTURES
  if(xtermHead!=NULL){
  	xtermHead->prev=newXterm;
  }
  newXterm->next = xtermHead;
  xtermHead = newXterm;
  //sfwrite(&lock, stderr, "newXterm added in list: local var xtermProcess is %d struct val is %d\n",xtermProcess,xtermHead->xtermProcess);
  //printf("newXterm added in list: local var xtermProcess is %d struct val is %d\n",xtermProcess,xtermHead->xtermProcess);
  //ADD TO GLOBAL POLL STRUCT THAT CORRESPONDS TO THE CHATFD GLOBALS
  clientPollFds[clientPollNum].fd = fd;
  clientPollFds[clientPollNum].events = POLLIN;
  clientPollNum++;
  //sfwrite(&lock, stderr, "setChatUser(): fd just set is : %d clientPollNum for it is %d\n",clientPollFds[clientPollNum-1].fd, clientPollNum-1);
  //printf("setChatUser(): fd just set is : %d clientPollNum for it is %d\n",clientPollFds[clientPollNum-1].fd, clientPollNum-1);
}

Xterm* getXtermByUsername(char* username){
	Xterm* xtermPtr;
    for(xtermPtr = xtermHead; xtermPtr!=NULL; xtermPtr = xtermPtr->next){
      if (strcmp(xtermPtr->toUser,username) == 0){
        return xtermPtr;
      }
    }
    return NULL;
}

Xterm* getXtermByPid(pid_t pid){
	Xterm* xtermPtr;
    for(xtermPtr = xtermHead; xtermPtr!=NULL; xtermPtr = xtermPtr->next){
      if( (xtermPtr->xtermProcess)==pid){
      	//printf("xtermProcess = %d, pid = %d", xtermPtr->xtermProcess,pid);
        return xtermPtr;
      }
    }
    return NULL;
}

Xterm* getXtermByChatFd(int fd){
	Xterm* xtermPtr;
    for(xtermPtr = xtermHead; xtermPtr!=NULL; xtermPtr = xtermPtr->next){
      if(xtermPtr->chatFd==fd){
        return xtermPtr;
      }
    }
    return NULL;
}

void createSocketPair(int socketsArray[], int size){
  if(size <2){
    sfwrite(&lock, stderr, "CreateSocketPair(): insufficient size of array\n");
    //fprintf(stderr,"CreateSocketPair(): insufficient size of array\n");
  }
  int status=-1;
  status = socketpair(AF_UNIX,SOCK_STREAM,0,socketsArray);
  if(status<0){
    sfwrite(&lock, stderr, "CreateSocketPair(): error with socketpair()\n");
    //fprintf(stderr,"CreateSocketPair(): error with socketpair()\n");
  }
}

char** buildXtermArgs(char* xtermArgs[],int xOffset, char* otherUser, char* originalUser, int socketCopy, int auditFd){
  if(xOffset<0){
    sfwrite(&lock, stderr, "buildXtermArgs error(): defaulting to xOffset of 0\n");
    //fprintf(stderr,"buildXtermArgs error(): defaulting to xOffset of 0\n");
    xOffset=0;
  }

  xtermArgs[1]="-geometry\0";
  char offsetBuffer[100];
  sprintf(offsetBuffer,"45x35+%d",xOffset);
  xtermArgs[2]=offsetBuffer;
  xtermArgs[3]="-T\0";

  char user1[1024];
  memset(user1,0,1024);
  if(otherUser!=NULL){
    strncpy(user1,otherUser,1024);
  }
  xtermArgs[4]=user1;
  xtermArgs[5]="-e\0";
  xtermArgs[6]="./chat\0";


  //ARGS FOR THE CHAT PROGRAM 
  char socket[1024];
  sprintf(socket,"%d",socketCopy);
  xtermArgs[7]=socket;
  xtermArgs[8]=user1;

  char user2[1024];
  memset(user2,0,1024);
  if(originalUser!=NULL){
	strcpy(user2,originalUser);
  }
  xtermArgs[9]=user2;
  char fdBuf[10];
  memset(&fdBuf, 0, 10);
  sprintf(fdBuf, "%d", auditFd);
  xtermArgs[10]=fdBuf;
  return xtermArgs;
}

bool statFile(char* file){
  struct stat statFile;
  if (stat(file, &statFile) >= 0){
    return true;
  }else {
    //fprintf(stderr,"statFile(): file does not exist");
  }
  return false;
}

/*
 * Finds the directory in the path environment variable
 */
char* statFind(char* findDir, char* buffer){
  char* token;
  char* savePtr;
  char* pathArray[1024];

  //GET PATH STRING
  char path[strlen(getenv("PATH"))+1];
  memset(&path,0,strlen(path));
  strcpy(path, getenv("PATH"));

  //CHOP UP PATH, PUT IT INTO AN ARRAY
  token = strtok_r(path,":",&savePtr);
  int paths =0;
  while(token != NULL){
    pathArray[paths++]= token;
    token = strtok_r(NULL,":",&savePtr);
  }

  //BUILD NEW DIR
  int sourceSize = strlen(path) + strlen(findDir) + 1;
  char sourceDir[sourceSize];
  memset(&sourceDir,0,sourceSize);

  //TEST DIRS
  int i;
  for (i = 0; i < paths; i++){
  strcpy(sourceDir, pathArray[i]);
    strcat(sourceDir, "/");
    strcat(sourceDir, findDir);

    //IF FOUND, RETURN IN BUFFER
    if(statFile(sourceDir)){
      memset(&buffer,0,strlen(buffer));
      strcpy(buffer,sourceDir);
      return buffer;
    }
    memset(&sourceDir,0,sourceSize);
  }
  //NOT FOUND DIR
  return NULL;
}

int createXterm(char * sendToUser, char* originalUser, int auditFd){
  pid_t pid;
  int socketArr[10];

  //BUILD SOCKET PAIR AND SET IN PARENT 
  createSocketPair(socketArr,10);

  pid=fork();
  if(pid==0){ // CHILD
    //BUILD ARGS FOR XTERM 
    char xtermString[1024];
    memset(&xtermString,0,1024);
    char* xtermArgs[1024];
    buildXtermArgs(xtermArgs, 320 ,sendToUser, originalUser, socketArr[1], auditFd);

    //EXECUTE XTERM
    int execStatus =-1;
    execStatus = execvp( statFind("xterm",xtermString) ,xtermArgs);

    // FAILED XTERM, CLEAN UP GLOBAL RESOURCES
    if(execStatus<0){
      sfwrite(&lock, stderr, "createXterm():Failed to execute\n");
      //fprintf(stderr,"createXterm():Failed to execute\n");
      exit(EXIT_FAILURE); 
    }
  }
  else if(pid<0){ //PARENT
    sfwrite(&lock, stderr, "createXterm(): error forking\n");
    //fprintf(stderr,"createXterm(): error forking\n");
    exit(EXIT_FAILURE);
  }
  //UPDATE DATA STRUCTURES
  setChatUser(sendToUser,socketArr[0], pid);
  return socketArr[0];
}