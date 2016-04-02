#include "shellHeader.h"

/*****************/
/*   Globals  ***/
/***************/
/*
 * Initialize Global variables for the history 
 */
int historySize = 50;
char* historyCommand[50];
int historyHead = 0;
int current=0;

          
/* Terminal Variables */
static Job* jobHead=NULL;
pid_t foreground=-1;


				/***********************************/
                /*          Tests                 */
                /**********************************/
void testJobNode(){
  Job* jobNode1 = createJobNode(1,2,"node1\0",0,1);
  Job* jobNode2 = createJobNode(2,3,"node2\0",0,2);
  Job* jobNode3 = createJobNode(3,3,"node3\0",0,0);

  jobNode1->next = jobNode2;
  jobNode1->prev = NULL;
  jobNode2->prev = jobNode1;
  jobNode2->next = jobNode3;
  jobNode3->next = NULL;
  jobNode3->prev = jobNode2;

  jobHead = jobNode1;
  printJobListWithHandling();
  printJobListWithHandling();
}


                 /**************************************/
                  /*        MAIN PROGRAM               */
				 /**************************************/

/*Global Error Var */
int error = 0;

/*Strings for help menu: WARNING!!! LAST ELEMENT MUST BE NULL, for the printHelpMenu() function */
char * helpStrings[]={"exit [n]","pwd -[LP]","set [-abefhkmnptuvxBCHP] [-o option->",
"ls [-l]","cd [-L|[-P [-e]] [-@]] [dir] ", "echo [-neE] [arg ...]","ps [n]","printenv [n]",NULL};

