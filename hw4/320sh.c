#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

// Assume no input line will be longer than 1024 bytes
#define MAX_INPUT 1024

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

void parseCommandLine(char* cmd){

  /* Headline printing for debugging

  char * promptString = "\nprinting commandline:\n\0";
  write(1,promptString,strlen(promptString)); 
  int cmdLen = strnlen(cmd,MAX_INPUT); 
  write(1,cmd,cmdLen); */

  /* initialize*/
  char * token;
  char * savePtr;
  char * argArray[MAX_INPUT]; /* at most max_input args */
  int argCount = 0;
  token = strtok_r(cmd," ",&savePtr);
  while(token != NULL){

  	/*Test: Print out token 
    write(1,token,strlen(token));
    write(1,"-->",3); */

    argArray[argCount++]= token;
    token = strtok_r(NULL," ",&savePtr);
  }

  /*Test: Print out contents of argArray */
  write(1,"\nfinished\n\0",11);
  for(int i = 0; i < argCount; i++){
  	write(1,"\n",1);
  	write(1,argArray[i],strlen(argArray[i]));
  }
  printf("Argcount is %d\n",argCount);
  printf("last item is %s\n", argArray[argCount-1]);
  fflush(stdout);
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
    
    parseCommandLine(cmd);
  }
  return 0;
}
