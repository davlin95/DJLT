#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <termios.h>
#include <sys/wait.h>
#include <sys/stat.h>
#include <stdbool.h>
#include <curses.h>
#include "shellHeader.h"

/*Global Error Var */
int error = 0;

/*Strings for help menu: WARNING!!! LAST ELEMENT MUST BE NULL, for the printHelpMenu() function */
char * helpStrings[]={"exit [n]","pwd -[LP]","set [-abefhkmnptuvxBCHP] [-o option->",
"ls [-l]","cd [-L|[-P [-e]] [-@]] [dir] ", "echo [-neE] [arg ...]","ps [n]","printenv [n]",NULL};

/* Make global array of built-in strings */
char * globalBuiltInCommand[] = {"ls","cd","pwd","help","echo","set","ps","printenv","exit"};
char temporaryCommandlineCache[MAX_INPUT];
char *temporaryCommandlineCachePtr;
bool inCommandlineCache;

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
 * A function that turns relative to absolute path. Evaluation. 
 * @param cmdString: string to be parsed. 
 * @param buffer: buffer to be returned. contains the evaluated path. 
 * @param buffer size: size of the buffer, safety. 
 * @return char*: filled buffer. 
 */
char* turnRelativeToAbsolute(char* cmdString,char* buffer, int bufferSize){
  int pathSize = strlen(getenv("PWD"));
  char buildingPath[pathSize+1]; // add one for null byte
  strcpy(buildingPath,getenv("PWD"));
  char *buildingPathPtr = buildingPath;

  /*Test print
  ("\nbuildingPath is: %s\n",buildingPath);
  char buff[strlen(buildingPath)];
  printf("\nparentPath is: %s\n",parentDirectory(buildingPath,buff));
  printf("command is: %s\n",command);
  fflush(stdout); */

  /*Find the traversal path */
  char * traversal[MAX_INPUT];
  parseByDelimiter(traversal,cmdString,"/ ");

  /*Test Print the array we just filled 
  int i =0;
  while(traversal[i] != NULL&& traversal[i] !='\0'){
    printf("traversal is: %s\n",traversal[i]);
    i++;
  }
  fflush(stdout);*/

  /* Build the path to the file */
  int count =0;
  //int childSize=0;
  int result=0; 
  //printf("travesal is %s\n", *traversal);
  while(traversal[count] != NULL && traversal[count] !='\0'){
  	if( (result = strcmp(traversal[count],".")) ==0){ 

  	}else if((result = strcmp(traversal[count],"..")) ==0){ /* Move up one parent*/
      parentDirectory(buildingPathPtr);
      printf("\n moved to parent path: %s\n",buildingPathPtr);
      fflush(stdout);
  	}else{  
  	  /*Append */
      directoryAppendChild(buildingPathPtr,traversal[count]);
      printf("\nappended child, buildingPathPtr is: %s\n",buildingPathPtr);
      fflush(stdout);
  	}
    count++; /* Move onto next traversal string */
  }

  /*Test print result
  printf("built finally: %s\n",buildingPathPtr);
  fflush(stdout);*/

  if(bufferSize>0)
    return strncpy(buffer,buildingPathPtr,bufferSize-1);

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
  strcpy(cmdCopy,cmd);

  /* Quote Parsing */
  char *firstQuote;
  char *nextQuote;
  char *lastQuote;

  if ((firstQuote = strchr(cmdCopy, '"')) != NULL){
    lastQuote = strrchr(cmdCopy, '"');
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
  token = strtok_r(cmdCopy," \n",&savePtr);
  while(token != NULL){
    argArray[argCount]= token;
    argCount++;
    token = strtok_r(NULL," \n",&savePtr);
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
  char *slash = "/";
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
    strcat(strcpy(src, pathArray[i]), slash);
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
  int badDirectory = 0;
  if (argArray != NULL && argArray[strlen(argArray)-1] == '/'){
    argArray[strlen(argArray)-1] = '\0';
  }
  if(argArray == NULL){
    setenv("OLDPWD", getenv("PWD"), 1);
    setenv("PWD", getenv("HOME"), 1);
    chdir(getenv("PWD"));
  }
  else if(strcmp(argArray,"-")==0){ 
    char temp[strlen(getenv("OLDPWD"))];
    strcpy(temp,getenv("OLDPWD"));
    setenv("OLDPWD", getenv("PWD"), 1);
    setenv("PWD", temp, 1);
    chdir(getenv("PWD")); 
  }else if(argArray[0] == '.'){
    if (argArray[1] == '\0')
      setenv("OLDPWD", getenv("PWD"), 1);
    else if(argArray[1] == '/'){
      char *argPtr = &argArray[1];
      char path[strlen(getenv("PWD")) + strlen(argArray) - 1];
      strcat(strcpy(path, getenv("PWD")), argPtr);
      struct stat pathStat;
      if (stat(path, &pathStat) >= 0){
        setenv("OLDPWD", getenv("PWD"), 1);
        setenv("PWD", path, 1); 
        chdir(getenv("PWD"));
      } else 
        badDirectory = 1;
    }else if(argArray[1] == '.'){
      char path[strlen(getenv("PWD")) + strlen(argArray) - 2];
      strcpy(path, getenv("PWD"));
      if (strcmp(getenv("HOME"), getenv("PWD")) != 0){
        char *lastSlash = strrchr(path, '/');
        *lastSlash = '\0';
      }
      if (argArray[2] == '\0'){
        setenv("OLDPWD", getenv("PWD"), 1);
        setenv("PWD", path, 1); 
        chdir(getenv("PWD"));
      }else if (argArray[2] == '/'){
        char *argPtr = &argArray[2];
        strcat(path, argPtr);
        struct stat pathStat;
        if (stat(path, &pathStat) >= 0){
          setenv("OLDPWD", getenv("PWD"), 1);
          setenv("PWD", path, 1); 
          chdir(getenv("PWD"));
        }else 
        badDirectory = 1;
      }else 
        badDirectory = 1;
    }else 
        badDirectory = 1;
    }else if(argArray[0] == '\0'){
          argArray[0] = '/';
          badDirectory = 1;
    }  
    else {
      struct stat pathStat;
      char path[strlen(getenv("PWD")) + strlen(argArray) + 1];
      strcpy(path, getenv("PWD"));
      strcat((strcat(path, "/")), argArray);
      if (stat(path, &pathStat) >= 0){
          setenv("OLDPWD", getenv("PWD"), 1);
          setenv("PWD", path, 1); 
          chdir(getenv("PWD"));
      }
      else 
        badDirectory = 1;
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
  char buf[MAX_INPUT];
  path = getcwd(buf, MAX_INPUT);
  write(1, path, strlen(path));
  write(1, "\n", 1);
  printf("pid is %d\n", getpid());
  fflush(stdout);
}
void processSet(char argArray[]){
  char * token;
  char * token2;
  char *equals = "=";
  token = strtok(argArray, equals);
  token2 = strtok(NULL, equals);
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
  saveHistoryToDisk();
  exit(0);
}

bool executeArgArray(char * argArray[], char * environ[]){
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
    }else if(strcmp(command,"exit")==0){
      processExit();
    }else{
      /* Test Print: Use statfind() to launch the program from shell 
      printf("\nuse statfind() to launch the built in\n");
      printf("%s\n",command);
      fflush(stdout); */

      /*Find and execute the binary path */
      char * fullCommandPath; 
      fullCommandPath = statFind(argArray[0]);
      if(fullCommandPath!=NULL){
        createNewChildProcess(fullCommandPath,argArray);
      }else { /*Not found */
      	write(1,argArray[0],strlen(argArray[0]));
        printError(command);
        error = 1;
      }
    }
  }else{ /* Is an object file such as a.out */
    /*Make buffer */
    char buffer[MAX_INPUT];

    /*Parse into absolute path */
    if(turnRelativeToAbsolute(argArray[0],buffer,MAX_INPUT)!=NULL){
      printf("\nValue of buffer is: %s\n",buffer);
      if(statExists(buffer)){
    	printf("dir exists: %s\n",buffer);
    	createNewChildProcess(buffer,argArray);
      }
      else
        error = 1;
    }else{
      fprintf(stderr,"Error in turnRelativeToAbsolute for %s",argArray[0]);
    }
  }
  return true;
}

void createNewChildProcess(char* objectFilePath,char** argArray){
  pid_t pid;
  int status;
  if((pid=fork())==0){
    printf("child's parent pid is %d\n", getppid());
    int ex = execvp(objectFilePath,argArray);
    if(ex){      
      printError(objectFilePath);
    }
    exit(0);
  }else{
      wait(&status);
      if (status)
        error = 1;
      printf("\nParent Process reaped child process %d\n",pid);
      fflush(stdout);
  }
}

void  initializeHistory(){
  /*Initialize buffers for the directory to store history */
  int homePathSize = strlen(getenv("HOME"));
  char * historyFileExtension = "/.320sh_history\0";
  int historyFileExtensionSize = strlen(historyFileExtension);
  char historyPath[homePathSize + historyFileExtensionSize+1];

  /* Build the directory history */
  strcpy(historyPath, getenv("HOME"));
  strcat(historyPath,historyFileExtension);

  /* Start reading the history file */
  FILE *historyFile;
  if ((historyFile = fopen(historyPath, "r"))){
    char line[MAX_INPUT];
    while(fgets(line, sizeof(line), historyFile)){
      if (line != NULL) storeHistory(line);
      printf("\nLoading string : %s", line);
    }
    fclose(historyFile);

  }else{
  	/* Print to stdout */
    printError("Unable to initialize prompt history");
  }
  printHistoryCommand();
}


void saveHistoryToDisk(){
  /*Initialize buffers for the directory to store history */
  int homePathSize = strlen(getenv("HOME"));
  char * historyFileExtension = "/.320sh_history\0";
  int historyFileExtensionSize = strlen(historyFileExtension);
  char* historyPath= malloc((homePathSize + historyFileExtensionSize+1)*sizeof(char));

  /*Build the path */
  strcat(historyPath,getenv("HOME"));
  strcat(historyPath,historyFileExtension);
  printf("HOME IS: %s", getenv("HOME"));

  /* Start writing the history file */
  FILE *historyFile;
  historyFile = fopen(historyPath,"w");
  free(historyPath);
  /*Tests: */
  printf("\n%s\n",historyPath);
  printf("\nhistoryFILE NULL?%d\n",(historyFile==NULL));

  /* Move to earliest point in recent history*/
  int i,currentHistoryIndex,nextHistoryIndex;
  for(i=0; i<historySize;i++)
    currentHistoryIndex = moveBackwardInHistory();
  
   /* IMPORTANT: MUST BE '=', not double equals, "==". Is an assignment */
  while( (nextHistoryIndex=moveForwardInHistory()) 
    && (nextHistoryIndex != currentHistoryIndex) ){ // while there's more strings to store
  	  // print the current history string to file
  	  if( historyCommand[currentHistoryIndex] !=NULL){
		fprintf(historyFile, "%s\n", historyCommand[currentHistoryIndex]); 
        printf("\nStoring %s",historyCommand[currentHistoryIndex]);
  	  }
      currentHistoryIndex++;
  }
  /* No more strings left in forwardInHistory, one more time to store the final string */
  if( historyCommand[currentHistoryIndex] !=NULL){
    fprintf(historyFile,"%s\n", historyCommand[currentHistoryIndex]); 
    printf("Storing: %s\n", historyCommand[currentHistoryIndex]); 
  }
  /*Close file */
  fclose(historyFile);
}

int main (int argc, char ** argv, char **envp) {
  /*debug flag*/
  int debug = 0;
  if (argv[1] != NULL){
    if (strcmp(argv[1], "-d") == 0)
      debug = 1;
  }

  int quote = 0;
  int finished = 0;
  char *prompt = "320sh> ";
  char cmd[MAX_INPUT];
  bool escapeSeen = false;
  bool inCommandlineCache=true;

  initializeHistory();
  while (!finished) {

    char *cursor;
    char *lastCursor;
    char buffer; 
    char last_char;
    int rv;
    int count;

    // Print the current directory 
    write(1, "\n[", 2);
    write(1, getenv("PWD"), strlen(getenv("PWD")));
    write(1, "] ", 2);

    // Print the shell prompt
    rv = write(1, prompt, strlen(prompt));
    if (!rv) { 
      finished = 1;
      break;
    }
    // read and parse the input
    for(rv = 1, count = 0, 
	  cursor=cmd, lastCursor=cursor,last_char = 1;
	  rv && (++count < (MAX_INPUT-1))&& (last_char != '\n') ; ) {
	  /* Read the value */
      rv = read(0, &buffer, 1);
    /*encounter a quotation in terminal command*/
      if (buffer == '"')
        quote++;

      last_char = buffer;
      
      escapeSeen = false; //enable printing to screen 

      /* Detect escape sequence keys like arrows */
      if(last_char=='\033'){
      	escapeSeen = true; // disable printing to screen for terminal commands
      	if((rv = read(0, &buffer, 1)>0) && buffer=='['){
      	  last_char = buffer;

      	  char * historicString; // Used later if reached inside switch statement.
		  if((rv = read(0, &buffer, 1)>0)){
            switch(buffer){
             case 'A':
                       /* To Store cache or not*/
                       if(inCommandlineCache && hasHistoryEntry()) {
                       	 inCommandlineCache=false;
                         *lastCursor = '\0'; //terminate the current string
                       	 storeCommandLineCache(cmd);
                       	 char historyHeadBuffer[MAX_INPUT];
                       	 strcpy(historyHeadBuffer,historyCommand[(historyHead-1)%historySize]);
                         historicString = historyHeadBuffer;
                       }else if(inCommandlineCache && !hasHistoryEntry() ){
                         historicString=NULL; // set to NULL to do nothing;
                       }else{
                         historicString = historyCommand[moveBackwardInHistory()];
                       } 

                       /*Test Print out contents of historyCommand array 
             	       printf("\nhistoric string is: %s",historicString);fflush(stdout);
             	       printf("\tcurrent is: %d",current);fflush(stdout);*/

             	       /*Update the cmd line*/
             	       if(historicString!=NULL)
                         strcpy(cmd,historicString);
                       else
                       	 *cmd = '\0';
                       
                       /*Clear the previous args, before printing historic string */
                       while(lastCursor>cmd){
                         write(1,deleteAscii,strlen(deleteAscii));
                         lastCursor--;
                       }
                       cursor = cmd;
                       cursor+= (strlen(cmd)); // incremented at end of loop to right place
                       lastCursor=cursor;
                       write(1,cmd,strlen(cmd));
                  break; 
             case 'B': 
                       /* Choose cache or actual history storage*/
                       if((current==(historyHead-1))&& !inCommandlineCache){
                       	 inCommandlineCache=true;
                         historicString = temporaryCommandlineCachePtr;

                         /*Test: indicator of entering cache 
                         printf("entered the cache\n");fflush(stdout);*/

                       }else if((current==(historyHead-1))&& inCommandlineCache){
                         historicString = temporaryCommandlineCachePtr;
                       }else{
					     historicString = historyCommand[moveForwardInHistory()];
                       } 

                  	   /*Test Print out contents of historyCommand array 
                       printf("\nhistoric string is: %s",historicString);fflush(stdout);
             	       printf("\tcurrent is: %d",current);fflush(stdout);*/

             	       if(historicString!=NULL)
                         strcpy(cmd,historicString);
                       else *cmd = '\0';

                       /*Clear the previous args, before printing historic string */
                       while(lastCursor>cmd){
                         write(1,deleteAscii,strlen(deleteAscii));
                         lastCursor--;
                       }

                       cursor = cmd;
                       cursor+= (strlen(cmd)); // incremented at end of loop to write place
                       lastCursor=cursor;
                       write(1,cmd,strlen(cmd));
                       
                  break;
             case 'C': 
                       if(((cursor - cmd) < (MAX_INPUT-2))&& (cursor<lastCursor)){
                       	 write(1,moveRightAscii,strlen(moveRightAscii));
                       	 cursor++;
                       } 
                  break;
             case'D': 
                       if(cursor - cmd >0){
                       	 write(1,moveLeftAscii,strlen(moveLeftAscii));
                         cursor--;
                       } 
                      // DO SOMETHING with HISTORY
                  break;
            }
		  }
		}
      }
      else if (last_char==8||last_char==127){
        char* swapper1;
      	char* swapper2;

      	//PRECONDITION: 
        if(cursor-cmd>0){

      	  //MOVE BACK THE CURSOR AND SAVE SPOT
          write(1,backspaceAscii,strlen(backspaceAscii));
          write(1,saveCursorPosAscii,strlen(saveCursorPosAscii));

          /*CHANGES TO THE DATA IN CMD*/

		  //Initialize cursor positions
          cursor--; 
          swapper1 = cursor;
          swapper2 = cursor+1;

          /* Iterate and make changes*/
          while(swapper2!=NULL && *swapper2!='\0'){ //while the right-hand pos has a significant byte
            // swap with left pos
            *swapper1 = *swapper2; 
            write(1,swapper2,1); // display on console too

            //increment data ptr and move cursor right on console
            swapper1++;
            swapper2++;
          }        
          /* null terminate the new last space in memory, and delete last char on console*/
          *lastCursor='\0';

          if(lastCursor>cmd)
            lastCursor--;
          *lastCursor= '\0';
          write(1,moveRightAscii,strlen(moveRightAscii));
          write(1,deleteAscii,strlen(deleteAscii));

          /* reload the previous position of cursor*/
          write(1,loadCursorPosAscii,strlen(loadCursorPosAscii));
        }
      }
      /* Detect CTRL-C */
      else if(last_char == 3) {
        write(1, "^c", 2);
      }else if(last_char=='\n'){
        
      }else {
      	/* Display the last seen char to terminal */
      	if(!escapeSeen)
		      write(1, &last_char, 1);
		/* Transfer the value into cmd pointed to by cursor */
        *cursor = buffer;
        lastCursor+=2;
		*lastCursor='\0';
		lastCursor--; // becomes cursor+1
		cursor++;

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

    char* argArray[MAX_INPUT]; /* Array of arguments */
    char ** parsedArgArray = parseCommandLine(cmd, argArray); /* Fill up array of arguments */
    if (debug){
      char *cmdEnd = strrchr(cmd, '\n');
        *cmdEnd = '\0';
      write(2, "RUNNING: ", 9);
      write(2, cmd, strlen(cmd));
      error = 0;
    }
    if(executeArgArray(parsedArgArray,envp)==0) continue;

    /*Test: show historyCommand 
    printHistoryCommand();*/
    
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
  /*canonicalSettings();*/
  return 0;
}

