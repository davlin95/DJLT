#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <termios.h>
#include <sys/wait.h>
#include <sys/stat.h>
#include <stdbool.h>
#include <curses.h>
#include <fcntl.h>
#include "shellHeader.h"

/*Global Error Var */
int error = 0;

/*Strings for help menu: WARNING!!! LAST ELEMENT MUST BE NULL, for the printHelpMenu() function */
char * helpStrings[]={"exit [n]","pwd -[LP]","set [-abefhkmnptuvxBCHP] [-o option->",
"ls [-l]","cd [-L|[-P [-e]] [-@]] [dir] ", "echo [-neE] [arg ...]","ps [n]","printenv [n]",NULL};

/* Make global array of built-in strings */
char * globalBuiltInCommand[] = {"cd","pwd","help","echo","set","exit"};
char temporaryCommandlineCache[MAX_INPUT];
char *temporaryCommandlineCachePtr;
bool inCommandlineCache;
char restOfCMDLine[MAX_INPUT];

/* Function fills in buffer with the bytes between two cursors, into global array restOfCMDLine*/
bool storeBytesBetweenPointersIntoGlobalArray(char* cursorSpot, char*lastCursorSpot){
  int numElements = lastCursorSpot-cursorSpot;

  /*Check bad cases */
  if((numElements>MAX_INPUT)|| (numElements<0)) 
    return false;

  char* restOfCMDLinePtr= restOfCMDLine;
  /*Copy bytes between the pointers */
  while(cursorSpot<=lastCursorSpot){
    *restOfCMDLinePtr = *cursorSpot;
    cursorSpot++;
    restOfCMDLinePtr++;
  }
  if(*cursorSpot!='\0') {
  	*cursorSpot = '\0';
  }
  //printf("\nrestofcommandline is: %s\n",restOfCMDLine);
  return true;
}

/*
 * A function that stores the copy of the string passed, into the global string buffer temporaryCommandlineCache
 * @param cmd: the string to be cached. 
 * @return: none
 */
void storeCommandLineCache(char* cmd){
  if(cmd!=NULL){
  	temporaryCommandlineCachePtr=temporaryCommandlineCache;
    strncpy(temporaryCommandlineCache,cmd,strnlen(cmd,MAX_INPUT));
  }else temporaryCommandlineCachePtr=NULL;
}

/*
 * A function that prints the strings in the global helper array[] for the help menu
 * @return: none
 */
void printHelpMenu(){
  char** str = helpStrings;
  while(*str!=NULL){
  	printf("%-30s\n",*str); 
  	str++;
  }
  fflush(stdout);
}

          
/* Terminal Variables */
var* varHead=NULL;

/*
 * A function that returns the current variable node.
 * @param keyString: the key string 
 * @return var: the corresponding node that has the key-value pair;
 */
var* getVarNode(char* keyString){
  /*Initialize*/
  char* str;
  var* varPtr = varHead;

  /*check case*/
  if(varPtr ==NULL) return NULL;
  
  /* search list for matching keyString*/
  while(varPtr!=NULL){ 
    str = varPtr->key;
    int result =0;
    if( (result = strcmp(str,keyString))==0) 
      return varPtr;
  	else 
  	  varPtr = varPtr->next;
  }
  return NULL;
}

/*
 * A function that creates a node (key/value) pair
 * @param key: the key string
 * @param value: the value string
 * @param var* : the pointer to the created struct in memory.
 */
var* createNode(char* key, char* value){
  static var node;
  node.key = key;
  node.value = value;
  return &node;
}

/*
 * A function that sets a new value to the key, or creates a new key-value pair if key not exists. 
 * @param key: the key string
 * @param value: the value string
 * @return : none
 */
void setKeyValuePair(char* key, char* value){
  var* node = getVarNode(key);
  if(node!=NULL){
  	node->value = value;
  }else{
    node = createNode(key,value);
    node->next = varHead;
    varHead = node;
  }
}

/*
 * Global variables for the history 
 */
int historySize = 50;
char* historyCommand[50];
int historyHead = 0;
int current=0;

/*
 *  A helper function that prints the history for debugging 
 *  @return: void
 */
void printHistoryCommand(){
  
    /*If history is NULL */
  int i=0;
  if(historyCommand[i]==NULL){
    printf("History command is null");fflush(stdout);
  } 
    /*Prints history */
  while(historyCommand[i]!=NULL){
    printf("\nhistoric string at index %d: %s\n",i,historyCommand[i]);fflush(stdout);
    i++;
  }
}

/*
 * A function that stores a string into history global structure. 
 * @param string: the string to be stored. 
 * @return : none
 */
 void storeHistory(char * string){
  int headIndex = historyHead % historySize; /*Valid index within the array */
  
  /* Free the previous string */
  if(historyCommand[headIndex]!=NULL){
  	free(historyCommand[headIndex]);
  }
  /* Allocate memory for new string */
  char* newString = malloc((strlen(string)+1)*sizeof(char));
  strcpy(newString,string);
  historyCommand[headIndex]=newString;

  /* Reset counters*/
  historyHead++; /*Next available space to store next string */
  current=(historyHead-1); /*Index of most current String */
}

