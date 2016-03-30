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


/* Key definitions and Canonicals */
struct termios termios_set;
struct termios termios_prev;
int error = 0;
/*Strings for help menu: WARNING!!! LAST ELEMENT MUST BE NULL, for the printHelpMenu() function */
char * helpStrings[]={"exit [n]","pwd -[LP]","set [-abefhkmnptuvxBCHP] [-o option->",
"ls [-l]","cd [-L|[-P [-e]] [-@]] [dir] ", "echo [-neE] [arg ...]","ps [n]","printenv [n]",NULL};


/* Make global array of built-in strings */
char * globalBuiltInCommand[] = {"ls","cd","pwd","help","echo","set","ps","printenv","exit"};

/*Helper Function Key-Value Strings */
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

var* createNode(char* key, char* value){
  static var node;
  node.key = key;
  node.value = value;
  return &node;
}

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

/*Stores the history of commands */
int historySize = 50;
char* historyCommand[50];
int historyHead = 0;
int current=0;

/* History functions*/
void storeHistory(char * string){
  int headIndex = historyHead % historySize; /*Valid index within the array */
  
  /* Free the previous string */
  if(historyCommand[headIndex]!=NULL){
  	free(historyCommand[headIndex]);
  }
  /* Allocate for new string */
  char* newString = malloc((strlen(string)+1)*sizeof(char));
  strcpy(newString,string);
  historyCommand[headIndex]=newString;

  /* Reset counters*/
  historyHead++; /*Next available space to store next string */
  current=(historyHead-1); /*Index of most current String */
}

int moveForwardInHistory(){
  if( (current<historyHead) && 
  	(( (current+1)%historySize) != (historyHead%historySize))) {
  	current++;
  	return (current%historySize);
  }
  return(current%historySize); /* Can't move forward, don't increment */
}

int moveBackwardInHistory(){
	if((current<historyHead)
	  &&(current>0)
	  &&(((current-1)%historySize) != ((historyHead-1)%historySize))){ /*Not overwriting the most current cmd string*/
		current--;
		return(current%historySize);
	}
	return(current%historySize);
}


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

int alphaNumerical(char *argString){
  int i;
  char* position = argString;
  for (i = 0; i < strlen(argString); i++){
    if ((*position > 47 && *position < 58) || (*position > 64 && *position < 91) || (*position > 96 && *position < 123)){
      position++;
    }
  }
  if (position == argString + strlen(argString))
    return i;
  return 0;
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
  token = strtok_r(cmdCopy," \n",&savePtr);
  while(token != NULL){
  	/*Test: Print out token 
    write(1,token,strlen(token));
    write(1,"-->",3); */

    /* Fill up argArray with split strings */
    /*token = strcat(token, "\0"); */   /*might already be terminated with \0 */
    argArray[argCount]= token;
    argCount++;
    token = strtok_r(NULL," \n",&savePtr);
  }
  argArray[argCount]=NULL; /* Null terminate */

  /* Parsed a non-empty string, store in history*/
  if(argCount>1){
    storeHistory(cmd);
  }

  /*Test: Print out contents of argArray 
  write(1,"\nfinished\n\0",11);
  int i;
  for(i = 0; i < argCount; i++){
  	char * argString = argArray[i];
  	write(1,"\n",1);
  	write(1,argArray[i],strlen(argString));
  	printf("\nisBuiltin Result: %d",isBuiltIn(argString));
  	fflush(stdout);
  }
  printf("\nArgcount is %d\n",argCount);
  printf("last item is %s\n", argArray[argCount-1]);
  fflush(stdout);*/

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
          write(1, "cd: ", 4);
          write(1, argArray, strlen(argArray));
          write(1, ": No such file or directory\n", 28);
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
  }
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
    char historyPath[strlen(getenv("HOME")) + 15];
    strcat((strcpy(historyPath, getenv("HOME"))), "/.320sh_history");
    FILE *historyFile;
    historyFile = fopen(historyPath, "w");
    if (historyCommand[historyHead] == NULL){
      int position;
      for (position = 0; position < historyHead; position++){
        fprintf(historyFile, "%s", historyCommand[position]);
      }
    } else {
      int headPosition = historyHead;
        fprintf(historyFile, "%s", historyCommand[historyHead]);
        historyHead++;
        if (historyHead == 50)
            historyHead = 0;
        while (historyHead != headPosition){
          fprintf(historyFile, "%s", historyCommand[historyHead]);
          historyHead++;
          if (historyHead == 50)
            historyHead = 0;
        }
    }
  fclose(historyFile);
  exit(0);
}


