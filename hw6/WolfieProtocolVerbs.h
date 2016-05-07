#include "sharedMethods.h"
#include "sfwrite.c"


char messageOfTheDay[1024];
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


int getMessages(char** messageArray, char* protocolBuffer){
    char * savePtr;
    char *token;
    int arrayIndex = 0;
    char message[1024];
    char* msgPtr = message;
    char tempString[1024];

    //CLEAR THE BUFFERS
    memset(&tempString, 0, 1024);
    memset(&message, 0, 1024);

    //CHOP UP A COPY OF THE TESTED STRING
    strcpy(tempString, protocolBuffer);
    token = strtok_r(tempString, "\r\n\r\n", &savePtr);
    while (token != NULL){
      strcpy(msgPtr, token);
      strcat(msgPtr, "\r\n\r\n");
      messageArray[arrayIndex++] = msgPtr;
      msgPtr = (void*)msgPtr + strlen(message);
      token = strtok_r(NULL,"\r\n\r\n",&savePtr);
    }
    messageArray[arrayIndex] = NULL;
    return arrayIndex;
}
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
bool extractArgsAndTest(char *string, char *buffer){
  char *argPtr = (void *)strchr(string, ' ') + 1;
  char *protocolTerminator = (void *)string + strlen(string) - 4;
  if (strcmp(protocolTerminator, "\r\n\r\n") == 0){
    strncpy(buffer, argPtr, strlen(argPtr) - 4);
    return true;
  }
  return false;
} 

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
    if (arrayIndex != 3 || strcmp(protocolArray[2], "\r\n\r\n")!=0){
      return false;
    }
    //PASSES TEST
    strcpy(buffer, protocolArray[1]);
    return true;
}

/*
 *  A function that takes a string and tests it for the structure of "MSG TOPERSON FROMPERSON MSG \r\n\r\n"
 *  returns true if passes format test. copies the message into the buffer.
 */
bool extractArgAndTestMSG(char *string, char* toBuffer, char* fromBuffer, char *messageBuffer){
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
    if ( arrayIndex < 5 ){
      //fprintf(stderr,"extractArgAndTestMSG() error: not enough arguments in buildProtocolMSGString");
      return false;
    }else if(strcmp(protocolArray[arrayIndex-1], "\r\n\r\n")!=0 ){
      //fprintf(stderr,"extractArgAndTestMSG() error: buildProtocolString() not ended with protocol ending");
      return false;
    }else if(strcmp(protocolArray[0], PROTOCOL_MSG)!=0){
      //fprintf(stderr,"extractArgAndTestMSG() error: buildProtocolString() not started with protocol string");
      return false;
    }

    //COPY THE TO PERSON IF POSSIBLE
    if(toBuffer!=NULL){
      strcpy(toBuffer,protocolArray[1]);
    }

    //COPY THE FROM PERSON IF POSSIBLE 
    if(fromBuffer!=NULL){
      strcpy(fromBuffer,protocolArray[2]);
    }
    //COPY ALL ARGS AFTER FROM PERSON, UP TO BEFORE THE PROTOCOL FOOTER
    int i;
    for(i=3; i<arrayIndex-1 ;i++){
      if(messageBuffer!=NULL){
          strcat(messageBuffer, protocolArray[i]);
          //DON"T ADD SPACE AFTER LAST MESSAGE WORD
          if(i!=arrayIndex-2){
            strcat(messageBuffer," ");
          }
      }
    }
    return true;
}

/*
 * A function that builds a string in the format "PROTOCOL MIDDLE \R\N\R\N"
 * returns true unless buffer is NULL
 */