/*
 * A function that moves the current history point forward. Changes stored in history are persistent.
 * Up to value (head -1), which is latest string history entry
 * @return int: the int value that can be inserted into historyCommand to retrieve the next history string.
 */
int moveForwardInHistory(){
    /* While conditions:
	 * 1) Current not exceed head
	 * 2) next history string is not the buffered input [address of historyHead]
	 */
  if( (current<historyHead) && 
  	(( (current+1)%historySize) != (historyHead%historySize))) {
  	current++;
  	return (current%historySize);
  }
  /* Can't move forward, don't increment */
  return(current%historySize); 
}

/*
 * A function that moves the current history point backwards. Changes stored in history are persistent.
 * Up to value 1, which is next to-earliest string history index. S
 * @return int: the int value that can be inserted into historyCommand to retrieve the prev history string.
 */
int moveBackwardInHistory(){
    /* While conditions:
     * 1) cursor before historyHead
	 * 2) Current is not the earliest string, so it can move backwards
	 */
	if((current<historyHead)
	  &&(current>0)
	  &&(((current-1)%historySize) != ((historyHead-1)%historySize))){ /*Not overwriting the most current cmd string*/
		current--;
		return(current%historySize);
	}
    /* Can't move forward, don't increment */
	return(current%historySize);
}

/* 
 * A function that determines whether there are strings in the history at all. 
 * @return: false if history is empty, true if at least one history string stored. 
 */
bool hasHistoryEntry(){
  int headIndex = (historyHead-1) % historySize; /*Valid index within the array */
  /* Free the previous string */
  if(historyCommand[headIndex]!=NULL){
  	return true;
  }
  return false;
}

/* 
 * A function that turns relative to absolute path evaluation. 
 * @param cmdString: string to be parsed. 
 * @param buffer: buffer to be returned. contains the evaluated path. 
 * @param buffer size: size of the buffer, safety. 
 * @return char*: filled buffer. 
 */
char* turnRelativeToAbsolute(char* cmdString,char* buffer, int bufferSize){
  if(strlen(buffer)>bufferSize)bufferSize = strlen(buffer);
  if( bufferSize<=0 )return NULL; //invalid

  char* buildingPath = buffer; 
  strcpy(buildingPath,getenv("PWD"));

  /*Test print 
  printf("\nbuildingPath is: %s\n",buildingPath);
  char* testBuff=calloc(strlen(buildingPath),1);
  strcpy(testBuff, buildingPath);
  printf("\nparentPath is: %s\n",parentDirectory(buildingPath));
  printf("commandString is: %s\n",cmdString);
  fflush(stdout); 
  free(testBuff);*/

  /*Find the traversal path */

  char* savePtr;
  char* token;
  int argCount=0;
  char * traversal[MAX_INPUT];
  char buffer2[strlen(cmdString)+1];
  strcpy(buffer2,cmdString);
  token = strtok_r(buffer2,"/",&savePtr);
  while(token!=NULL){
    traversal[argCount++]=token;
    token = strtok_r(NULL,"/",&savePtr);
  }
  traversal[argCount]='\0';
  //parseByDelimiter(traversal,cmdString,"/ ");

  /*Test Print the array we just filled 
  int i =0;
  while(traversal[i] != NULL&& traversal[i] !='\0'){
    printf("traversal is: %s\n",traversal[i]);
    i++;
  }
  fflush(stdout);*/

  /* Build the path to the file */
  int count =0;
  int result=0; 
  //printf("travesal is %s\n", *traversal);
  while(traversal[count] != NULL && traversal[count] !='\0'){
  	if( (result = strcmp(traversal[count],".")) ==0){ 

  	}else if((result = strcmp(traversal[count],"..")) ==0){ /* Move up one parent*/
      parentDirectory(buildingPath);
      printf("\n moved to parent path: %s\n",buildingPath);
      fflush(stdout);
  	}else{  
  	  /*Append */
      directoryAppendChild(buildingPath,traversal[count]);
      printf("\nappended child, buildingPathPtr is: %s\n",buildingPath);
      fflush(stdout);
  	}
    count++; /* Move onto next traversal string */
  }

  /*Test print result
  printf("built finally: %s\n",buildingPathPtr);
  fflush(stdout);*/

  if(bufferSize>0){
  	return buffer;
  }
  return NULL; /*if invalid size */
}

/* 
 * A function that determines alphanumerical-ness of a string.
 * @param argString : string to be parsed and determined. 
 * @return: int 0 if false, 1 if true. 
 */
int alphaNumerical(char *argString){
  int i;
  char* position = argString;
  for (i = 0; i < strlen(argString); i++){
    if ((*position > 47 && *position < 58) || 
    	(*position > 64 && *position < 91) ||
    	 (*position > 96 && *position < 123)){

      /*Move to the next position */
      position++;
    }else{
    	/* failed test*/
    	return 0;
    }
  }
  return 1;

  /* LEGACY CODE:
  if (position == argString + strlen(argString))
    return i;
  return 0; */
}

