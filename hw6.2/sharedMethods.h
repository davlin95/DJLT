#define VERBOSE "\x1b[1;34m"
#define ERROR "\x1b[1;31m"
#define DEFAULT "\x1b[0m"

            /******* ACCESSORY METHODS ******/
 /* 
  * A help menu function for client
  */ 
 void displayHelpMenu(char **helpMenuStrings){
    char** str = helpMenuStrings;
    while(*str!=NULL){
      printf("%-30s\n",*str); 
      str++;
    }
 }

 /*
  *	read wrapper method that reads byte by byte 
  */
bool ReadBlockedSocket(int fd, char* buffer){
  int bytes = 0;
  int counter = 0;
  char character;
  char* bufferPtr=buffer;
  while( ((bytes = read(fd,&character,1) )>0) && strstr(buffer,"\r\n\r\n")==NULL && counter<1023){
    *bufferPtr=character;
     bufferPtr++;
     counter++;
  }
  if(bytes == -1){
    return false;
  }
  return true;
}

 /*
  * read wrapper method that reads byte by byte 
  */
bool ReadNonBlockedSocket(int fd, char* buffer){
  int bytes = 0;
  int counter =0;
  char character;
  char* bufferPtr=buffer;
  while( ((bytes = read(fd,&character,1) )>0) && strstr(buffer,"\r\n\r\n")==NULL && counter<1024  ){
    *bufferPtr=character;
     bufferPtr++;
     counter++;
  }
  if(bytes == 0){
    return false;
  }
  return true;
}

/*
 * returns the argc+1, for the null that is attached. 
 */
int initArgArray(int argc, char **argv, char **argArray){
    int i;
    int argCount = 0;
    for (i = 0; i<argc; i++){
        if (strcmp(argv[i], "-h")!=0 && strcmp(argv[i], "-v")!=0 && strcmp(argv[i], "-c")!=0){
            argArray[argCount++] = argv[i];
        }
    }
    argArray[argCount++] = NULL;
    return argCount;
}

int initFlagArray(int argc, char **argv, char **flagArray){
    int i;
    int argCount = 0;
    for (i = 0; i<argc; i++){
        if (strcmp(argv[i], "-h")==0 || strcmp(argv[i], "-v")==0 || strcmp(argv[i], "-c")==0){
            flagArray[argCount++] = argv[i];
        }
    }
    flagArray[argCount] = NULL;
    return argCount;
}

