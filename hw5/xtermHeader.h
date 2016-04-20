#include <sys/stat.h>

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

void createXterm(){
	pid_t pid;
	pid=fork();
	if(pid==0){
		char xtermString[1024];
		char* xtermArgs[5];
		memset(&xtermString,0,1024);
		int execStatus =-1;
		execStatus = execvp( statFind("xterm",xtermString) ,xtermArgs);
		if(execStatus<0)
			fprintf(stderr,"createXterm():Failed to execute");
	}

}