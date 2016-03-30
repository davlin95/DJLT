#define UP_KEY "^[[A"
#define DOWN_KEY "^[[B"
#define RIGHT_KEY "^[[C"
#define LEFT_KEY "^[[D"


/* Assume no input line will be longer than 1024 bytes */
#define MAX_INPUT 1024

/*Header Prototypes */
int isBuiltIn(char * command);

bool executeArgArray(char * argArray[], char * environ[]);
char ** parseCommandLine(char* cmd, char ** argArray);
char *statFind(char *cmd);
void printError(char* command);
void createNewChildProcess(char* objectFilePath,char** argArray);

/* ANSII CODE FOR ARROW MOVEMENT */
char* moveLeftAscii = "\033[D";
char * moveRightAscii ="\033[C";
char * backspaceAscii = "\b";
char * deleteAscii = "\b \b";
char * eraseLineAscii = "\033[K";
char * cursorPosAscii = "\033[6n";
char * saveCursorPosAscii = "\033[s";
char * loadCursorPosAscii = "\033[u";


/* Terminal Variables */
typedef struct variableNode{
  char* key;
  char* value;
  struct variableNode * next;
}var;

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

  token = strtok_r(buffer,delimiters,&savePtr);
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

/* Test Function*/
void test(){

  // Print environment variables 
  /*
  printf("\nTesting environmentP: \n");
  char** env;
  for(env = envp; *env != '\0'; env++){
  	char * envString = *env;
  	printf("%s\n",envString);
  }*/

  /*Test: historyCommand
  char* str[]={"1","2","3","4","5","6","7","8","9","10","11","12","13","14","15","16","17","18","19","20","21","22","23","24","25",
    "26","27","28","29","30","31","32","33","34","35","36","37","38","39","40","41","42","43","44","45","46","47","48","49","50","51"};
  int i;
  
  for(i=0;i<51;i++){
  	storeHistory(str[i]);
  	printf("\nStoring string %s\n",str[i]);
  	fflush(stdout);
  }
  
  int moved=0;
  for(i=0;i<100;i++){
    moved=moveForwardInHistory();
    printf("\n Moved is %d\n",moved);
  	fflush(stdout);
  }
  if(moved >=0){
        historyCommand[moved]="->";
  }
  for(i=0; historyCommand[i]!=NULL;i++){
  	write(1,"\n",1);
  	write(1,historyCommand[i],strlen(historyCommand[i]));
  }
  write(1,"\n",1); */

  /*End Of Test History Command */
}


     
      