char ** parseCommandLine(char* cmd, char ** argArray){

  /* Headline printing for debugging
  char * promptString = "\nprinting commandline:\n\0";
  write(1,promptString,strlen(promptString)); 
  int cmdLen = strnlen(cmd,MAX_INPUT); 
  write(1,cmd,cmdLen); */

  /* initialize*/
  char * token;
  char * savePtr;
  int argCount = 0;
  char cmdCopy[MAX_INPUT];
  strcpy(cmdCopy,cmd),
  checkQuote(cmdCopy);
  token = strtok_r(cmdCopy," ",&savePtr);
  while(token != NULL){
    argArray[argCount]= token;
    argCount++;
    token = strtok_r(NULL," ",&savePtr);
  }
  argArray[argCount]=NULL; /* Null terminate */
  int position;

  /*Reparse, and turn quotes into space */
  for (position = 0; position < argCount; position++){
    char *temp = argArray[position];
    if (*temp == '"'){
      char *end = temp + strlen(argArray[position]) - 1;
      *end = '\0';
      temp++;
      char *current;
      for (current = temp; current < end; current++){
        if (*current == '\352')
          *current = ' ';
      }
    argArray[position]=temp;
    }
  }
  /* Parsed a non-empty string, store in history*/
  if(argCount>0){
    storeHistory(cmd);
    //printf("\nStored into historyCommand");fflush(stdout);
  }else{
    //printf("\nUnable to store into historyCommand");fflush(stdout);
  } 
  return argArray;
}

char *statFind(char *cmd){
  char *token;
  char *savePtr;
  char *pathArray[MAX_INPUT];
  int argCount = 0;
  char *command;
  struct stat pathStat;
  char path[strlen(getenv("PATH"))];
  strcpy(path, getenv("PATH"));
  token = strtok_r(path,":",&savePtr);
  while(token != NULL){
    pathArray[argCount]= token;
    argCount++;
    token = strtok_r(NULL,":",&savePtr);
  }
  for (int i = 0; i < argCount; i++){
    char src[strlen(pathArray[i]) + strlen(cmd) + 1];
    strcat(strcpy(src, pathArray[i]), "/");
    if (stat((command = strcat(src, cmd)), &pathStat) >= 0)
      return command;
  }
  return NULL;
}

int isBuiltIn(char * command){
  /* Num elements of built in commands */
  int numCommands = (int)sizeof(globalBuiltInCommand)/sizeof(char *);

  /*Test : Prints out num elements in globalBuiltInCommand Array 
  printf("\nnumber of global builtin Commands : %d", numCommands);
  fflush(stdout); */
  
  int i;
  for(i=0; i< numCommands; i++){
  	if(strcmp(command,globalBuiltInCommand[i])==0)
  	 return 1;
  }
  return 0; /*No command match found. return false */
} 

void processCd(char argArray[]){
  printf("cd: argArray is %s\n", argArray);
  int badDirectory = 0;
  if (argArray != NULL && argArray[strlen(argArray)-1] == '/'){
    argArray[strlen(argArray)-1] = '\0';
  }
  if(argArray == NULL){
    changeDir(getenv("HOME"));
  } else if(strcmp(argArray,"-")==0){ 
    char *temp = malloc(strlen(getenv("OLDPWD"))*sizeof(char));
    strcpy(temp,getenv("OLDPWD"));
    changeDir(temp);
    free(temp);
  }else if(strcmp(argArray, ".") == 0){
    changeDir(getenv("PWD"));
  }else if(strncmp(argArray, "./", 2) == 0){
    char *argPtr = &argArray[1];
    char *path = malloc((strlen(getenv("PWD")) + strlen(argPtr))*sizeof(char));
    strcat(strcpy(path, getenv("PWD")), argPtr);
    struct stat pathStat;
    if (stat(path, &pathStat) >= 0){
      changeDir(path);
    }else 
      badDirectory = 1;
    free(path);
  }else if(strcmp(argArray, "..") == 0){
    char *path = malloc((strlen(getenv("PWD")))*sizeof(char));
    strcpy(path, getenv("PWD"));
    parentDirectory(path);
    changeDir(path);
    free(path);
  }else if (strncmp(argArray, "../", 3) == 0){
    printf("argArray is %s\n", argArray);
    printf("argArray[2] is %s\n", &argArray[2]);
    char *argPtr = &argArray[2];
    char *path = malloc((strlen(getenv("PWD")) + strlen(argPtr))*sizeof(char));
    strcpy(path, getenv("PWD"));
    parentDirectory(path);
    strcat(path, argPtr);
    printf("argptr is %s\n", argPtr);
    printf("cd ../ and path is %s\n ", path);
    struct stat pathStat;
    if (stat(path, &pathStat) >= 0){
      changeDir(path);
    }else 
      badDirectory = 1;
    free(path);
  } else {
    struct stat pathStat;
    char *path = malloc((strlen(getenv("PWD")) + strlen(argArray) + 1)*sizeof(char));
    strcpy(path, getenv("PWD"));
    strcat((strcat(strcpy(path, getenv("PWD")), "/")), argArray);
    if (stat(path, &pathStat) >= 0){
      changeDir(path);
    }
    else 
      badDirectory = 1;
    free(path);
  }
  if (badDirectory){
      write(2, "cd: ", 4);
      write(2, argArray, strlen(argArray));
      write(2, ": No such file or directory\n", 28);
      error = 1;
  }  
}
void processPwd(){
  char *path;
  char *buf = malloc(MAX_INPUT*sizeof(char));
  path = getcwd(buf, MAX_INPUT);
  write(1, path, strlen(path));
  write(1, "\n", 1);
  free(buf);
}
void processSet(char argArray[]){
  char *token = malloc(strlen(argArray)*sizeof(char));
  char *token2 = malloc(strlen(argArray)*sizeof(char));
  token = strtok(argArray, "=");
  token2 = strtok(NULL, "=");
  printf("argArray has changed to %s\n", argArray);
  printf("token is %s and token2 is %s\n", token, token2);
  if (getenv(token) != NULL){
    printf("setenv returned %d\n", setenv(token, token2, 1));
    printf("token is %s and token2 is %s\n", token, token2);
  }
  else if (alphaNumerical(token)){
    printf("setenv returned %d\n", setenv(token, token2, 1));
    printf("token is %s and token2 is %s\n", token, token2);
  } else 
    error = 1;
  printf("getenv(%s) is %s\n", token, getenv(token));
  free(token);
  free(token2);
}

