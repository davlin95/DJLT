#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdbool.h>
#include <unistd.h>

bool isAllDigits(char* string){
  int i;
  char* checkPtr=string;
  for(i=0;i<strlen(string);i++){
    if(*checkPtr>'9' || *checkPtr < '0' ){
    	return false;
    }
  }
  return true;
}


int main(int argc, char ** argv) {

    if(argv[1]!=NULL && isAllDigits(argv[1]) ){
    	int chatFd = atoi(argv[1]);
    	printf("chatFd is %d",chatFd);
    }

  	while(1){
  		char stdinBuffer[1024];
  		memset(&stdinBuffer,0,1024);
  		int bytes =-1;
  		bytes = read(0,&stdinBuffer,1024);
  		if(bytes>0){
  			printf("read input: %s\n",stdinBuffer);
  		}
	}  
  	return 0;
}