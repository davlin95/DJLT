#include <sys/stat.h>
#include <sys/types.h>
#include <sys/socket.h>

int allChatFds[1024];
char* allChatUsers[1024];
int chatIndex =0;

void setChatUser(char* username, int fd){
	char* newUser = malloc((strlen(username)+1)*sizeof(int));
	strcpy(newUser,username);

	allChatFds[chatIndex]=fd;
	allChatUsers[chatIndex]=newUser;
	chatIndex++;
}

int getIndexFromFd(int fd){
	int i;
	for(i=0; i< 1024;i++){
		if(allChatFds[i]==fd){
			return i;
		}
	}
	return -1;
}

bool deleteChatUserByUsername(char* username){
	int fd = getFdFromusername(username);
	if(fd<0){
		fprintf(stderr,"deleteChatUserByUsername(): username not associated with an fd\n");
		return false;
	}
	int index = getIndexFromFd(fd);
	if(index<0){
		fprintf(stderr,"deleteChatUserByUsername(): index not found for this fd\n");
		return false;
	}
	if(allChatUsers[index]!=NULL){
		free(allChatUsers[index]);
		allChatUsers[index]=NULL;
	}else{
		fprintf(stderr,"deleteChatUserByUsername(): no strings associated with this index\n");
		return false;
	}
}


void initializeChatGlobals(){
	memset(allChatUsers,'\0',sizeof(allChatUsers));
	memset(allChatFds,0,1024);
}


int getFdFromUsername(char* username){
	if(username==NULL){
		fprintf("getFdFromUsername(): input username is NULL\n");
	}
	int i;
	for(i=0;i<1024;i++){
		if(allChatUsers[i]!=NULL && (strcmp(username,allChatUsers[i])==0) ){
		  if(allChatFds[i]>0)
		  	return allChatFds[i];
		}
	}
	//fprintf("getFdFromUsername(): username not found in list.\n");
	return -1;
}


char* getUsernameFromFd(int fd){
	if(fd <0){
		fprintf("getUsernameFromFd(): input fd is erroneous\n");
	}
	int i;
	for(i=0;i<1024;i++){
		if(allChatFds[i]>0 && allChatFds[i]==fd ){
		  if(allChatUsers[i]!=NULL)
		  	return allChatUsers[i];
		}
	}
	//fprintf("getFdFromUsername(): fd not found in list.\n");
	return NULL;
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

char** buildXtermArgs(char* xtermArgs[],int xOffset, char* userName, int socketCopy){
	if(xOffset<0){
		fprintf(stderr,"buildXtermArgs error(): defaulting to xOffset of 0\n");
		xOffset=0;
	}
	xtermArgs[1]="-geometry\0";
	char offsetBuffer[100];
	sprintf(offsetBuffer,"45x35+%d",xOffset);
	xtermArgs[2]=offsetBuffer;
	xtermArgs[3]="-T\0";

	char user[1024];
	memset(user,0,1024);
	if(userName!=NULL){
		strncpy(user,userName,1024);
	}
	xtermArgs[4]=user;
	xtermArgs[5]="-e\0";
	xtermArgs[6]="./chat\0";

	char socket[1024];
	sprintf(socket,"%d",socketCopy);
	xtermArgs[7]=socket;

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

void createXterm(char * sendToUser){
	pid_t pid;
	pid=fork();
	if(pid==0){
		//BUILD ARGS
		char xtermString[1024];
		memset(&xtermString,0,1024);
		char* xtermArgs[1024];

		//BUILD SOCKET PAIR 
		int socketArr[10];
		createSocketPair(socketArr,10);

		buildXtermArgs(xtermArgs, 320 ,sendToUser,socketArr[1]);

		//EXECUTE XTERM
		int execStatus =-1;
		execStatus = execvp( statFind("xterm",xtermString) ,xtermArgs);
		if(execStatus<0)
			fprintf(stderr,"createXterm():Failed to execute");
	}else if(pid<0){
		fprintf(stderr,"createXterm(): error forking\n");
	}
}