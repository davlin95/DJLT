/* Create file defining WOLFIE PROTOCOL */

#ifndef PROTOCOL_WOLFIE
#define PROTOCOL_WOLFIE "WOLFIE \r\n\r\n"
#endif 

#ifndef PROTOCOL_EIFLOW
#define PROTOCOL_EIFLOW "EIFLOW \r\n\r\n"
#endif 

#ifndef PROTOCOL_IAM
#define PROTOCOL_IAM "IAM"
#endif 

#ifndef PROTOCOL_BYE
#define PROTOCOL_BYE "BYE \r\n\r\n"
#endif 

#ifndef PROTOCOL_MOTD
#define PROTOCOL_MOTD "MOTD"
#endif 

#ifndef PROTOCOL_HI
#define PROTOCOL_HI "HI"
#endif 

#ifndef PROTOCOL_LISTU
#define PROTOCOL_LISTU "LISTU \r\n\r\n"
#endif 

#ifndef PROTOCOL_UTSIL
#define PROTOCOL_UTSIL "UTSIL"
#endif 

#ifndef PROTOCOL_TIME
#define PROTOCOL_TIME "TIME \r\n\r\n"
#endif 

#ifndef PROTOCOL_EMIT
#define PROTOCOL_EMIT "EMIT"
#endif 

#ifndef PROTOCOL_MSG
#define PROTOCOL_MSG "MSG"
#endif 

#ifndef PROTOCOL_UOFF
#define PROTOCOL_UOFF "UOFF"
#endif 

#ifndef PROTOCOL_IAMNEW
#define PROTOCOL_IAMNEW "IAMNEW"
#endif 

#ifndef PROTOCOL_HINEW
#define PROTOCOL_HINEW "HINEW"
#endif 

#ifndef PROTOCOL_NEWPASS
#define PROTOCOL_NEWPASS "NEWPASS"
#endif 

#ifndef PROTOCOL_SSAPWEN
#define PROTOCOL_SSAPWEN "SSAPWEN \r\n\r\n"
#endif 

#ifndef PROTOCOL_AUTH
#define PROTOCOL_AUTH "AUTH"
#endif 

#ifndef PROTOCOL_PASS
#define PROTOCOL_PASS "PASS"
#endif 

#ifndef PROTOCOL_SSAP
#define PROTOCOL_SSAP "SSAP \r\n\r\n"
#endif 

#ifndef PROTOCOL_ERR0
#define PROTOCOL_ERR0 "ERR 00 USER NAME TAKEN \r\n\r\n"
#endif 

#ifndef PROTOCOL_ERR1
#define PROTOCOL_ERR1 "ERR 01 USER NOT AVAILABLE \r\n\r\n"
#endif 

#ifndef PROTOCOL_ERR2
#define PROTOCOL_ERR2 "ERR 02 BAD PASSWORD \r\n\r\n"
#endif 

#ifndef PROTOCOL_ERR100
#define PROTOCOL_ERR100 "ERR 100 INTERNAL SERVER ERROR \r\n\r\n"
#endif



/* Create file defining WOLFIE PROTOCOL */

#ifndef WOLFIE
#define WOLFIE 1
#endif 

#ifndef EIFLOW
#define EIFLOW 2
#endif 

#ifndef IAM
#define IAM 3
#endif 

#ifndef BYE
#define BYE 4
#endif 

#ifndef MOTD
#define MOTD 5
#endif 

#ifndef HI
#define HI 6
#endif 

#ifndef LISTU
#define LISTU 7
#endif 

#ifndef UTSIL
#define UTSIL 8
#endif 

#ifndef TIME
#define TIME 9
#endif 

#ifndef EMIT
#define EMIT 10
#endif 

#ifndef MSG
#define MSG 11
#endif 

#ifndef UOFF
#define UOFF 12
#endif 

#ifndef IAMNEW
#define IAMNEW 13
#endif 

#ifndef HINEW
#define HINEW 14
#endif 

#ifndef NEWPASS
#define NEWPASS 15
#endif 

#ifndef SSAPWEN
#define SSAPWEN 16
#endif 

#ifndef AUTH
#define AUTH 17
#endif 