bool buildProtocolString(char* buffer, char* protocol, char* middle){
  if(buffer==NULL||protocol == NULL || middle == NULL) {
    //fprintf(stderr,"error: buildProtocolString() on a NULL or missing parameter");
    return false;
  }
  if( (strcmp(buffer," ")==0 ) || (strcmp(protocol," ")==0 ) || (strcmp(middle," ")==0 )) {
    //fprintf(stderr,"error: buildProtocolString() on a space char");
    return false;
  }
  //BUILD STRING 
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
        //fprintf(stderr,"error: buildMSGPROTOCOL() on a NULL or missing parameter");
        return 0;
    }
    if( (strcmp(buffer," ")==0 ) || (strcmp(toPerson," ")==0 ) || (strcmp(fromPerson," ")==0 ) || (strcmp(message," ")==0 )) {
    //fprintf(stderr,"error: buildProtocolString() on a space char");
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

void protocolMethod(int fd, int wolfieVerb, char* optionalString, char* optionalString2, char* optionalString3, int verbose, pthread_mutex_t *lock);

/* 
 * Process a string containing the "/chat" command, includes testing for errors. 
 */
bool processChatCommand(int fd, char* string, char* thisUserName, int verbose, pthread_mutex_t *lock){
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
      //fprintf(stderr,"processChatCommand(): array missing parameters or not chat string\n");
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
    protocolMethod(fd,MSG,protocolArray[1],thisUserName,messageReplica, verbose, lock);
    return true;
}

void protocolMethod(int fd, int wolfieVerb, char* optionalString, char* optionalString2, char* optionalString3, int verbose, pthread_mutex_t *lock){
  char buffer[1032];
  memset(&buffer,0,1032);
  switch(wolfieVerb){
    case WOLFIE: 
                if (verbose){
                  sfwrite(lock, stdout, VERBOSE "%s", PROTOCOL_WOLFIE);
                  sfwrite(lock, stdout, DEFAULT "");
                  //printf(VERBOSE "%s" DEFAULT, PROTOCOL_WOLFIE);
                }
                send(fd,PROTOCOL_WOLFIE,strlen(PROTOCOL_WOLFIE),0); // MACRO NULL TERMINATED BY DEFAULT
                break;
    case EIFLOW: 
                if (verbose){
                  sfwrite(lock, stdout, VERBOSE "%s", PROTOCOL_EIFLOW);
                  sfwrite(lock, stdout, DEFAULT "");
                  //printf(VERBOSE "%s" DEFAULT, PROTOCOL_EIFLOW);
                }
                send(fd,PROTOCOL_EIFLOW,strlen(PROTOCOL_EIFLOW),0); // MACRO NULL TERMINATED BY DEFAULT
                break;
    case BYE:    
                if (verbose){
                  sfwrite(lock, stdout, VERBOSE "%s", PROTOCOL_BYE);
                  sfwrite(lock, stdout, DEFAULT "");
                  //printf(VERBOSE "%s" DEFAULT, PROTOCOL_BYE);
                }
                send(fd,PROTOCOL_BYE,strlen(PROTOCOL_BYE),0); // MACRO NULL TERMINATED BY DEFAULT
                break;
    case IAM:   
                if(optionalString!=NULL){
                  	buildProtocolString(buffer, PROTOCOL_IAM, optionalString); 
                    if (verbose){
                      sfwrite(lock, stdout, VERBOSE "%s",  buffer);
                  sfwrite(lock, stdout, DEFAULT "");
                    }
                    //printf(VERBOSE "%s" DEFAULT, buffer);
                    send(fd, buffer,strlen(buffer),0);
                } // MACRO NULL TERMINATED BY DEFAULT
                break;

    case MOTD:   
                buildProtocolString(buffer, PROTOCOL_MOTD, messageOfTheDay);
                if (verbose){
                  sfwrite(lock, stdout, VERBOSE "%s",  buffer);
                  sfwrite(lock, stdout, DEFAULT "");
                  //printf(VERBOSE "%s" DEFAULT, buffer);
                }
                send(fd,buffer,strlen(buffer),0); // MACRO NULL TERMINATED BY DEFAULT
                break;
    case HI:   
                if(optionalString!=NULL){
                  	buildProtocolString(buffer, PROTOCOL_HI, optionalString); 
                    if (verbose){
                      sfwrite(lock, stdout, VERBOSE "%s",  buffer);
                  sfwrite(lock, stdout, DEFAULT "");
                    }
                        //printf(VERBOSE "%s" DEFAULT, buffer);
                    send(fd, buffer,strlen(buffer),0);
                }
                break;
    case LISTU:    
                if (verbose){
                  sfwrite(lock, stdout, VERBOSE "%s", PROTOCOL_LISTU);
                  sfwrite(lock, stdout, DEFAULT);
                  //printf(VERBOSE "%s" DEFAULT, PROTOCOL_LISTU);
                }
                send(fd,PROTOCOL_LISTU,strlen(PROTOCOL_LISTU),0); // MACRO NULL TERMINATED BY DEFAULT
                break;
    case UTSIL:   
                if(optionalString!=NULL){
                 	buildProtocolString(buffer, PROTOCOL_UTSIL, optionalString); 
                  if (verbose){
                    sfwrite(lock, stdout, VERBOSE "%s",  buffer);
                  sfwrite(lock, stdout, DEFAULT "");
                    //printf(VERBOSE "%s" DEFAULT, buffer);
                  }
                    send(fd, buffer,strlen(buffer),0);
                }
                break;
    case TIME:    
                if (verbose){
                  sfwrite(lock, stdout, VERBOSE "%s", PROTOCOL_TIME);
                  sfwrite(lock, stdout, DEFAULT "");
                  //printf(VERBOSE "%s" DEFAULT, PROTOCOL_TIME);
                }
                send(fd,PROTOCOL_TIME,strlen(PROTOCOL_TIME),0); // MACRO NULL TERMINATED BY DEFAULT
                break;
    case EMIT:   
                if(optionalString!=NULL){
                  buildProtocolString(buffer, PROTOCOL_EMIT, optionalString); 
                  if (verbose){
                    sfwrite(lock, stdout, VERBOSE "%s",  buffer);
                  sfwrite(lock, stdout, DEFAULT "");
                  }
                      //printf(VERBOSE "%s" DEFAULT, buffer);
                    send(fd, buffer,strlen(buffer),0);
                }
                break;
    case MSG:   
                buildMSGProtocol(buffer,optionalString,optionalString2, optionalString3); 
                if (verbose){
                  sfwrite(lock, stdout, VERBOSE "%s",  buffer);
                  sfwrite(lock, stdout, DEFAULT "");
                  //printf(VERBOSE "%s" DEFAULT, buffer);
                }
                send(fd,buffer,strlen(buffer),0); // MACRO NULL TERMINATED BY DEFAULT
                break;

    case UOFF:   
                if(optionalString!=NULL){
                  buildProtocolString(buffer, PROTOCOL_UOFF, optionalString); 
                  if (verbose){
                   sfwrite(lock, stdout, VERBOSE "%s",  buffer);
                  sfwrite(lock, stdout, DEFAULT "");
                  }
                        //printf(VERBOSE "%s" DEFAULT, buffer);
                    send(fd, buffer,strlen(buffer),0);
                }
                break;
    case IAMNEW:   
                if(optionalString!=NULL){
                  buildProtocolString(buffer, PROTOCOL_IAMNEW, optionalString); 
                  if (verbose){
                    sfwrite(lock, stdout, VERBOSE "%s",  buffer);
                  sfwrite(lock, stdout, DEFAULT "");
                  }
                        //printf(VERBOSE "%s" DEFAULT, buffer);
                    send(fd, buffer,strlen(buffer),0);
                }
                break;
    case HINEW:   
                if(optionalString!=NULL){
                  buildProtocolString(buffer, PROTOCOL_HINEW, optionalString); 
                  if (verbose){
                    sfwrite(lock, stdout, VERBOSE "%s",  buffer);
                  sfwrite(lock, stdout, DEFAULT "");
                  }
                       // printf(VERBOSE "%s" DEFAULT, buffer);
                    send(fd, buffer,strlen(buffer),0);
                }
                break;
    case NEWPASS:   
                if(optionalString!=NULL){
                  buildProtocolString(buffer, PROTOCOL_NEWPASS, optionalString); 
                  if (verbose){
                    sfwrite(lock, stdout, VERBOSE "%s",  buffer);
                  sfwrite(lock, stdout, DEFAULT "");
                  }
                        //printf(VERBOSE "%s" DEFAULT, buffer);
                    send(fd, buffer,strlen(buffer),0);
                }
                break;
    case SSAPWEN:    
                if (verbose){
                  sfwrite(lock, stdout, VERBOSE "%s", PROTOCOL_SSAPWEN);
                  sfwrite(lock, stdout, DEFAULT "");
                  //printf(VERBOSE "%s" DEFAULT, PROTOCOL_SSAPWEN);
                }
                send(fd,PROTOCOL_SSAPWEN,strlen(PROTOCOL_SSAPWEN),0); // MACRO NULL TERMINATED BY DEFAULT
                break;
    case AUTH:    
                if (verbose){
                  sfwrite(lock, stdout, VERBOSE "%s", PROTOCOL_AUTH);
                  sfwrite(lock, stdout, DEFAULT "");
                  //printf(VERBOSE "%s" DEFAULT, PROTOCOL_AUTH);
                }
                send(fd,PROTOCOL_AUTH,strlen(PROTOCOL_AUTH),0); // MACRO NULL TERMINATED BY DEFAULT
                break;
    case PASS:   
                if(optionalString!=NULL){
                  buildProtocolString(buffer, PROTOCOL_PASS, optionalString); 
                  if (verbose){
                    sfwrite(lock, stdout, VERBOSE "%s", buffer);
                  sfwrite(lock, stdout, DEFAULT "");
                  }
                        //printf(VERBOSE "%s" DEFAULT, buffer);
                    send(fd, buffer,strlen(buffer),0);
                }
                break;
    case SSAP:    
                if (verbose){
                  sfwrite(lock, stdout, VERBOSE "%s", PROTOCOL_SSAP);
                  sfwrite(lock, stdout, DEFAULT "");
                  //printf(VERBOSE "%s" DEFAULT, PROTOCOL_SSAP);
                }
                send(fd,PROTOCOL_SSAP,strlen(PROTOCOL_SSAP),0); // MACRO NULL TERMINATED BY DEFAULT
                break;
    case ERR1:    
                if (verbose){
                  sfwrite(lock, stdout, ERROR "%s", PROTOCOL_ERR1);
                  sfwrite(lock, stdout, DEFAULT "");
                  //printf(ERROR "%s" DEFAULT, PROTOCOL_ERR1);
                }
                send(fd,PROTOCOL_ERR1,strlen(PROTOCOL_ERR1),0); // MACRO NULL TERMINATED BY DEFAULT
                break;
    case ERR2:    
                if (verbose){
                  sfwrite(lock, stdout, ERROR "%s", PROTOCOL_ERR2 );
                  sfwrite(lock, stdout, DEFAULT "");
                  //printf(ERROR "%s" DEFAULT, PROTOCOL_ERR2);
                }
                send(fd,PROTOCOL_ERR2,strlen(PROTOCOL_ERR2),0); // MACRO NULL TERMINATED BY DEFAULT
                break;
    case ERR100:   
                if (verbose){
                  sfwrite(lock, stdout, VERBOSE "%s", PROTOCOL_ERR100 );
                  sfwrite(lock, stdout, DEFAULT "");
                  //printf(ERROR "%s" DEFAULT, PROTOCOL_ERR100); 
                }
                send(fd,PROTOCOL_ERR100,strlen(PROTOCOL_ERR100),0); // MACRO NULL TERMINATED BY DEFAULT
                break;
    case ERR0:    
                if (verbose){
                  sfwrite(lock, stdout, ERROR "%s", PROTOCOL_ERR0);
                  sfwrite(lock, stdout, DEFAULT "");
                  //printf(ERROR "%s" DEFAULT, PROTOCOL_ERR0);
                }
                send(fd,PROTOCOL_ERR0,strlen(PROTOCOL_ERR0),0); // MACRO NULL TERMINATED BY DEFAULT
                break;
  }

}





