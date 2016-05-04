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
#include <time.h>

char **initArgArray(int argc, char **argv, char **argArray);
char **initFlagArray(int argc, char **argv, char **flagArray);

int main(int argc, char ** argv){
    char outstr[200];
    time_t t;
    struct tm *tmp;
    t = time(NULL);
    tmp = localtime(&t);
    if (tmp == NULL){
    	perror("localtime");
    	exit(EXIT_FAILURE);
    }
    if (strftime(outstr, sizeof(outstr), argv[1], tmp)==0){
    	fprintf(stderr, "strftime returned 0");
    	exit(EXIT_FAILURE);
    }
    printf("Result string is %s\n", outstr);
    exit(EXIT_SUCCESS);
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
