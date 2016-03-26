#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/wait.h>
#include <sys/stat.h>

/*Header Prototypes */
int isBuiltIn(char * command);
char ** parseCommandLine(char* cmd, char ** argArray);
void executeArgArray(char * argArray[],char * environ[]);
void *statFind(char *cmd);
void printError(char* command);
void createNewChildProcess(char* objectFilePath,char** argArray, char** environ);


/* Assume no input line will be longer than 1024 bytes */
#define MAX_INPUT 1024

/* Make global array of built-in strings */
char * globalBuiltInCommand[] = {"ls","cd","pwd","echo","set","ps","printenv"};

/* Test Function*/
void test(char ** envp){

  /* Print environment variables 
  printf("\nTesting environmentP: \n");
  char** env;
  for(env = envp; *env != '\0'; env++){
  	char * envString = *env;
  	printf("%s\n",envString);
  }
  printf("Path: %s\n", getenv("PATH")); */


}

void toDoList(){

/* Tasks*/
   /* 
      -- statFind() : Checks path variable for ls command using stat
      -- parseCommandLine(): parses and executes command line. 
      -- setPath(): sets path variable. Also must update printenv results. 
      -- help() : printout the built in's. 
      -- pwd(): 
   */

  /* David : */
     /* 
		-- parseCommandLine(): currently fills up the arg list. last item is '\0'?
		--
     */

  /* Wilson: */
      /*
      	-- statFind()
      	--
      */

}

void printError(char * command){
  write(2,"\n",1);
  write(2,command,strnlen(command,MAX_INPUT));
  char * msg = ":command not found\n\0";
  write(2,msg,strlen(msg));  
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
  token = strtok_r(cmd," \n",&savePtr);
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
  token = strtok_r(getenv("PATH"),":",&savePtr);
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
void pwd(){
  char *path;
  char buf[MAX_INPUT];
  path = getcwd(buf, MAX_INPUT);
  write(1, path, strlen(path));
  write(1, "\n", 1);
}
void set(char argArray[]){
  char * token;
  char * token2;
  char *equals = "=";
  token = strtok(argArray, equals);
  token2 = strtok(NULL, equals);
  if (getenv(token) != NULL)
    setenv(token, token2, 1);
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
      printf("\nexecute cd\n");
      fflush(stdout);
    }else if(strcmp(command,"pwd")==0){
      pwd();
    }else if(strcmp(command,"echo")==0){
      printf("\nexecute echo\n");
      fflush(stdout);
    }else if(strcmp(command,"set")==0){
      set(argArray[1]);
    }else if(strcmp(command,"help")==0){
      printf("\nexecute help\n");
      fflush(stdout);
    }else{
      /* Use statfind() to launch the program from shell */
      printf("\nuse statfind() to launch the built in\n");
      printf("%s\n",command);
      fflush(stdout);

      /*Find and execute the binary path */
      char * fullCommandPath; 
      fullCommandPath = (char*)statFind(argArray[0]);
      if(fullCommandPath!=NULL){
        createNewChildProcess(fullCommandPath,argArray,environ);
      }else { /*Not found */
        printError(command);
      }

    }

  }else{ /* Is an object file such as a.out */
    printf("\nobject file is like a.out\n");
    fflush(stdout);

  }
}

void createNewChildProcess(char* objectFilePath,char** argArray, char** environ){
  pid_t pid;
  int status;
  if((pid=fork())==0){
    execve(objectFilePath,argArray,environ);
    exit(0);
  }else{
      wait(&status);
      printf("\nParent Process reaped child process %d\n",pid);
      fflush(stdout);
  }

}

int 
main (int argc, char ** argv, char **envp) {

  int finished = 0;
  char *prompt = "320sh> ";
  char cmd[MAX_INPUT];


  while (!finished) {
    char *cursor;
    char last_char;
    int rv;
    int count;

    // Print the prompt
    rv = write(1, prompt, strlen(prompt));
    if (!rv) { 
      finished = 1;
      break;
    }
    // read and parse the input
    for(rv = 1, count = 0, 
	  cursor = cmd, last_char = 1;
	  rv 
	  && (++count < (MAX_INPUT-1))
	  && (last_char != '\n');
	cursor++) {
      rv = read(0, cursor, 1);
      last_char = *cursor;
      if(last_char == 3) {
        write(1, "^c", 2);
      } else {
		write(1, &last_char, 1);
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
  return 0;
}
