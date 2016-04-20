
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
  Client* newClient = createClient();
  setClientUserName(newClient,clientUserName);
  startSession(newClient->session);
  newClient->session->commSocket = fd;
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
  if(strcmp(protocolBuffer, PROTOCOL_WOLFIE)!=0){
    return NULL;
  }else{
    protocolMethod(fd,EIFLOW,NULL,NULL,NULL);
    printf("sent elflow to client\n");
  }

  /*memset the protocol buffer so it can be reused for second verb*/
  memset(&protocolBuffer,0,1024);
  bytes =-1;

  bytes = read(fd,&protocolBuffer,1024);
  if (protocol_IAM_Helper(protocolBuffer, userBuffer) != NULL){
    if (getClientByUsername(userBuffer)==NULL){
      protocolMethod(fd, HI, userBuffer,NULL,NULL);
      sendMessageOfTheDay(fd);
      return userBuffer;
    }
  }
  protocolMethod(fd, ERR0, NULL,NULL,NULL);
  protocolMethod(fd, BYE, NULL,NULL,NULL);
  return NULL; 
}