void executeArgArray(char * argArray[], char * environ[]){
  int count;
  for(count=0; argArray[count]!=NULL;++count)
  	;

  /*Test: Print count */
  printf("\n value of count of argArray in executeArgArray is: %d\n", count);
  for(int i=0;i<count;i++){
  	printf("arg[%d] is %s\n", i,argArray[i]);
  }
  fflush(stdout);
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
  char historyPath[strlen(getenv("HOME")) + 15];
  strcat((strcpy(historyPath, getenv("HOME"))), "/.320sh_history");
  FILE *historyFile;
  if ((historyFile = fopen(historyPath, "r"))){
    char line[MAX_INPUT];
    while (fgets(line, sizeof(line), historyFile)){
      if (line != NULL)
      storeHistory(line);
    }
  } else {
      historyFile = fopen(historyPath, "w");
    }
  fclose(historyFile);
}
int main (int argc, char ** argv, char **envp) {
  int finished = 0;
  char *prompt = "320sh> ";
  char cmd[MAX_INPUT];
  bool escapeSeen = false;
  initializeHistory();
  while (!finished) {

    char *cursor;
    char buffer; 
    char last_char;
    int rv;
    int count;

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

    // read and parse the input
    for(rv = 1, count = 0, 
	  cursor = cmd, last_char = 1;
	  rv && (++count < (MAX_INPUT-1))&& (last_char != '\n') ; ) {
	  /* Read the value */
      rv = read(0, &buffer, 1);
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
                       historicString = historyCommand[moveBackwardInHistory()];
                       if(historicString!=NULL){
                         /*Test Print out contents of historyCommand array 
             	         printf("\nhistoric string is: %s",historicString);fflush(stdout);
             	         printf("\tcurrent is: %d",current);fflush(stdout);
             	         int i=0;
             	         while(historyCommand[i]!=NULL){
             	           printf("\nhistoric string at index %d: %s\n",i,historyCommand[i]);fflush(stdout);
             	           i++;
             	         }*/
                         strcpy(cmd,historicString);
                         printf("\n%s\n",cmd);
                         cursor = cmd;
                       }else printf("\nError historic string shouldn't be null");
                       // DO SOMETHING with HISTORY
                  break; 
             case 'B': 
                       historicString = historyCommand[moveForwardInHistory()];
                       if(historicString!=NULL){
                  	     /*Test Print out contents of historyCommand array 
                         printf("\nhistoric string is: %s",historicString);fflush(stdout);
             	         printf("\tcurrent is: %d",current);fflush(stdout);
             	         int i=0;
             	         while(historyCommand[i]!=NULL){
             	           printf("\nhistoric string at index %d: %s\n",i,historyCommand[i]);fflush(stdout);
             	           i++;
             	         }*/
                         strcpy(cmd,historicString);
                         cursor = cmd;
                       }else printf("\nError historic string shouldn't be null");
             		   // DO SOMETHING with HISTORY
                  break;
             case 'C': write(1,moveRightAscii,strlen(moveRightAscii));
                       if((cursor - cmd) < (MAX_INPUT-2)) cursor++;
                       // DO SOMETHING with HISTORY
                  break;
             case'D': write(1,moveLeftAscii,strlen(moveLeftAscii));
                       if(cursor - cmd >0) cursor--;
                      // DO SOMETHING with HISTORY
                  break;
            }
		      }
		    }
      }
      else if (last_char==8||last_char==127){
        write(1,backspaceAscii,strlen(backspaceAscii));
        write(1,deleteAscii,strlen(deleteAscii));
      }
      /* Detect CTRL-C */
      else if(last_char == 3) {
        write(1, "^c", 2);
      }else {
      	/* Display the last seen char to terminal */
      	if(!escapeSeen)
		  write(1, &last_char, 1);
		/* Transfer the value into cmd pointed to by cursor */
        *cursor = buffer;
		cursor++;
      }
    } 
    *cursor = '\0';
    if (!rv) { 
      finished = 1;
      break;
    }
    // Execute the command, handling built-in commands separately 
    // Just echo the command line for now
    // write(1, cmd, strnlen(cmd, MAX_INPUT));
    char* argArray[MAX_INPUT]; /* Array of arguments */
    char ** parsedArgArray = parseCommandLine(cmd, argArray); /* Fill up array of arguments */
    executeArgArray(parsedArgArray,envp);
  }
  /*canonicalSettings();*/
  return 0;
}

