#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <termios.h>
#include <sys/wait.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <signal.h>
#include <stdbool.h>
#include <curses.h>
#include <stdlib.h>


/* Assume no input line will be longer than 1024 bytes */
#define MAX_INPUT 1024

/*Debug variables */
int debugHistory = 0;

/*******************/
/*  STRUCTS         */
/********************/

/* Struct Variables */
typedef struct jobNode{
  pid_t pid;
  pid_t processGroup;
  char jobName[MAX_INPUT];
  int exitStatus;
  int runStatus;
  struct jobNode * next;
  struct jobNode * prev;
}Job;


/*********************/
/*Header Prototypes */
/********************/

//Main Program
bool executeArgArray(char * argArray[], char * environ[]);
char ** parseCommandLine(char* cmd, char ** argArray);
char *statFind(char *cmd);
void printError(char* command);
void processExit();
int isBuiltIn(char * command);
void printShallowJobNode(Job* jobPtr);
void killChildHandler();
void createNewChildProcess(char* objectFilePath,char** argArray,int foreground);
char* getNextNonEscapeQuoteAfterPosition(char* position);

//HistoryCommand Program
int moveBackwardInHistory();
int moveForwardInHistory();
bool hasHistoryEntry();
void storeHistory(char * string);
bool saveHistoryToDisk();
void storeCommandLineCache(char* cmd);
void  initializeHistory();
void printHistoryCommand();
void clearHistory();

//JobNode Program
bool checkForBackgroundSpecialChar(char* argArray[],int argCount);
void createBackgroundJob(char* newJob, char** argArray,bool setForeground);
char* runStatusToString(int runStatus);
Job* setJobNodeValues(pid_t pid, pid_t processGroup, char* jobName, int exitStatus, int runStatus);
void printJobList();
Job* getJobNode(pid_t pid);
Job* getJobNodeAtPosition(int position);
int getPositionOfJobNode(Job* node);
void printJobNode(Job* jobPtr, int JID);
void processFG(char ** argArray,int argCount);
bool suspendProcess(pid_t pid);
bool killProcess(pid_t pid);
bool continueProcess(pid_t pid);
void processJobs();
int jobSize();
Job* createJobNode(pid_t pid, pid_t processGroup, char* jobName, int exitStatus, int runStatus);
char* runStateSymbol(int runStatus);
//SIGNALS
 void printJobListWithHandling();
 void waitKillChildHandler();

/* ANSII CODE FOR ARROW MOVEMENT */
char* moveLeftAscii = "\033[D";
char * moveRightAscii ="\033[C";
char * backspaceAscii = "\b";
char * deleteAscii = "\b \b";
char * eraseLineAscii = "\033[0K";
char * cursorPosAscii = "\033[6n";
char * saveCursorPosAscii = "\033[s";
char * loadCursorPosAscii = "\033[u";
char * queryCursorAscii = "\033[6n";


    /*Cursor Wrappers */
void queryCursorPos(){
  write(1,queryCursorAscii,strlen(queryCursorAscii));
}
void deleteCharAtCursor(){
  write(1,deleteAscii,strlen(deleteAscii));
}
void clearLineAfterCursor(){
  write(1,eraseLineAscii,strlen(eraseLineAscii));
}
void moveCursorLeft(){
  write(1,moveLeftAscii,strlen(moveLeftAscii));
}
void moveCursorRight(){
  write(1,moveRightAscii,strlen(moveRightAscii));
}
void moveCursorBack(int spaces){
  if(spaces<0){
    printError("Can't move cursor back negative spaces\n");
  }else{
    int i=0;
    while(i<spaces){
      moveCursorLeft();
      i++;
    }
  }
}
void moveCursorForward(int spaces){
  if(spaces<0){
    printError("Can't move cursor back negative spaces\n");
  }else{
    int i=0;
    while(i<spaces){
      moveCursorRight();
      i++;
    }
  }
}