#ifndef PASS
#define PASS 18
#endif 

#ifndef SSAP
#define SSAP 19
#endif 

#ifndef ERR0
#define ERR0 20
#endif 

#ifndef ERR1
#define ERR1 21
#endif 

#ifndef ERR2
#define ERR2 22
#endif 

#ifndef ERR100
#define ERR100 23
#endif

/* 
 * A function that checks if the returned string is the protocol verb to be compared 
 */
bool checkVerb(char *protocolVerb, char* string){
  if (strncmp(string, protocolVerb, strlen(protocolVerb)) == 0)
    return true;
  return false;
}

/*
 * Takes a string and tests it for the following structure. PROTOCOL_TYPE MIDDLEARGUMENT \r\n\r\n
 * and fills the buffer with the middle argument, if it returns true (meaning it pass formatting test)
 */
bool extractArgAndTest(char *string, char *buffer){
    char * savePtr;
    char *token;
    int arrayIndex = 0;
    char tempString[1024];
    char * protocolArray[1024];

    //CLEAR THE BUFFERS
    memset(&tempString, 0, 1024);
    memset(&protocolArray, 0, 1024);

    //CHOP UP A COPY OF THE TESTED STRING
    strcpy(tempString, string);
    token = strtok_r(tempString, " ", &savePtr);
    while (token != NULL){
      protocolArray[arrayIndex++] = token;
      token = strtok_r(NULL," ",&savePtr);
    }

    //LOOK FOR TEST CASES
    if (arrayIndex > 3 || strcmp(protocolArray[2], "\r\n\r\n")!=0)
      return false;
    //PASSES TEST
    strcpy(buffer, protocolArray[1]);
    return true;
}

/*
 *  A function that takes a string and tests it for the structure of "MSG TOPERSON FROMPERSON MSG \r\n\r\n"
 *  returns true if passes format test. copies the message into the buffer.
 */
bool extractArgAndTestMSG(char *string, char *buffer){
    char * savePtr;
    char *token;
    int arrayIndex = 0;
    char tempString[1024];
    char * protocolArray[1024];

    //CLEAR BUFFERS
    memset(&tempString, 0, 1024);
    memset(&protocolArray, 0, 1024);
    memset(&string, 0, strlen(string));

    //CHOP UP THE STRING
    strcpy(tempString, string);
    token = strtok_r(tempString, " ", &savePtr);
    while (token != NULL){
      protocolArray[arrayIndex++] = token;
      token = strtok_r(NULL," ",&savePtr);
    }

    //ERROR CHECK THE INPUT, 
    if ( arrayIndex < 5 ){
      fprintf(stderr,"error: not enough arguments in buildProtocolString()");
      return false;
    }else if(strcmp(protocolArray[arrayIndex-1], "\r\n\r\n")!=0 ){
      fprintf(stderr,"error: buildProtocolString() not ended with protocol ending");
      return false;
    }else if(strcmp(protocolArray[0], PROTOCOL_MSG)!=0){
      fprintf(stderr,"error: buildProtocolString() not started with protocol string");
      return false;
    }

    //COPY ALL ARGS AFTER MSG, UP TO BEFORE THE PROTOCOL FOOTER
    int i;
    for(i=1; i<arrayIndex-1 ;i++){
      strcat(buffer, protocolArray[i]);
    }
    return true;
}

/*
 * A function that builds a string in the format "PROTOCOL MIDDLE \R\N\R\N"
 * returns true unless buffer is NULL
 */
bool buildProtocolString(char* buffer, char* protocol, char* middle){
  if(buffer==NULL||protocol == NULL || middle == NULL) {
    fprintf(stderr,"error: buildProtocolString() on a NULL or missing parameter");
    return false;
  }
  if( (strcmp(buffer," ")==0 ) || (strcmp(protocol," ")==0 ) || (strcmp(middle," ")==0 )) {
    fprintf(stderr,"error: buildProtocolString() on a space char");
    return false;
  }

  strcat(buffer,protocol);
  strcat(buffer," ");
  strcat(buffer, middle);
  strcat(buffer," \r\n\r\n");
  return true;
}