void processEcho(char argArray[]){
  char *argPtr = &argArray[1];
  if (argArray != NULL){
    if (argArray[0] == '$'){
      if (strcmp(argPtr, "?")==0){
        if (error)
          write(1, "1", 1);
        else
          write(1, "0", 1);
        error = 0;
      }
      else if (getenv(argPtr) != NULL)
        write(1, getenv(argPtr), strlen(getenv(argPtr)));
    }
    else {
        write(1, argArray, strlen(argArray));
    }
  }
  write(1, "\n", 2);
}
void processExit(){
  if(!saveHistoryToDisk()){
  	printError("Error saving history to disk\n");
  }
  printHistoryCommand();
  exit(0);
}

bool executeArgArray(char * argArray[], char * environ[]){
  printf("-----executeargarray----\narg0 is %s\n", argArray[0]);
    printf("arg1 is %s\n", argArray[1]);
    printf("arg2 is %s\n", argArray[2]);
  int count; 

  for(count=0; argArray[count]!=NULL;++count)
  	;
  if(count==0) return false;
  /*Test: Print count 
  printf("\n value of count of argArray in executeArgArray is: %d\n", count);
  for(int i=0;i<count;i++){
  	printf("arg[%d] is %s\n", i,argArray[i]);
  }
  fflush(stdout);*/
  /*Test end*/

  int argPosition =0;
  char * command = argArray[argPosition];
  if(isBuiltIn(command)){ /*Pre-built binary command */

    /*Our implemented commands */
    if(strcmp(command,"cd")==0){ 
      processCd(argArray[1]);
    }else if(strcmp(command,"pwd")==0){
      processPwd();
    }else if(strcmp(command,"echo")==0){
      processEcho(argArray[1]);
    }else if(strcmp(command,"set")==0){
      processSet(argArray[1]);
    }else if(strcmp(command,"help")==0){
      printf("\nexecute help\n");
      printHelpMenu();
      fflush(stdout);
    }else 
      processExit();
  } else if(*(argArray[0]) != '.'){
    printf("else if arg0 is %s\n", argArray[0]);
    printf("arg1 is %s\n", argArray[1]);
    printf("arg2 is %s\n", argArray[2]);
      /* Test Print: Use statfind() to launch the program from shell 
      printf("\nuse statfind() to launch the built in\n");
      printf("%s\n",command);
      fflush(stdout); */

      /*Find and execute the binary path */
    char * fullCommandPath;
      if((fullCommandPath = statFind(argArray[0]))!=NULL){
        printf("-----afterstatfind----\narg0 is %s\n", argArray[0]);
    printf("arg1 is %s\n", argArray[1]);
    printf("arg2 is %s\n", argArray[2]);
        createNewChildProcess(fullCommandPath,argArray);
        printf("returned from forking\n");
      }else { /*Not found */
      	write(1,argArray[0],strlen(argArray[0]));
        printError(command);
        error = 1;
      }
  } else{ /* Is an object file such as a.out */
    /*Make buffer */
    int callocSize = MAX_INPUT + strlen(getenv("PWD")) + strlen("320sh> ") + 1;
    char* buffer= calloc(callocSize,1 );

    /*Parse into absolute path */
    if(turnRelativeToAbsolute(argArray[0],buffer,callocSize)!=NULL){
      printf("\nValue of buffer is: %s\n",buffer);
      if(statExists(buffer)){
      	safeFreePtrNull(buffer);
    	printf("dir exists: %s\n",buffer);
    	createNewChildProcess(buffer,argArray);
      printf("returned from creating child\n");
      free(buffer);
      }
      else{
      	/*Path Doesn't Exists: user error */
        error = 1;
      	safeFreePtrNull(buffer);
      }
    }else{
      /*turnRelativePathToAbsolute is erroneous, conversion error */
      safeFreePtrNull(buffer);
      fprintf(stderr,"Error in turnRelativeToAbsolute for %s",argArray[0]);
      return false;
    }
  }
  return true;
}
bool validSyntax(char **argArray){
  int position = 0;
  bool valid = true;
  if ((*argArray[position] == '<') || (*argArray[position] == '>') || (*argArray[position] == '|')){
    valid = false;
  }
  while(argArray[position] != NULL){
    position++;
  }
  if ((*argArray[position - 1] == '<') || (*argArray[position - 1] == '>') || (*argArray[position - 1] == '|')){
    valid = false;
  }
  if (!valid)
    printError(argArray[position]);
    
  return valid;
}
int checkPiping(char **argArray){
  printf("made it to check piping\n");
  int i = 0;
  int numberCmds = 1;
  int prevIsPipe = 0;
  while(argArray[i] != NULL){
    if (*argArray[i] == '|' && !prevIsPipe){
      argArray[i] = NULL;
      numberCmds++;
      prevIsPipe = 1;
    }else if (*argArray[i] == '|' && prevIsPipe){
      return 0;  
    }else 
      prevIsPipe = 0;
    i++;
  }
    return numberCmds;
}

