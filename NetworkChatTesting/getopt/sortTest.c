#define _GNU_SOURCE
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>  
#include <sys/socket.h>
#include <netinet/in.h>  
#include <string.h> 
#include <errno.h>   
#include <netdb.h>
#include <sys/types.h>
#include <sys/poll.h> 
#include <sys/fcntl.h>  
#include <signal.h> 
#include <sys/wait.h>

int main(int argc, char* argv[]){ 
  pid_t pid;
  int status;
  char *argArray[] = {"sort", argv[1], argv[2], NULL};
  if ((pid=fork())==0){
  	printf("1 - %s, 2- %s\n", argv[1], argv[2]);
      if (execvp("sort", {"sort", argv[1], argv[2], NULL}) < 0)
      	fprintf(stderr, "error");
    }
    else{
    	wait(&status);
    }
  return(0);
}