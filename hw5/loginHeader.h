
bool validUsername(char *username){
  char *charPtr = username;
  while (*charPtr != '\0'){
    if (*charPtr < 33 || *charPtr > 126){
      return false;
    }
    charPtr++;
  }
  return true;
}

char* protocol_IAM_Helper(char* string, char *userBuffer){
    if (checkVerb(PROTOCOL_IAM, string)){
      if (extractArgAndTest(string, userBuffer)){
        if (validUsername(userBuffer))
          return userBuffer;
      }
    }
    return NULL;
}

void processValidClient(char* clientUserName, int fd){
  /*********** CREATE CLIENT STRUCT **********/
  Client* newClient = createClient();
  setClientUserName(newClient,clientUserName);
  startSession(newClient->session);
  newClient->session->commSocket = fd;
  addClientToList(newClient);
}

char* performLoginProcedure(int fd,char* userBuffer){
  char protocolBuffer[1024];
  memset(&protocolBuffer,0,1024);
  int bytes=-1;
  bytes = read(fd,&protocolBuffer,1024);
  printf("protocolBuffer contains: %s\n", protocolBuffer);
  if(strcmp(protocolBuffer, PROTOCOL_WOLFIE)!=0){
    return NULL;
  }else{
    protocolMethod(fd,EIFLOW,NULL);
  }
  /*memset the protocol buffer so it can be reused for second verb*/
  memset(&protocolBuffer,0,1024);
  bytes =-1;
  bytes = read(fd,&protocolBuffer,1024);
  printf("protocolBuffer contains: %s\n", protocolBuffer);
  if (protocol_IAM_Helper(protocolBuffer, userBuffer) != NULL){
    if (getClientByUsername(userBuffer)==NULL){
    protocolMethod(fd, HI, userBuffer);
    sendMessageOfTheDay(fd);
    return userBuffer;
    }
  }
  protocolMethod(fd, ERR0, NULL);
  protocolMethod(fd, BYE, NULL);
  return NULL; 
}