void createNewChildProcess(char* objectFilePath,char** argArray){
  printf("made it to create new child\n");
  pid_t pid;
  int status;
  /* check whether arguments start or end with <, >, or | */
  if (validSyntax(argArray)){
    /* parse through argArray to find pipes and determine the # of commands */
    int noOfCmds;
    noOfCmds = checkPiping(argArray);
    /* create pipes with each pipe being two values in the array */
    int pipes[2*(noOfCmds-1)];
    /* pipe every other pipe i.e. 0 with 2, 2, with 4, and so on*/  
    int i;
    for (i = 0; i <(noOfCmds - 1); i+=2){
      if (pipe(pipes + i) < 0){
        write(2, "Error piping.\n", 14);
        error = 1;
      }
    }
    /* commands = index of current command executing (used to determine dup) */
    int commands = 0;
    /* nextArgArray contains the next array that will be executed */
    /* while number of commands is greater than 0, fork and pipe. (redirect if required) */
    while(noOfCmds > 0){
      if((pid=fork())==0){
        int argNo = 0;
        int fd;
        /* check for redirecting into */
        while (argArray[argNo] != NULL){
          if (strcmp(argArray[argNo], "<") == 0){
            fd = open(argArray[argNo + 1], O_RDONLY, 0666);
            dup2(fd, 0);
            close(fd);
          }
          argNo++;
        }
        argNo = 0;
        /*check for redirecting out of */
        while (argArray[argNo] != NULL){
          if (strcmp(argArray[argNo], ">") == 0){
            fd = open(argArray[argNo + 1], O_RDWR | O_CREAT | O_TRUNC, 0666);
            dup2(fd, 1);
            close(fd);
          }
          argNo++;
        }
        /* IF ONLY ONE CMD, EXECUTE AND EXIT */
        if (commands == 0 && noOfCmds == 1){ 
          if (execvp(objectFilePath,argArray) < 0){
            printError(objectFilePath);
            exit(1);
          }
          exit(0);
        }
        /* --------ELSE MORE THAN ONE COMMAND---------*/
         /* NOT LAST CMD */
        if (noOfCmds != 1){
            if (dup2(pipes[commands*2 + 1], 1) < 0){
              write(2, "Error piping.\n", 14);
              error = 1;
            }
        }
            /*NOT FIRST CMD */
        if(commands !=0){ 
            if (dup2(pipes[commands*2 - 2], 0) < 0){
              write(2, "Error piping.\n", 14);
              error = 1;
          }
        }
        /* for first command, just execute after creating pipes */
        if (commands == 0){
          if (execvp(objectFilePath,argArray) < 0){
            printError(objectFilePath);
            exit(1);
          }
        /* for remaining commands, determine arg array by getting position after null */
        } else {

          int count = 0;
          int i = 0; 
          while(count != commands){
            printf("count is %d and commands is %d\n", count, commands);
            while (argArray[i] !=NULL){
              i++;
              printf("i = %d\n", i);
            }
            i++;
            printf("i = %d\n", i);
            count++;
          }
          printf("i = %d\n", i);
          printf("count is %d but should be 3\n", i);
          printf("argArray[0] = %s\n", argArray[0]);
          printf("argArray[1] = %s\n", argArray[1]);
          printf("argArray[2] = %s\n", argArray[2]);
          printf("argArray[3] = %s\n", argArray[3]);
          printf("argArray[4] = %s\n", argArray[4]);
          printf("argArray[5] = %s\n", argArray[5]);
          printf("argArray[6] = %s\n\n", argArray[6]);
          printf("argArray[7] = %s\n\n", argArray[7])

          printf("argArray[null+1] = %s\n", argArray[i]);
          printf("argArray[null+2] = %s\n", argArray[i+1]);
          char *nextArgArray[MAX_INPUT];
          memset(nextArgArray, '\0', MAX_INPUT);
          int j = 0;
          while(argArray[i] != NULL){
            nextArgArray[j] = malloc(strlen(argArray[i]));
            strcpy(nextArgArray[j], argArray[i]);
            j++;
            i++;
          }
          nextArgArray[j+1] = NULL;
        /* after getting arg array, statfind to find path, and execute */
          if (execvp(statFind(nextArgArray[0]), nextArgArray) < 0){
            printError(nextArgArray[0]);
            exit(1);
          }
          int k = 0;
          while(nextArgArray[k] != NULL){
            free(nextArgArray[k]);
          }
        }
        exit(0);
  }else{
    if (noOfCmds != 1){
      close(pipes[commands*2 + 1]);
    }
    if (commands!=0){
      close(pipes[commands*2 - 2]);
    }
    noOfCmds--;
    commands++;
    /* -----to do -------- */
    /* close certain pipes opened by child in parent (not sure which ones) */
      wait(&status);
      if (status)
        error = 1;
      printf("\nParent Process reaped child process %d\n",pid);
  }
}
}
}
/* 
 * A function that determines if there is piping and returns the number of commands. 
 * For each pipe found, make null to create the different arrays for each command that will be 
 * passed into execvp. Returns 0 if there is an error.   
 */