/*
 * A function that builds a string in the format "MSG TOPERSON FROMPERSON MSG \R\N\R\N"
 * returns true unless buffer is NULL.
 */
bool buildMSGProtocol(char* buffer,char* toPerson,char* fromPerson, char* message){
    if(buffer==NULL || toPerson == NULL || fromPerson==NULL || message==NULL){
        fprintf(stderr,"error: buildMSGPROTOCOL() on a NULL or missing parameter");
        return 0;
    }
    if( (strcmp(buffer," ")==0 ) || (strcmp(toPerson," ")==0 ) || (strcmp(fromPerson," ")==0 ) || (strcmp(message," ")==0 )) {
    fprintf(stderr,"error: buildProtocolString() on a space char");
    return false;
  }
    memset(&buffer,0,strlen(buffer));
    strcat(buffer,PROTOCOL_MSG);
    strcat(buffer," ");
    strcat(buffer,toPerson);
    strcat(buffer," ");
    strcat(buffer,fromPerson);
    strcat(buffer," ");
    strcat(buffer,message);
    strcat(buffer," \r\n\r\n");
    return 1;
}

void protocolMethod(int fd, int wolfieVerb, char* optionalString, char* optionalString2, char* optionalString3);

/* 
 * Process a string containing the "/chat" command, includes testing for errors. 
 */
bool processChatCommand(int fd, char* string, char* thisUserName){
    char * savePtr;
    char *token;
    int arrayIndex = 0;
    char tempString[1024];
    char * protocolArray[1024];

    //CLEAR BUFFERS
    memset(&tempString, 0, 1024);
    memset(&protocolArray, 0, 1024);

    //CHOP UP THE STRING
    strcpy(tempString, string);
    token = strtok_r(tempString, " ", &savePtr);
    while (token != NULL){
      protocolArray[arrayIndex++] = token;
      token = strtok_r(NULL," ",&savePtr);
    }

    //ERROR CHECK THE INPUT, 
    if ( arrayIndex < 2 ||strcmp(protocolArray[0], "/chat")!=0 ){
      fprintf(stderr,"processChatCommand(): array missing parameters or not chat string\n");
      return false;
    }

    // BUILD THE TRAILING MESSAGE INTO ONE ARGUMENT STRING
    char messageReplica[1024];
    memset(&messageReplica,0,1024);

    int i;
    for(i=2;i<arrayIndex;i++){
        strcat(messageReplica,protocolArray[i]);
        if(i<arrayIndex-1){
          strcat(messageReplica," ");
        }
    }
    messageReplica[1023]='\0';

    //CALL PROTOCOL METHODS BASED ON PARAMETERS
    protocolMethod(fd,MSG,protocolArray[1],thisUserName,messageReplica);
    return true;
}

