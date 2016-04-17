/****************** SERVER CODE ****************/
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <string.h>
#include <errno.h>
#include <netdb.h>
#include <sys/types.h>

char **initArgArray(int argc, char **argv, char **argArray);
char **initFlagArray(int argc, char **argv, char **flagArray);

int main(int argc, char ** argv){
    char * argArray[1024];
    char * flagArray[1024];
    memset(&argArray, 0, sizeof(argArray));
    memset(&flagArray, 0, sizeof(flagArray));
    initArgArray(argc, argv, argArray);
    initFlagArray(argc, argv, flagArray);
    return 0;
}
    
char **initArgArray(int argc, char **argv, char **argArray){
    int i;
    int argCount = 0;
    for (i = 0; i<argc; i++){
        if (strcmp(argv[i], "-h")!=0 && strcmp(argv[i], "-v")!=0){
            argArray[argCount] = argv[i];
            argCount++;
        }
    }
    return argArray;
}
char **initFlagArray(int argc, char **argv, char **flagArray){
    int i;
    int argCount = 0;
    for (i = 0; i<argc; i++){
        if (strcmp(argv[i], "-h")==0 || strcmp(argv[i], "-v")==0 || strcmp(argv[i], "-c")==0){
            flagArray[argCount] = argv[i];
            argCount++;
        }
    }
    return flagArray;
}