/* 
 * A function that initializes the historyCommand global var, from a searched dir path 
 * for a text file in the home directory relative to the computer. 
 * Does not initialize if not found.  
 */
void  initializeHistory(){

  /*Initialize buffers for the directory to store history */
  int homePathSize = strlen(getenv("HOME"));
  char * historyFileExtension = "/.320sh_history\0";
  int historyFileExtensionSize = strlen(historyFileExtension);
  char* historyPath=malloc( (homePathSize + historyFileExtensionSize+1) *sizeof(char));

  /* Build the directory history */
  strcpy(historyPath, getenv("HOME"));
  strcat(historyPath,historyFileExtension);

  /* Start reading the history file */
  FILE *historyFile;
  historyFile = fopen(historyPath, "r");
  free(historyPath);

  char line[MAX_INPUT];
  if(historyFile){ 
  	/*Read for lines from file*/
    while(fgets(line, sizeof(line), historyFile)){
    	/* Check for NULL lines and lines of spaces, or '\n' */
      if (line != NULL){
      	if(parseByDelimiterNumArgs(line," \n")>0){
      		storeHistory(line);
      	}
      }
    }
	/*Handling*/
    fclose(historyFile);
  }else{
  	/* Print to stdout */
    printError("Unable to initialize prompt history");
  }
  printHistoryCommand();
}

/* A function that saves the history command global array into the computer's home directory. 
 * Should be called once upon entry into main. 
 */
bool saveHistoryToDisk(){

  /*Initialize buffers for the directory to store history */
  int homePathSize = strlen(getenv("HOME"));
  char * historyFileExtension = "/.320sh_history";
  int historyFileExtensionSize = strlen(historyFileExtension);
  char* historyPath= malloc((homePathSize + historyFileExtensionSize+1)*sizeof(char));
  if(historyPath==NULL) return false;

  printf("Concatenating:\n");
  /*Build the path */
  strcpy(historyPath,getenv("HOME"));
  strcat(historyPath,historyFileExtension);
  printf("HOME IS: %s", getenv("HOME"));

  /* Start writing the history file */
  FILE *historyFile;
  historyFile = fopen(historyPath,"w");
  printf("\nopen to file %d:\n",historyFile==NULL);
  printHistoryCommand();
  /*Tests: 
  printf("\n%s\n",historyPath);
  printf("\nhistoryFILE NULL?%d\n",(historyFile==NULL)); */

  /* Move to earliest point in recent history*/
  int i,currentHistoryIndex=-1,nextHistoryIndex=-1;
  for(i=0; i<historySize;i++){
    currentHistoryIndex = moveBackwardInHistory();
  }
  printf("moved back to beginning:\n");
   /* IMPORTANT: MUST BE '=', not double equals, "==". Is an assignment */
  while( (nextHistoryIndex=moveForwardInHistory()) 
    && (nextHistoryIndex != currentHistoryIndex) ){ // while there's more strings to store
      printf("inWhile: %d %d\n",nextHistoryIndex,currentHistoryIndex);
      printf("string is: %s\n",historyCommand[currentHistoryIndex]);
  	  // print the current history string to file
  	  if( historyCommand[currentHistoryIndex] != NULL){
        printf("File is null: %d\n",historyFile==NULL);
		fprintf(historyFile, "%s\n", historyCommand[currentHistoryIndex]); 
        printf("\nStoring %s",historyCommand[currentHistoryIndex]);
  	  }
      currentHistoryIndex=nextHistoryIndex;
  }
  /* No more strings left in forwardInHistory, one more time to store the final string */
  if( historyCommand[currentHistoryIndex] !=NULL){
    fprintf(historyFile,"%s\n", historyCommand[currentHistoryIndex]); 
    printf("Storing: %s\n", historyCommand[currentHistoryIndex]); 
  }
  /*Close file */
  free(historyPath);
  fclose(historyFile);
  printf("Done saving to disk:\n");
  return true;
}

