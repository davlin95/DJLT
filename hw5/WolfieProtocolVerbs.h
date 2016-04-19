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

bool checkVerb(char *protocolVerb, char* string){
  if (strncmp(string, protocolVerb, strlen(protocolVerb)) == 0)
    return true;
  return false;
}

bool extractArgAndTest(char *string, char *buffer){
    char * savePtr;
    char *token;
    int arrayIndex = 0;
    char tempString[1024];
    char * protocolArray[1024];
    memset(&tempString, 0, 1024);
    memset(&protocolArray, 0, 1024);
    strcpy(tempString, string);
    token = strtok_r(tempString, " ", &savePtr);
    while (token != NULL){
      protocolArray[arrayIndex++] = token;
      token = strtok_r(NULL," ",&savePtr);
    }
    if (arrayIndex > 3 || strcmp(protocolArray[2], "\r\n\r\n")!=0)
      return false;
    strcpy(buffer, protocolArray[1]);
    return true;
}

bool buildProtocolString(char* buffer, char* protocol, char* middle){
  if(buffer==NULL) {printf("buffer is null\n"); return 0;}
  printf("middle is %s\n", middle);
  strcat(buffer,protocol);
  strcat(buffer," ");
  strcat(buffer, middle);
  strcat(buffer," \r\n\r\n");
  return 1;
}

void protocolMethod(int fd, int wolfieVerb, char* optionalString){
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
    /*case MSG:   
                send(fd,PROTOCOL_BYE,strlen(PROTOCOL_BYE),0); // MACRO NULL TERMINATED BY DEFAULT
                break;
    case UOFF:   
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