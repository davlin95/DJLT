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
void createNewChildProcess(char* objectFilePath,char** argArray, char** environ);

/* Small Helper Functions*/
char** parseByDelimiter(char** dst,char*src,char* delimiters){
  char* savePtr;
  char* token;
  int argCount=0;

  token = strtok_r(src,delimiters,&savePtr);
  while(token!=NULL){
    dst[argCount++]=token;
    token = strtok_r(NULL,delimiters,&savePtr);
  }
  dst[argCount]=NULL;
  return dst;
}

void printError(char * command){
  write(2,"\n",1);
  write(2,command,strnlen(command,MAX_INPUT));
  char * msg = ":command not found\n\0";
  write(2,msg,strlen(msg));  
}