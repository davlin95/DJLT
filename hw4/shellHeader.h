#define UP_KEY "^[[A"
#define DOWN_KEY "^[[B"
#define RIGHT_KEY "^[[C"
#define LEFT_KEY "^[[D"


/* Assume no input line will be longer than 1024 bytes */
#define MAX_INPUT 1024

/*Header Prototypes */
int isBuiltIn(char * command);
char ** parseCommandLine(char* cmd, char ** argArray);
void executeArgArray(char * argArray[],char * environ[]);
char *statFind(char *cmd);
void printError(char* command);
void createNewChildProcess(char* objectFilePath,char** argArray);


/* Small Helper Functions*/
char** parseByDelimiter(char** dst,char*src,char* delimiters){
  char* savePtr;
  char* token;
  int argCount=0;
  char buffer[strlen(src)+1];
  strcpy(buffer,src);

  /*Test src, and buffer equality
  printf("\n%s\n","Test src, and buffer equality");
  printf("\n%s\n",src);
  printf("\n%s\n",buffer);
  fflush(stdout);*/

  token = strtok_r(src,delimiters,&savePtr);
  while(token!=NULL){
    dst[argCount++]=token;

    /*Print Token
    char * str = "\n token value is: ";
    write(1,str,strlen(str));
    write(1,dst[argCount-1],strlen(dst[argCount-1]));*/
    
    token = strtok_r(NULL,delimiters,&savePtr);
  }
  dst[argCount]='\0';
  return dst;
}

void printError(char * command){
  write(2,"\n",1);
  write(2,command,strnlen(command,MAX_INPUT));
  char * msg = ":command not found\n\0";
  write(2,msg,strlen(msg));  
}

char* parentDirectory(char* currentDirPath){
  if (strcmp(getenv("HOME"), currentDirPath) != 0){
    char *lastSlash = strrchr(currentDirPath, '/');
    if(lastSlash!=NULL)
      *lastSlash = '\0';
  }
  return currentDirPath;
}

char* directoryAppendChild(char* currentDirPath, char* child){
  return strcat(strcat(currentDirPath,"/"), child);
}

int statExists(char* dir){
  struct stat pathStat;
  if (stat(dir, &pathStat) >= 0){
    return 1;
  }else {
    write(1,dir,strlen(dir));
    write(1,": No such file or directory\n", 28);
  }
  return 0;
}


     
      