int main (int argc, char ** argv, char **envp) {
  /*debug flag*/
  int debug = 0;
  int counter = 0;
  while (counter < argc){
    if (strcmp(argv[counter], "-d") == 0)
      debug = 1;
    counter++;
  }
  int quote = 0;
  int finished = 0;
  char *prompt = "320sh> ";
  char cmd[MAX_INPUT];
  bool escapeSeen = false;
  bool inCommandlineCache=true;
  
  /* Loads the history */
  initializeHistory();

  while (!finished) {
    char *cursor;  	//current position of modification 
    char *lastCursor;
    
    /*Used to shift elements in cmd for insertions. 
    char *relayer1;
    char *relayer2;
    char* tempByte='\0';*/

    char buffer; 
    char last_char;
    int rv;
    int rewrite=0;
    int count;
	int lastVisitedHistoryIndex_UP=-1;
	int lastVisitedHistoryIndex_DOWN=-1;
    // Print the current directory 
    write(1, "[", 1);
    write(1, getenv("PWD"), strlen(getenv("PWD")));
    write(1, "] ", 2);

    // Print the shell prompt
    rv = write(1, prompt, strlen(prompt));
    if (!rv) { 
      finished = 1;
      break;
    }
    write(1, saveCursorPosAscii, strlen(saveCursorPosAscii));

      /*READING AND PARSING THE INPUT */

    /*ForLoop Initializes: 
     * 1)rv: we can read from stdin. If read 0 bytes, stop
     * 2)cursor=cmd: position in memory
     * 3) lastCursor: ending bound in memory 
     * 4) last_char = 1; Runs first time
     */

    /* ForLoop conditions: 
     * 1) rv : 
     * 2) count : cannot execute more than MAX_INPUT times
     * 3) last_char != \n : stops reading when press enter.
     */

     /* Continue conditions: Handle at tail end.
     */

    for(rv = 1, count = 0, 
	  cursor=cmd, lastCursor=cursor,last_char = 1;
	  rv && (++count < (MAX_INPUT-1))&& (last_char != '\n') ; ) {

	  /* Read the value */
      rv = read(0, &buffer, 1);

      last_char = buffer; // hold the last_char for evaluation. 
      escapeSeen = false; //enable possible printing to screen
	  if (last_char == '"'){
        quote++;
      }
      /* Detect escape sequence keys like arrows */
      if(last_char=='\033'){
      	escapeSeen = true; // disable printing to terminal screen. Arrows invisible

      	/*Next arrow sequence*/
      	if((rv = read(0, &buffer, 1)>0) && buffer=='['){
      	  last_char = buffer;
 			
      	  /* Handle Directional Sequence */
		  if((rv = read(0, &buffer, 1)>0)){
            switch(buffer){
            	            /* KEY_UP */
             case 'A':
                       /* Cache the current work and move into history*/
                       if(inCommandlineCache && hasHistoryEntry()) {
                       	 inCommandlineCache=false; // set flag
                         *lastCursor = '\0'; //terminate the current string
                       	 storeCommandLineCache(cmd);                       	 

                       	 /*Set to historic string's latest entry */
                       	 if(historyCommand[(historyHead-1)%historySize]!=NULL){
                       	   strcpy(cmd,historyCommand[(historyHead-1)%historySize]);
                       	   lastVisitedHistoryIndex_UP =(historyHead-1)%historySize;
                       	   rewrite =1;
                       	 }

                       }else if(inCommandlineCache && !hasHistoryEntry()){
                       	 //do nothing
                       	 rewrite =0;
                       }else{
                         /*Copy Over the string from history */
                       	 int previousHistoryIndex = moveBackwardInHistory();
                       	 if(previousHistoryIndex!=lastVisitedHistoryIndex_UP){
                       	    strcpy(cmd,historyCommand[previousHistoryIndex]);
                       	    lastVisitedHistoryIndex_UP=previousHistoryIndex;
                       	    rewrite=1;
                       	 }

                       }

                       if(rewrite){
                       	 /*Test printf("rewriting UP previousHistoryIndex =%d\n",lastVisitedHistoryIndex_UP);*/
                       	 rewrite =0;
                         /*Delete the printed chars previously in memory, from its displayed 
                         position on screen */
                          write(1, loadCursorPosAscii, strlen(loadCursorPosAscii));
                          write(1, eraseLineAscii, strlen(eraseLineAscii));
                         /*
                         while(lastCursor>cmd){
                           write(1,deleteAscii,strlen(deleteAscii));
                           lastCursor--;
                         }*/  
                         /*Show the cmd contents to the terminal, and set pointers*/
                         cursor = cmd;
                         while(cursor!=NULL && *cursor!='\0'){
                           write(1,cursor,1);
                           cursor++;
                         }
                         lastCursor=cursor;
                       }
                  break; 
                                 /*DOWN KEY */
             case 'B': 
                       /* Enter cached command line*/
                       if(hasHistoryEntry() && (current==(historyHead-1))&& !inCommandlineCache){
                       	 inCommandlineCache=true;

                         if(temporaryCommandlineCachePtr!=NULL){
                           strcpy(cmd,temporaryCommandlineCachePtr);
                           rewrite=1;
                         }

                         /*Test: indicator of entering cache 
                         printf("entered the cache\n");fflush(stdout);*/

                       }else if(inCommandlineCache){
                       	  //do nothing
                       	  rewrite =0;
                       }else{ /* Must be within the history */

                       	 /* Get the next String in history*/
                       	 int nextStringIndex = moveForwardInHistory();
                       	 if(nextStringIndex!=lastVisitedHistoryIndex_DOWN){
					     	strcpy(cmd,historyCommand[nextStringIndex]);
					     	lastVisitedHistoryIndex_DOWN = nextStringIndex;
					     	rewrite =1;
                       	 }
                       } 
                       /*Delete the printed chars previously in memory, from its displayed 
                         position on screen */
                       if(rewrite){
                       	 /*Test: printf("rewriting\n");*/
                       	 rewrite=0;
                         write(1, loadCursorPosAscii, strlen(loadCursorPosAscii));
                         write(1, eraseLineAscii, strlen(eraseLineAscii));

                         /*Show the cmd contents to the terminal, and set pointers*/
                         cursor = cmd;
                         while(cursor!=NULL && *cursor!='\0'){
                           write(1,cursor,1);
                           cursor++;
                         }
                         lastCursor=cursor;
					   }
                       break;
                              /*KEY RIGHT*/
             case 'C': 
                       if(((cursor - cmd) < (MAX_INPUT-2))&& (cursor<lastCursor)){
                       	 write(1,moveRightAscii,strlen(moveRightAscii));
                       	 cursor++;
                       } 
                       break;
                            /*KEY LEFT */
             case'D': 
                       if(cursor - cmd >0){
                       	 write(1,moveLeftAscii,strlen(moveLeftAscii));
                         cursor--;
                       } 
                       break;
            }
		  }
		}
      }
      else if (last_char==8||last_char==127){ /* A delete was seen */
        /*char* swapper1;
      	char* swapper2;*/

      	//PRECONDITION: Deleting a space within the cmd array
        if(cursor-cmd>0){
          /*Failed to store necessary bytes */
          if(!storeBytesBetweenPointersIntoGlobalArray(cursor,lastCursor)){
            continue;
          }

      	  //DELETE PREVIOUS CHAR ON SCREEN, PRINT REST OF CMD LINE
          deleteCharAtCursor();
          clearLineAfterCursor();
          write(1,restOfCMDLine,strlen(restOfCMDLine));
          moveCursorBack(strlen(restOfCMDLine));

          if(cursor>cmd){
            cursor--;
          }
          strcpy(cursor,restOfCMDLine);
		  
		  if(lastCursor>cmd){
            lastCursor--;
    	  }
          *lastCursor='\0';

        }
      }
      /* Detect CTRL-C */
      else if(last_char == 3) {
        write(1, "^c", 2);
      }else if(last_char=='\n'){
      	char * newline= "\n";
        write(1, newline, 1);
      }else {
      	/* If not an escape character, print the byte and reflect change in memory */
      	if(!escapeSeen &&(cursor==lastCursor)){
      	  write(1, &last_char, 1);
      	  *cursor = last_char;
      	  cursor++;
          lastCursor++;
		  *lastCursor='\0';
      	}

		/* Same as above, except must shift all elements over to reflect insertion*/
		else if(!escapeSeen && cursor<lastCursor){
		  /* Check successful storage of rest of cmd data */
		  if(!storeBytesBetweenPointersIntoGlobalArray(cursor,lastCursor)){
		    continue;
		  }
		  write(1, &last_char, 1);
		  *cursor = last_char;
		  cursor++;

		  /*Get rest of command line, write to screen, copy to cursorposition*/
		  char* restOfCMDLinePtr=restOfCMDLine;
		  write(1,restOfCMDLinePtr,strlen(restOfCMDLinePtr));
		  strcpy(cursor,restOfCMDLinePtr);

		  int i=0;
          while(i<strlen(restOfCMDLine)){
            write(1,moveLeftAscii,strlen(moveLeftAscii));
            i++;
          }

		  /*Increase size by 1, due to insertion */
		  lastCursor++;
		  *lastCursor='\0';

		  /*
		  relayer1 = cursor;
		  relayer2= cursor+1;
		  *tempByte = *relayer2;
		  while(relayer2 < lastCursor){
		  	*relayer2 = *relayer1;
		    write(1, relayer2, 1);
		  	relayer1++;
		  	relayer2++;
		  	*tempByte = *relayer2;
		  }
		  *relayer2 = *tempByte;
		  write(1, relayer2, strlen(relayer2));
		  *cursor = buffer;*/
		}else{ 

		   /* An escape sequence key pressed. */

          //*Show no changes to screen or memory. Do nothing. 

		}
      }
      if ((quote%2 != 0) && (last_char == '\n')){
        write(1, ">", 1);
        last_char = 1;
      }
    } 
    if (!rv) { 
      finished = 1;
      break;
    }
    // Execute the command, handling built-in commands separately 
    // Just echo the command line for now

    //Upon evaluation, we are back in the commandlineCache. Necessary to keep track of history cycling.
    inCommandlineCache=true; 
    char* argArray[MAX_INPUT]; /* Array of arguments */
    char ** parsedArgArray = parseCommandLine(cmd, argArray); /* Fill up array of arguments */

    if (debug){
      write(2, "RUNNING: ", 9);
      write(2, cmd, strlen(cmd));
      error = 0;
    }
    //checkRedirection(parsedArgArray);
    if(executeArgArray(parsedArgArray,envp)==0) continue;
    
    if (debug){
      write(2, "ENDED: ", 7);
      write(2, cmd, strlen(cmd));
      write(2, " (ret=", 6);
      if (error)
        write(2, "1)\n", 3);
      else 
        write(2, "0)\n", 3);
    }
  }
  return 0;
}

