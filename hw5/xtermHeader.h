#include <sys/stat.h>
#include <sys/types.h>
#include <sys/socket.h>



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
	}
}