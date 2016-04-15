#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>

int main(void){
  int i;
  //for(i=0;i<10;i++){
  	sleep(10);
  	char * str = "child finished sleeping";
  	write(1,str,strlen(str));
  //}
}