/* Make global array of built-in strings */
char * globalBuiltInCommand[] = {"ls","cd","pwd","help","echo","set","ps","printenv","exit","jobs"};
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
  free(testBuff); */

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
  int result=0; 
  //printf("travesal is %s\n", *traversal);
  while(traversal[count] != NULL && traversal[count] !='\0'){
  	if( (result = strcmp(traversal[count],".")) ==0){ 

  	}else if((result = strcmp(traversal[count],"..")) ==0){ /* Move up one parent*/
      parentDirectory(buildingPath);
      //printf("\n moved to parent path: %s\n",buildingPath);
      fflush(stdout);
  	}else{  
  	  /*Append */
      directoryAppendChild(buildingPath,traversal[count]);
      //printf("\nappended child, buildingPathPtr is: %s\n",buildingPath);
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
  if(!saveHistoryToDisk()){
  	printError("Error saving history to disk\n");
  }
  printHistoryCommand();
  exit(0);
}

bool executeArgArray(char * argArray[], char * environ[]){
  int count;
  int fgState=0;

  /*Parse through and find count, exit if no args */
  for(count=0; argArray[count]!=NULL;++count);
  if(count==0) return false;

  /* Look for special '&' background process symbol */
  fgState = !checkForBackgroundSpecialChar(argArray,count);
  //printf("fgState is: %d ", fgState);

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
      printHelpMenu();
    }else if(strcmp(command,"exit")==0){
      processExit();
    }else if (strcmp(command,"jobs")==0){
      processJobs();
    }else if(strcmp(command,"fg")==0){
      processFG(argArray,count);
    }
    else{
      /* Test Print: Use statfind() to launch the program from shell 
      printf("\nuse statfind() to launch the built in\n");
      printf("%s\n",command);
      fflush(stdout); */

      /*Find and execute the binary path */
      char * fullCommandPath; 
      fullCommandPath = statFind(argArray[0]);
      if(fullCommandPath!=NULL && (!fgState)){
      	/*ALTERNATE
      	createNewChildProcess(fullCommandPath,argArray);*/
      	/*ALTERNATE:*/
        createBackgroundJob(fullCommandPath,argArray,0);
        //printf("\nfinished execution createBackgroundJob in executeArgArray\n");
        return false; //Continue loop upon exit
      }else if(fullCommandPath!=NULL && fgState ){
      	/*ALTERNATE 
      	createNewChildProcess(fullCommandPath,argArray); */
      	/*ALTERNATE:*/
		createBackgroundJob(fullCommandPath,argArray,1); 
        //printf("\nfinished execution createBackgroundJob in executeArgArray\n");
        return false;//continue loop upon exit
      }else { /*Not found */
      	write(1,argArray[0],strlen(argArray[0]));
        printError(command);
        error = 1;
      }
    }
  }else{ /* Is an object file such as a.out */
    /*Make buffer */
    int callocSize = MAX_INPUT + strlen(getenv("PWD")) + strlen("320sh> ") + 1;
    char* buffer= calloc(callocSize,1 );

    /*Parse into absolute path */
    if(turnRelativeToAbsolute(argArray[0],buffer,callocSize)!=NULL){
      //printf("\nValue of buffer is: %s\n",buffer);
      if(statExists(buffer)){
      	safeFreePtrNull(buffer);

      	/*ALTERNATE
      	createNewChildProcess(argArray[0],argArray);*/

      	/*ALTERNATE EXECUTION*/
    	//printf("dir exists: %s\n",buffer);
    	createBackgroundJob(buffer,argArray,0);
    	//printf("finished object execution createBackgroundJob\n");
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

void createNewChildProcess(char* objectFilePath,char** argArray){
  pid_t pid;
 // int status;

  if((pid=fork())==0){
    //printf("child's parent pid is %d\n", getppid());
    int ex = execvp(objectFilePath,argArray);
    if(ex){      
      printError(objectFilePath);
    }
    exit(0);
  }else{

  	  /*KIND OF WORKING 
  	  while((waitpid(-1,&status,WNOHANG))>0){
        printf("\nParent Process WNOHANG reaped child process %d\n",pid);
  	  }*/

       /*SLOW 
      wait(NULL); */

     /* if (status)
        error = 1; */
    /*
      printf("\nParent Process reaped child process %d\n",pid);
      fflush(stdout);*/
  }
}

/* 
 * An alternate execution method. Used for running background processes 
 */
void createBackgroundJob(char* newJob, char** argArray,bool setForeground){
    /*Assign handlers*/
  signal(SIGCHLD,killChildHandler);
  pid_t pid;

  /*PARENT PROCESS*/
  if((pid=fork())!=0){
  	/*Make the forked child part of this group*/
  	setpgid(pid,getpid());  
  	sleep(1);

  	//ADD JOB TO THE DATASTRUCTURE
  	/*Blocking*/
  	sigset_t block,unblock;
  	sigfillset(&block);
    sigprocmask(SIG_SETMASK,&block,&unblock);

    if(newJob!=NULL){
      char* newJobString = malloc( sizeof(char)*(strlen(newJob)+1))	;
      strcpy(newJobString,newJob);
      setJobNodeValues(pid,getpgid(pid),newJobString,0,2);
    }
  	if(setForeground){//set foreground
  	  foreground = pid;
  	}else{
	  //printJobNode(getJobNode(pid),-1);
  	  printJobListWithHandling();
  	}
  	//UNBLOCKING THE SIGNALS
  	sigprocmask(SIG_SETMASK,&unblock,NULL);
  }
  else{   /*CHILD*/

  	/*Test PID */
    printf("child's parent pid is %d\n", getppid());
    printf("Inside child, pid is %d\n", getpid()); 

    /* Execute and catch error */
    int ex = execvp(newJob,argArray);
    if(ex){      
      //printf("child process pid: %d not executing %s",pid, newJob);
    }
    kill(getppid(),SIGCHLD);
    exit(0);
  }	  
}

void killChildHandler(){
    pid_t pid=0;
    sigset_t block,unblock;

  /* Test Print Strings
   char* pidString="\nPid is: \0";
   char * str = "\nSignal!child process pid: finished executing\n\0";*/
 
    while((pid=waitpid(-1,NULL, WUNTRACED)>0) ){
    sigfillset(&block);
    sigprocmask(SIG_SETMASK,&block,&unblock);

    /*TEST PRINT RECEIPT SIGNAL */
    char reapString[MAX_INPUT]={'\0'};
    sprintf(reapString,"Reaped Child process: %d ...\n",pid);
    write(1,reapString,strlen(reapString)); 

    /*MODIFY THE DATASTRUCTURE*/
    Job* terminatedJobNode;
    terminatedJobNode = getJobNode(pid);
    if(terminatedJobNode!=NULL){
      terminatedJobNode->runStatus=0;
	  /*SHOW TO SCREEN*/
      printJobNode(terminatedJobNode,getPositionOfJobNode(terminatedJobNode));
    }

    /*UNBLOCK ALL SIGNALS*/
    sigprocmask(SIG_SETMASK,&unblock,NULL);
  }
}


int main (int argc, char ** argv, char **envp) {
  jobHead =NULL;
  //int firstTimeRun =1;

  /*debug flag*/
  int debug = 0;
  if (argv[1] != NULL){
    if (strcmp(argv[1], "-d") == 0)
      debug = 1;
  }

  int quote = 0;
  int finished = 0;
  //char *prompt = "320sh > ";
  char cmd[MAX_INPUT];
  bool escapeSeen = false;
  bool inCommandlineCache=true;
  
  /* Loads the history */
  initializeHistory();

  while (!finished) {
    char *cursor;  	//current position of modification 
    char *lastCursor;

    char buffer; 
    char last_char;
    int rv;
    int rewrite=0;
    int count;
	int lastVisitedHistoryIndex_UP=-1;
	int lastVisitedHistoryIndex_DOWN=-1;

	/*Test Start 
	printf("cmd is: %s, pid is: %d",cmd,getpid());*/

	// Clear previous cmd, or else first initialize 
	cmd[0] ='\0';

    /* Print newLine the first time 
    if(firstTimeRun){
      firstTimeRun=0;
      write(1,"\n",1);
    }*/

    /*REENTRANT PRINTING 
    write(1, "[", 1);
    write(1, getenv("PWD"), strlen(getenv("PWD")));
    write(1, "] ", 1);

    // Print the shell prompt
    rv = write(1, prompt, strlen(prompt));
    if (!rv) { 
      finished = 1;
      break;
    } */

    /*NON-REENTRANT PRINTING*/
    fprintf(stdout,"[%s]320sh >",getenv("PWD"));
    fprintf(stdout,"%s",saveCursorPosAscii);
    fflush(stdout);
    //write(1, saveCursorPosAscii, strlen(saveCursorPosAscii));

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
    //write(1,"\n at before read forloop\0",25);
    for(rv = 1, count = 0, 
	  cursor=cmd, lastCursor=cursor,last_char = 1;
	  rv && (++count < (MAX_INPUT-1))&& (last_char != '\n') ; ) {
      //printf("\tBegin parse forloop\n");
	  /* Read the value */
      rv = read(0, &buffer, 1);
      //printf(" \tread value\n");
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
        Job* fg=getJobNode(foreground);
        if(fg!=NULL){
          ((fg->next)->prev)=fg->prev;
          ((fg->prev)->next)=fg->next;
          printf("preparing to sigKill");
          if(kill((fg->pid),SIGKILL)){
          	printf("\nkilled child: %d",fg->pid);
          }
        }
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

    /*Print Debug Start */
    if (debug){
      char *cmdEnd = strrchr(cmd, '\n');
        *cmdEnd = '\0';
      write(2, "RUNNING: ", 9);
      write(2, cmd, strlen(cmd));
      error = 0;
    }
      /* assign signal handling */
      signal(SIGCHLD,killChildHandler);
    if(executeArgArray(parsedArgArray,envp)==0) continue;
    
    /*Print debug End */
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

			/***************************/
			/* HISTORYCOMMAND PROGRAM */
			/***************************/
/*
 * Global variables for the history 
 *
extern int historySize;
extern char* historyCommand[50];
extern int historyHead;
extern int current;*/

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
  //printHistoryCommand();
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

  //printf("Concatenating:\n");
  /*Build the path */
  strcpy(historyPath,getenv("HOME"));
  strcat(historyPath,historyFileExtension);
  printf("HOME IS: %s", historyPath);

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
    printf("last Storing: %s\n", historyCommand[currentHistoryIndex]); 
  }
  /*Close file */
  free(historyPath);
  fclose(historyFile);
  printf("Done saving to disk:\n");
  return true;
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




                 /**************************************/
                  /*        JOB NODE PROGRAM           */
				 /**************************************/

/* GLOBAL EXTERNS 
extern Job* jobHead; */

int jobSize(){
  int size =0;
  Job* jobPtr = jobHead;
  if(jobPtr!=NULL){
  	jobPtr++;
  }
  return size;
}
/*
 * A function that creates a job node containing job process info
 * @param pid: the job identifier
 * @param jobName: name of job
 * @param processGroup : 
 */
Job* createJobNode(pid_t pid, pid_t processGroup, char* jobName, int exitStatus, int runStatus){
  Job* newJobNode;
  if((newJobNode=malloc(sizeof(Job)))==NULL){
  	return NULL; /*Unable to allocate*/
  }
  /*Initialize memory */
  newJobNode->pid = pid;
  newJobNode->processGroup = processGroup;
  strcpy(newJobNode->jobName,jobName);
  newJobNode->exitStatus =exitStatus;
  newJobNode->runStatus = runStatus;
  newJobNode->next=NULL;
  newJobNode->prev=NULL;
  return newJobNode;
}

/*A builtin command that prints the job list */
void processJobs(){
  printJobListWithHandling();
}


/* A function that takes an arg array and finds the %int parameter for the processFG()
 * @param argArray, a NULL terminated argument array of string
 * @param argCount, the size of the arg Array
 * @return: -1 if an invalid size such as 0,-1. Value of num at %num otherwise
 */
int findFGParameter(char ** argArray, int argCount){

	char* str;
    int num=-1, i;
    char percentBuff[MAX_INPUT];

    for(i=0;i<argCount;i++){
      str= argArray[i];
      if(*str=='%'){
        str++;
        if(str!=NULL && *str>=48 && *str<=57){
          num = atoi(strcpy(percentBuff,str));
        }
      }
    }
    if(num>0)
      return num;
    else return -1;
}

void processFG(char ** argArray,int argCount){
  int jobIDPosition=0;
  Job* nextFGJob;

  /*calculate position from argArray, for the num in job linked list */
  jobIDPosition = findFGParameter(argArray,argCount);
  //printf("\nfindFGParameter in processFG() is :%d",jobIDPosition);


  /*Get the job at linked list position */
  nextFGJob = getJobNodeAtPosition(jobIDPosition);
  if(nextFGJob==NULL){
  	printf("%d, No such job",jobIDPosition);
  }else{
  	/*If to restart the FG Job */
  	Job* fgNode = getJobNode(foreground);
  	if( fgNode!=NULL && ((nextFGJob->pid)==(foreground))
  	  && ((fgNode->runStatus)<2)){
  		continueProcess(foreground);
  		printJobNode(fgNode,getPositionOfJobNode(fgNode));
  		//IGNORE CASE: if equal foreground but already running, runstatus ==2
  	}else if(fgNode!=NULL && (nextFGJob->pid)!=(foreground)) {
  		/*If to replace current FG Job*/
		suspendProcess(foreground);
		printJobNode(fgNode,getPositionOfJobNode(fgNode));
		continueProcess((nextFGJob->pid));
		foreground = (nextFGJob->pid);
		printJobNode(fgNode,getPositionOfJobNode(fgNode));

  	}else{
  		/* No active foreground, set the background as the new foreground */
  		foreground = nextFGJob->pid;
  		continueProcess(foreground);

  		printJobNode(getJobNode(foreground),getPositionOfJobNode(getJobNode(foreground)));
  	}

  }

}

bool suspendProcess(pid_t pid){
  Job* suspendJob;
  suspendJob = getJobNode(pid);
  if(suspendJob!=NULL){
  	suspendJob->runStatus=1;
  	kill((suspendJob->pid),SIGTSTP);
  	return true;
  }
  return false;
}

bool killProcess(pid_t pid){
  Job* killJob;
  killJob = getJobNode(pid);
  if(killJob!=NULL){
  	killJob->runStatus=0;
  	kill((killJob->pid),SIGSTOP);
  	return true;
  }
  return false;
}
bool continueProcess(pid_t pid){
  Job* continueJob;
  continueJob = getJobNode(pid);
  if(continueJob!=NULL){
  	continueJob->runStatus=2;
  	kill((continueJob->pid),SIGCONT);
  	return true;
  }
  return false;
}



/*
 * A function that sets new values to the jobNode with given pid, or creates a new jobNode with such values
 * if such pid not exists. 
 * @param key: the key string
 * @param value: the value string
 * @return : Job* , the created or modified jobNode
 */
Job* setJobNodeValues(pid_t pid, pid_t processGroup, char* jobName, int exitStatus, int runStatus){
  
  Job* jobNodePtr = getJobNode(pid);
  /*TEST JOB NODE RETRIEVED*/
  if(jobNodePtr!=NULL){
  	char* jobNameString = malloc(sizeof(char)* (strlen(jobNodePtr->jobName)+1));
  	strcpy(jobNameString,jobNodePtr->jobName);
  	write(1,jobNameString,strlen(jobNameString));
  }

  /*Test Print 
  printf("\nIn setJobNodeValues: pid is %d, jobName is %s",pid, jobName);
  printf("\nfound jobNodePtr==NULL: %d",jobNodePtr==NULL);*/

  if(jobNodePtr!=NULL){
    /*Set the Values for the Job Node, and return it */
    jobNodePtr->pid = pid;
    jobNodePtr->processGroup = processGroup;
    strcpy(jobNodePtr->jobName,jobName);
    jobNodePtr->exitStatus = exitStatus;
    jobNodePtr->runStatus = runStatus;
    return jobNodePtr;

  }else{

  	/*Create job node */
    jobNodePtr = createJobNode(pid,processGroup,jobName,exitStatus,runStatus);
    
    /*Test Printing Node 
    printf("\ncreated new jobnode:");
    printJobNode(jobNodePtr,69);
    printf("\tjobnode next: %p",jobNodePtr->next);
    printf("\tjobnode prev: %p",jobNodePtr->prev);*/


    /* Initialize cursor */
    Job *jobRunPtr = jobHead;

    /*If NULL HEAD, set created node as new head and return it*/
    if(jobRunPtr==NULL){
	  jobNodePtr->next = NULL;
	  jobNodePtr->prev = NULL;
      jobHead = jobNodePtr;
      
      return jobHead;

    }

    /*ELSE, FIND LAST ELEMENT AND RETURN IT*/
    while((jobRunPtr->next)!=NULL){
      jobRunPtr=jobRunPtr->next;
    }
    /* Set to the jobNode to last of the list*/
    jobRunPtr->next = jobNodePtr;
    jobNodePtr->prev = jobRunPtr;
    jobNodePtr->next =NULL;

    return jobNodePtr;
  }
}

/*
 * A function that returns the job node by its pid.
 * @param pid: the job pid identifier
 * @return Job Ptr: Pointer to the job node struct
 */
Job* getJobNode(pid_t pid){
  /*Initialize*/
  pid_t jobPID;
  Job* jobPtr = jobHead;

  /*check case*/
  if(jobPtr==NULL) return NULL;
  
  /* search list for matching pid*/
  while(jobPtr!=NULL){ 
    jobPID = jobPtr->pid;
    if(pid==jobPID) 
      return jobPtr;
  	else 
  	  jobPtr = jobPtr->next;
  }
  return NULL;
}

/* 
 * A function that prints the job status
 */
 void printJobList(){
   Job* jobPtr = jobHead;
   int jobID =1;
   while(jobPtr!=NULL){
     printJobNode(jobPtr,jobID);
     /*Print the next Job if possible */
     jobPtr=jobPtr->next;
     jobID++;
   }
 }

/* A function that prints the job list state, but also modifies runStatus. On second pass, deletes
 * the invalid and done jobs
 */
 void printJobListWithHandling(){
   Job* jobPtr = jobHead;
   //Job* freePtr;

   int jobID =1;
   while(jobPtr!=NULL){
   	 /*Remove invalid job Nodes*/
   	 if(jobPtr->runStatus>2||jobPtr->runStatus<0){

   	   /*Pointer to Free this block to delete it*/
   	   //freePtr=jobPtr;

   	   /*Delink adjacent chains, if they exist*/
   	 	if((jobPtr->prev)!=NULL){
   	       ((jobPtr->prev)->next)=jobPtr->next;
   	 	}
   	 	if((jobPtr->next)!=NULL){
   	       ((jobPtr->next)->prev)=jobPtr->prev;
   	 	}
   	   jobPtr=jobPtr->next;

   	   /*Release the memory*/
   	   //free(freePtr);
   	   continue;
   	 }
    	 /*Display the next valid Node*/
      printJobNode(jobPtr,jobID);

      /*If stopped, alter runStatus to mark for deletion on next pass*/
      if((jobPtr->runStatus)==0){
        	(jobPtr->runStatus) = -1;
      }

      jobPtr=jobPtr->next;
      jobID++;
   }

}

 /* 
 * A function that prints the job status re-entrantly
 */
void printJobNode(Job* jobPtr, int JID){
  char* sprintfStr=calloc(MAX_INPUT,1);
  sprintf(sprintfStr,"[%d]  (%d)%1s  %-12s %-50s\n",
  JID,
  (jobPtr->pid),
  runStateSymbol((jobPtr->runStatus)),
  runStatusToString((jobPtr->runStatus)),
  (jobPtr->jobName));

  /*CleanUp*/
  write(1,sprintfStr,strlen(sprintfStr));
  free(sprintfStr);
}

/* 
 * A function that turns the run status int to a string 
 * @param runStatus: the int for the Job
 * @return: the string literally returned. 
 */
char* runStatusToString(int runStatus){
  if(runStatus==0){
    return "DONE\0";
  }else if(runStatus==1){
    return "STOPPED\0";
  }else if(runStatus==2){
    return "Running\0";
  }else{
    return "ERROR\0";
  }
}
/* 
 * A function that turns the symbol for the run state. 
 */
char* runStateSymbol(int runStatus){
  if(runStatus==0){
    return "S\0";
  }else if(runStatus==1){
    return "s\0";
  }else if(runStatus==2){
    return "+\0";
  }else{
    return "?\0";
  }
}

Job* getJobNodeAtPosition(int position){
  Job* curJob = jobHead;
  int i;
  for(i=1;i<position;i++){
  	if(curJob!=NULL){
      curJob=curJob->next;
  	}else{
	  return NULL;
  	} 
  }
  return curJob;
}
int getPositionOfJobNode(Job* node){
  int posCount=1;
  Job* jobPtr=jobHead;
  while(node!=NULL && jobPtr!=NULL && (node->pid)!=(jobPtr->pid)){
  	jobPtr = jobPtr->next;
  	posCount++;
  }
  if(node!=NULL && jobPtr!=NULL && (node->pid)==(jobPtr->pid))
  	return posCount;
  else 
  	return -1;
}



