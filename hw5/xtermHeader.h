#include "clientHeader.h"


					/*************************/
					/*     GLOBALS CHAT     */
					/***********************/
int allChatFds[1024];
pid_t xtermArray[1024];
char* allChatUsers[1024];
int chatIndex =0;

 					/************************************************/
                    /*           XTERM CHAT PROGRAM                */
                    /***********************************************/

/*
 *A function that makes the socket non-blocking
 *@param fd: file descriptor of the socket
 *@return: 0 if success, -1 otherwise
 */
int makeNonBlockingForChat(int fd){
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
  return 1;
}

void setChatUser(char* username, int fd, pid_t xtermProcess){
  char* newUser = malloc((strlen(username)+1)*sizeof(int));
  strcpy(newUser,username);

  //ADD TO GLOBAL DATA STRUCTURES
  allChatFds[chatIndex]=fd;
  allChatUsers[chatIndex]=newUser;
  xtermArray[chatIndex]=xtermProcess;
  chatIndex++;

  if (makeNonBlockingForChat(fd)<0){ 
    fprintf(stderr, "Error making fd nonblocking.\n");
  }

  //ADD TO GLOBAL POLL STRUCT THAT CORRESPONDS TO THE CHATFD GLOBALS
  clientPollFds[clientPollNum].fd = fd;
  clientPollFds[clientPollNum].events = POLLIN;
  clientPollNum++;
  printf("setChatUser(): fd just set is : %d clientPollNum for it is %d\n",clientPollFds[clientPollNum-1].fd, clientPollNum-1);

}


int getChatFdFromUsername(char* username){
  if(username==NULL){
    fprintf(stderr,"getFdFromUsername(): input username is NULL\n");
  }
  int i;
  for(i=0;i<1024;i++){
    if(allChatUsers[i]!=NULL && (strcmp(username,allChatUsers[i])==0) ){
      if(allChatFds[i]>=0)
        return allChatFds[i];
    }
  }
  return -1;
}


char* getChatUsernameFromChatFd(int fd){
  if(fd <0){
    fprintf(stderr,"getUsernameFromFd(): input fd is erroneous\n");
  }
  int i;
  for(i=0;i<1024;i++){
    if(allChatFds[i]>=0 && allChatFds[i]==fd ){
      if(allChatUsers[i]!=NULL)
        return allChatUsers[i];
    }
  }
  return NULL;
}

/*
 * A function that gets the chat index associated with the pid, if not found returns -1;
 */ 
int getChatIndexOfPid(pid_t pid){
	int i;
	for(i=0;i<1024;i++){
		if(xtermArray[i]==pid){
			return i;
		}
	}
	return -1;
}

/*
 * A function that gets the index of fd in the allChatsFds array 
 */
int getChatIndexFromFd(int fd){
  int i;
  for(i=0; i< 1024;i++){
    if(allChatFds[i]==fd){
      printf("getIndexFromFd(): found index for this fd %d at index %d\n",fd,i);
      return i;
    }
  }
  return -1;
}

/*
 * A function that searches for the corresponding index in the poll structure that contains this FD. 
 */
int getPollIndexFromFd(int fd){
  int i;
  for(i=0; i< 1024;i++){
    if(clientPollFds[i].fd==fd){
      return i;
    }
  }
  return -1;
}

/*
 * Send SIGINT to process 
 */
void killXterm(pid_t xtermProcess){
	if(xtermProcess>=0){
	  kill(xtermProcess,SIGINT);
	}
}

/*
 * A function that cleans up relevant global information by username. 
 */
bool deleteChatUserByUsername(char* username){
  //GET FD ASSOCIATED WITH THE USERNAME
  int fd = getChatFdFromUsername(username);
  if(fd<0){
    fprintf(stderr,"deleteChatUserByUsername(): username not associated with an fd\n");
    return false;
  }

  // GET THE CHAT INDEX ASSOCIATED WITH THE FD 
  int index = getChatIndexFromFd(fd);
  if(index<0){
    fprintf(stderr,"deleteChatUserByUsername(): index not found for this fd\n");
    return false;
  }

  //CLEANS UP DATA AT THE INDEX SAFELY, FOR THE USERNAME STRING AND PID PROCESS
  if(allChatUsers[index]!=NULL){
    free(allChatUsers[index]);
    allChatUsers[index]=NULL;
    killXterm(xtermArray[index]);
    xtermArray[index]=-1;
  }else{
    fprintf(stderr,"deleteChatUserByUsername(): no strings associated with this index\n");
    return false;
  }

  //GET THE INDEX OF THE USER'S FD IN THE POLL STRUCTURE
  int pollIndex = getPollIndexFromFd(fd);
  if(pollIndex<0){
    fprintf(stderr,"deleteChatUserByUsername(): error finding pollindex for the fd\n");
    return false;
  }

  //CLEAN UP THE USER FD IN THE POLL STRUCTURE AND THE ALLCHAT STRUCTURE
  close(clientPollFds[pollIndex].fd);
  clientPollFds[pollIndex].fd=-1;
  allChatFds[index]=-1;

  return true;
}


void initializeChatGlobals(){
  memset(allChatUsers,'\0',sizeof(allChatUsers));
  memset(allChatFds,0,1024);
  memset(xtermArray,-1,1024);
}


void createSocketPair(int socketsArray[], int size){
  if(size <2){
    fprintf(stderr,"CreateSocketPair(): insufficient size of array\n");
  }
  int status=-1;
  status = socketpair(AF_UNIX,SOCK_STREAM,0,socketsArray);
  if(status<0){
    fprintf(stderr,"CreateSocketPair(): error with socketpair()\n");
  }

}

char** buildXtermArgs(char* xtermArgs[],int xOffset, char* otherUser, char* originalUser, int socketCopy){
  if(xOffset<0){
    fprintf(stderr,"buildXtermArgs error(): defaulting to xOffset of 0\n");
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

int createXterm(char * sendToUser, char* originalUser){
  printf("before setChatUser clientPollNum = %d\n",clientPollNum);
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
    buildXtermArgs(xtermArgs, 320 ,sendToUser, originalUser, socketArr[1]);

    //EXECUTE XTERM
    int execStatus =-1;
    execStatus = execvp( statFind("xterm",xtermString) ,xtermArgs);

    // FAILED XTERM, CLEAN UP GLOBAL RESOURCES
    if(execStatus<0){
      fprintf(stderr,"createXterm():Failed to execute\n");
      deleteChatUserByUsername(sendToUser);
      exit(EXIT_FAILURE); 
    }
  }else if(pid<0){
    fprintf(stderr,"createXterm(): error forking\n");
    exit(EXIT_FAILURE);
  }

  //UPDATE DATA STRUCTURES
  setChatUser(sendToUser,socketArr[0], pid);

  return socketArr[0];
}