void protocolMethod(int fd, int wolfieVerb, char* optionalString, char* optionalString2, char* optionalString3){
  char buffer[1032];
  memset(&buffer,0,1032);
  switch(wolfieVerb){
    case WOLFIE: 
                send(fd,PROTOCOL_WOLFIE,strlen(PROTOCOL_WOLFIE),0); // MACRO NULL TERMINATED BY DEFAULT
                break;
    case EIFLOW:
                send(fd,PROTOCOL_EIFLOW,strlen(PROTOCOL_EIFLOW),0); // MACRO NULL TERMINATED BY DEFAULT
                break;
    case BYE:   
                send(fd,PROTOCOL_BYE,strlen(PROTOCOL_BYE),0); // MACRO NULL TERMINATED BY DEFAULT
                break;
    case IAM:   
                if(optionalString!=NULL){
                  	buildProtocolString(buffer, PROTOCOL_IAM, optionalString);
                    printf("sending from protocolmethod to server: %s",buffer);
                	write(fd,buffer,strlen(buffer));
                    char * str = "finished writing to fd\n";
                    write(1,str,strlen(str));
                } // MACRO NULL TERMINATED BY DEFAULT
                break;

    case MOTD:   
                if(optionalString!=NULL){
                 	buildProtocolString(buffer, PROTOCOL_MOTD, optionalString);
                	send(fd,buffer,strlen(buffer),0); // MACRO NULL TERMINATED BY DEFAULT
                }
                break;
    case HI:   
                if(optionalString!=NULL){
                  	buildProtocolString(buffer, PROTOCOL_HI, optionalString);
                	send(fd,buffer,strlen(buffer),0); // MACRO NULL TERMINATED BY DEFAULT
                }
                break;
    case LISTU:   
                send(fd,PROTOCOL_LISTU,strlen(PROTOCOL_LISTU),0); // MACRO NULL TERMINATED BY DEFAULT
                break;
    case UTSIL:   
                if(optionalString!=NULL){
                 	buildProtocolString(buffer, PROTOCOL_UTSIL, optionalString);
                	send(fd,buffer,strlen(buffer),0); // MACRO NULL TERMINATED BY DEFAULT
                }
                break;
    case TIME:   
                send(fd,PROTOCOL_TIME,strlen(PROTOCOL_TIME),0); // MACRO NULL TERMINATED BY DEFAULT
                break;
    case EMIT:   
                if(optionalString!=NULL){
                  	buildProtocolString(buffer, PROTOCOL_EMIT, optionalString);
                	send(fd,buffer,strlen(buffer),0); // MACRO NULL TERMINATED BY DEFAULT
                }
                break;
    case MSG:   
                buildMSGProtocol(buffer,optionalString,optionalString2, optionalString3);
                send(fd,buffer,strlen(buffer),0); // MACRO NULL TERMINATED BY DEFAULT
                break;

   /* case UOFF:   
                send(fd,PROTOCOL_BYE,strlen(PROTOCOL_BYE),0); // MACRO NULL TERMINATED BY DEFAULT
                break;
    case IAMNEW:   
                send(fd,PROTOCOL_BYE,strlen(PROTOCOL_BYE),0); // MACRO NULL TERMINATED BY DEFAULT
                break;
    case HINEW:   
                send(fd,PROTOCOL_BYE,strlen(PROTOCOL_BYE),0); // MACRO NULL TERMINATED BY DEFAULT
                break;
    case NEWPASS:   
                send(fd,PROTOCOL_BYE,strlen(PROTOCOL_BYE),0); // MACRO NULL TERMINATED BY DEFAULT
                break;
    case SSAPWEN:   
                send(fd,PROTOCOL_BYE,strlen(PROTOCOL_BYE),0); // MACRO NULL TERMINATED BY DEFAULT
                break;
    case AUTH:   
                send(fd,PROTOCOL_BYE,strlen(PROTOCOL_BYE),0); // MACRO NULL TERMINATED BY DEFAULT
                break;
    case PASS:   
                send(fd,PROTOCOL_BYE,strlen(PROTOCOL_BYE),0); // MACRO NULL TERMINATED BY DEFAULT
                break;
    case PASS:   
                send(fd,PROTOCOL_BYE,strlen(PROTOCOL_BYE),0); // MACRO NULL TERMINATED BY DEFAULT
                break;
    case PASS:   
                send(fd,PROTOCOL_BYE,strlen(PROTOCOL_BYE),0); // MACRO NULL TERMINATED BY DEFAULT
                break;
    case PASS:   
                send(fd,PROTOCOL_BYE,strlen(PROTOCOL_BYE),0); // MACRO NULL TERMINATED BY DEFAULT
                break;
    case PASS:   
                send(fd,PROTOCOL_BYE,strlen(PROTOCOL_BYE),0); // MACRO NULL TERMINATED BY DEFAULT
                break;*/
    case ERR0:   
                send(fd,PROTOCOL_ERR0,strlen(PROTOCOL_ERR0),0); // MACRO NULL TERMINATED BY DEFAULT
                break;
  }

}


/*
 * returns the argc+1, for the null that is attached. 
 */
int initArgArray(int argc, char **argv, char **argArray){
    int i;
    int argCount = 0;
    for (i = 0; i<argc; i++){
        if (strcmp(argv[i], "-h")!=0 && strcmp(argv[i], "-v")!=0){
            argArray[argCount] = argv[i];
            argCount++;
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
            flagArray[argCount] = argv[i];
            argCount++;
        }
    }
    flagArray[argCount] = NULL;
    return argCount;
}
