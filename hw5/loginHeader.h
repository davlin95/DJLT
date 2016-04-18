
bool validUsername(char *username){
  char *charPtr = username;
  while (*charPtr != '\0'){
    if (*charPtr < 33 || *charPtr > 126){
      return false;
    }
    charPtr++;\
  }
  return true;
}

char* protocol_IAM_Helper(char* string, char *userBuffer){
    char * savePtr;
    char *token;
    int arrayIndex = 0;
    char * protocolArray[1024];
    char tempString[1024];
    memset(&tempString, 0, 1024);
    memset(&protocolArray, 0, 1024);

    strcpy(tempString, string);
    token = strtok_r(tempString, " ", &savePtr);
    while (token != NULL){
      protocolArray[arrayIndex++] = token;
      token = strtok_r(NULL," ",&savePtr);
    }
    if (arrayIndex > 3 || strcmp(protocolArray[0], PROTOCOL_IAM)!=0 || strcmp(protocolArray[2],"\r\n\r\n")!=0)
      return NULL;
    strcpy(userBuffer, protocolArray[1]);
    if (validUsername(userBuffer)!=0){
      return userBuffer;
    }
    return NULL;
}

void processValidClient(char* clientUserName){
  /*********** CREATE CLIENT STRUCT **********/
  Client* newClient = createClient();
  setClientUserName(newClient,clientUserName);
  startSession(newClient->session);
  addClientToList(newClient);
}

char* performLoginProcedure(int fd,char* userBuffer){
  //INITIALIZE
  char protocolBuffer[1024];
  memset(&protocolBuffer,0,1024);

  //WAIT FOR WOLFIE
  printf("waiting for WOLFIE\n");
  int bytes=1;
  bytes = read(fd,&protocolBuffer,1024);
  printf("read in response\n");
  if(strcmp(protocolBuffer,PROTOCOL_WOLFIE)!=0){
    printf("reading invalid protocol response from client: %s ...",protocolBuffer);
    return NULL;
  }else{
    protocolMethod(fd,EIFLOW,NULL);
    printf("sent elflow to client\n");
  }

  /*memset the protocol buffer so it can be reused for second verb*/
  memset(&protocolBuffer,0,1024);
  bytes =-1;
  printf("reading response to EIFLOW\n");
  bytes = read(fd,&protocolBuffer,1024);
  printf("testing response to IAM:%s\n",(char*)protocolBuffer);
  if (protocol_IAM_Helper(protocolBuffer,userBuffer)== NULL){
    printf("protocolIAM failed\n");
    return NULL;
  }
  printf("USER BUFFER IS :%s",userBuffer);
  if (returnClientData(userBuffer)==NULL){
    printf("client has available username \n");
    protocolMethod(fd, HI, userBuffer);
    printf("sent hi to client %d\n",fd);
    sendMessageOfTheDay(fd);
    return userBuffer;
  }
  protocolMethod(fd, ERR0, NULL);
  protocolMethod(fd, BYE, NULL);
  return NULL;
}