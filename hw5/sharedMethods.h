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
  *	read wrapper method
  */
bool Read(int fd, char* protocolBuffer){
	if (protocolBuffer)
	printf("entering read function and fd is %d\n", fd);
  if (read(fd, &protocolBuffer,1024) < 0){
    fprintf(stderr,"Read(): bytes read negative\n");
    return false;
  }
  printf("Read(): %s\n", protocolBuffer);
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