/* Check for special char & */
bool checkForBackgroundSpecialChar(char* argArray[],int argCount){
  char* str;
  int strLen, i;
    for(i=0;i<argCount;i++){
      if(strcmp(argArray[i],"&")==0) {
        return true;
      }
      else{
      str = argArray[i];
      strLen = strnlen(str,1024);
      if(  (*(str+strLen-1)) == '&' ){
        return true;
      }
    }
  }
  
  return false;
}

void safeDelinkJobNode(Job* jobPtr){
  if(jobPtr!=NULL){
    if((jobPtr->prev)!=NULL){
         (jobPtr->prev)->next = jobPtr->next;
    }
    if((jobPtr->next)!=NULL){
         (jobPtr->next)->prev = jobPtr->prev;
    }
  }
}

void safeFreePtrNull(char* ptr){
  if(ptr!=NULL){
    free(ptr);
  }
  ptr = NULL;
}

char* safeMalloc(char* ptr){
  /*Assign pointer, or new pointer space*/
  ptr = malloc(MAX_INPUT *sizeof(char));
  printf("size of ptr is %lu",sizeof(ptr));
  if(ptr==NULL){
    printError("\nMalloc Ran out of space: Exiting...");
    processExit();
  }
  return ptr;
}

/* Small Helper Functions*/

/* Parses src, separating it into tokens by delimiters. Does not affect src array. 
 * stores the tokens into the destination array dst. when dst array is full of tokens,
 * end the last element with a terminating byte '\0' " */
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
  printf("dst is %s\n", dst[0]);
  printf("dst is %s\n", dst[1]);
  printf("dst is %s\n", dst[2]);
  return dst;
}

int parseByDelimiterNumArgs(char*src,char* delimiters){
  char* savePtr;
  char* token;
  int argCount=0;
  char* argBuffer=malloc(MAX_INPUT*sizeof(char));
  strncpy(argBuffer,src,MAX_INPUT);

  /*Test src, and buffer equality
  printf("\n%s\n","Test src, and buffer equality");
  printf("\n%s\n",src);
  printf("\n%s\n",buffer);
  fflush(stdout);*/

  token = strtok_r(src,delimiters,&savePtr);
  while(token!=NULL){
    argCount++;   
    token = strtok_r(NULL,delimiters,&savePtr);
  }
  free(argBuffer);
  return argCount;
}

void printError(char * command){
  write(2,"\n",1);
  write(2,command,strnlen(command,MAX_INPUT));
  char * msg = ": command not found\n\0";
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
    write(1,":No such file or directory\n", 28);
  }
  return 0;
}

char *checkQuote(char *cmd){
  char *firstQuote;
  char *nextQuote;
  char *lastQuote;
  if ((firstQuote = strchr(cmd, '"')) != NULL){
    lastQuote = strrchr(cmd, '"');
    firstQuote++;
    if (firstQuote == lastQuote){
        firstQuote--;
        *firstQuote = ' ';
        *lastQuote = ' ';
    }
    else{
      while (firstQuote != lastQuote){
        nextQuote = strchr(firstQuote,'"');
        if (firstQuote == nextQuote){
          firstQuote--;
          *firstQuote = ' ';
          *nextQuote = ' ';
        }
        else{
          while (firstQuote != nextQuote){
            if (*firstQuote == ' ')
              *firstQuote = 0xEA;
            firstQuote++;
          }
        }
        if (firstQuote != lastQuote){
          firstQuote++;
          firstQuote = strchr(firstQuote, '"');
          firstQuote++;
          if (firstQuote == lastQuote){
            firstQuote--;
            *firstQuote = ' ';
            *lastQuote = ' ';
            firstQuote++;
          }
        }
      }
    }
  }
  return cmd;
}
void changeDir(char *changeDirTo){
  setenv("OLDPWD", getenv("PWD"), 1);
  setenv("PWD", changeDirTo, 1);
  chdir(changeDirTo); 
}

/* Test Function*/
void test(){
  char * args[5]={"abc","c", "d","e","f&"};
  printf("test found & : %d\n",checkForBackgroundSpecialChar(args,5)); fflush(stdout);

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


     
      


