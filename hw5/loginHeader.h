
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

bool protocol_Login_Helper(char* verb, char* string, char *userBuffer){
    if (checkVerb(verb, string)){
      if (extractArgAndTest(string, userBuffer)){
        return true;
      }
    }
    return false;
}

bool performLoginProcedure(int fd,char* userBuffer, char* passBuffer, int *newUser){
  //----------------------------------||
  //     READ RESPONSE FROM CLIENT    ||                       
  //----------------------------------||  
  char protocolBuffer[1024];
  memset(&protocolBuffer,0,1024);
  if (read(fd, &protocolBuffer,1024) < 0){
    fprintf(stderr,"Read(): bytes read negative\n");
    return false;
  }

  //-----------------------------------------------|| 
  //      CHECK IF EXPECTED: WOLFIE \r\n\r\n,      ||
  //-----------------------------------------------|| 
  if(checkVerb(PROTOCOL_WOLFIE, protocolBuffer)){

    if (verbose){
      printf(VERBOSE "%s" DEFAULT, protocolBuffer);
    }
    protocolMethod(fd,EIFLOW,NULL,NULL,NULL, verbose);
  }
  else{
    fprintf(stderr, "Expected protocol verb WOLFIE\n");
    return false;
  }
  //-----------------------------------------||
  //     READ NEXT RESPONSE FROM CLIENT      ||                       
  //-----------------------------------------||
  memset(&protocolBuffer,0,1024);
  if (read(fd, &protocolBuffer,1024) < 0){
    fprintf(stderr,"Read(): bytes read negative\n");
    return false;
  }
  if (verbose)
      printf(VERBOSE "%s" DEFAULT, protocolBuffer);
  //-------------------------------------------------------|| 
  //    CHECK IF RESPONSE: IAMNEW <username> \r\n\r\n      ||  
  //-------------------------------------------------------||
  if (protocol_Login_Helper(PROTOCOL_IAMNEW, protocolBuffer, userBuffer)){
    *newUser = true;
    if (validUsername(userBuffer) && getAccountByUsername(userBuffer)==NULL){
      protocolMethod(fd, HINEW, userBuffer,NULL,NULL, verbose);
    }
    else{
      protocolMethod(fd, ERR0, NULL,NULL,NULL, verbose);
      protocolMethod(fd, BYE, NULL,NULL,NULL, verbose);
      fprintf(stderr, "Invalid Username or account already exists.\n");
      return false; 
    }
  } 
  //--------------------------------------------------||
  //    CHECK IF RESPONSE: IAM <username> \r\n\r\n    || 
  //--------------------------------------------------||
  else if (protocol_Login_Helper(PROTOCOL_IAM, protocolBuffer, userBuffer)){
    if (getAccountByUsername(userBuffer)!=NULL){
      if(getClientByUsername(userBuffer)==NULL){
        protocolMethod(fd, AUTH, NULL, NULL, NULL, verbose);
      } 
      else{
        protocolMethod(fd, ERR0, NULL,NULL,NULL,verbose);
        protocolMethod(fd, BYE, NULL,NULL,NULL, verbose);
        fprintf(stderr, "User already signed in.\n");
        return false;
      }
    }
    else{
      protocolMethod(fd, ERR1, NULL,NULL,NULL, verbose);
      protocolMethod(fd, BYE, NULL,NULL,NULL, verbose);
      fprintf(stderr, "Account doesn't exist.\n");
      return false;
    }
  }
  else {
    fprintf(stderr, "Expected protocol verb IAM or IAMNEW\n");
    return false;
  }
  //-----------------------------------------||
  //     READ NEXT RESPONSE FROM CLIENT      ||                       
  //-----------------------------------------||
  /*
  if (Read(fd, protocolBuffer))
    return false;
  */
  memset(&protocolBuffer,0,1024);
  if (read(fd, &protocolBuffer,1024) < 0){
    fprintf(stderr,"Read(): bytes read negative\n");
    return false;
  }
  if (verbose)
      printf(VERBOSE "%s" DEFAULT, protocolBuffer);
  //------------------------------------------------------||
  //    CHECK IF RESPONSE: NEWPASS <password> \r\n\r\n    || 
  //------------------------------------------------------||
  if (protocol_Login_Helper(PROTOCOL_NEWPASS, protocolBuffer, passBuffer)){
    if (validPassword(passBuffer)){
      protocolMethod(fd, SSAPWEN, NULL, NULL, NULL, verbose);
      protocolMethod(fd, HI, userBuffer, NULL, NULL, verbose);
      protocolMethod(fd, MOTD, NULL, NULL, NULL, verbose);
      return true;
    }
    else{
      protocolMethod(fd, ERR2, NULL, NULL, NULL, verbose);
      protocolMethod(fd, BYE, NULL, NULL, NULL, verbose);
      fprintf(stderr, "Invalid Password\n");
      return false;
    }
  }

  //---------------------------------------------------||
  //    CHECK IF RESPONSE: PASS <password> \r\n\r\n    || 
  //---------------------------------------------------||
  else if (protocol_Login_Helper(PROTOCOL_PASS, protocolBuffer, passBuffer)){
    char *salt;
    unsigned char hashBuffer[1024];
    memset(&hashBuffer, 0, 1024);
    salt = getAccountByUsername(userBuffer)->salt;
    strcat(passBuffer, salt);
    sha256(passBuffer, hashBuffer);
    if (strcmp((getAccountByUsername(userBuffer))->password, (char*)hashBuffer)==0){
      protocolMethod(fd, SSAP, NULL, NULL, NULL, verbose);
      protocolMethod(fd, HI, userBuffer, NULL, NULL, verbose);
      protocolMethod(fd, MOTD, NULL, NULL, NULL, verbose);
      return true;
    }
    else{
      protocolMethod(fd, ERR2, NULL, NULL, NULL, verbose);
      protocolMethod(fd, BYE, NULL, NULL, NULL, verbose);
      fprintf(stderr, "Invalid Password\n");
      return false;
    }
  }
  else{
    fprintf(stderr, "expected NEWPASS or PASS\n");
  }
  return false;
}