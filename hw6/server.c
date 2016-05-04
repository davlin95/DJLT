#define _GNU_SOURCE
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>  
#include <sys/socket.h>
#include <netinet/in.h>  
#include <string.h> 
#include <errno.h>   
#include <netdb.h>
#include <sys/types.h>
#include <sys/poll.h> 
#include <sys/fcntl.h>  
#include <signal.h>
#include "serverHeader.h" 
#include "loginHeader.h"    


int main(int argc, char* argv[]){ 
  int argCounter; 
  char *portNumber;
  char * argArray[1024];
  char * flagArray[1024]; 
  memset(&argArray, 0, sizeof(argArray));
  memset(&flagArray, 0, sizeof(flagArray));
  
  //INITIALIZE DATABASE VARIABLES
  
  char *dbErrorMessage = 0;
  sqlite3 *database;
  int dbResult;
  char *sql;
  
  /**************************/
  /*  PARSE ARGS FOR FLAGS */
  /*************************/
  initFlagArray(argc, argv, flagArray);
  argCounter = 0;
  while (flagArray[argCounter] != NULL){
    if (strcmp(flagArray[argCounter], "-h")==0){   
      USAGE("./server");
      exit(EXIT_SUCCESS);
    }
    if (strcmp(flagArray[argCounter], "-v")==0){
      verbose = true;
    }
    argCounter++;   
  }

  //INIT ARGUMENTS INTO ARRAY 
  argCounter = initArgArray(argc, argv, argArray);
  if (argCounter < 4 || argCounter > 5){
    USAGE("./server"); 
    exit(EXIT_FAILURE);  
  } 

  /**************************************************/
  /* User entered DB Path With 5TH ELEMENT -C FLAG */
  /************************************************/
  if (argCounter == 5){
    //getAccounts(argArray[3]);
  //}  
    /*last_two = (void *)argArray[3] + strlen(argArray[3]) - 2; 
    if (strcmp(last_two, "db")!=0){
      FILE *accountsFile;
      size_t size = 1024;
      char username[1024];
      char password[1024];
      char salt[1024];
      char *usernamePtr = username;
      char *passwordPtr = password;
      char *saltPtr = salt;
      char *lastChar;
      memset(&username, 0, 1024);
      memset(&password, 0, 1024);
      memset(&salt, 0, 1024);
      accountsFile = fopen(argArray[3], "r");
      if (accountsFile != NULL){
        while(getline(&usernamePtr, &size, accountsFile) > 0){
          lastChar = strchr(usernamePtr, '\n');
          *lastChar = '\0';
          if (getline(&passwordPtr, &size, accountsFile) > 0){
            lastChar = strchr(passwordPtr, '\n');
            *lastChar = '\0';
          } else{
            fprintf(stderr, "Invalid Accounts File\n");
            exit(0);
          }

          if (getline(&saltPtr, &size, accountsFile) > 0){
            lastChar = strchr(saltPtr, '\n');
            *lastChar = '\0';
          } else{
            fprintf(stderr, "Invalid Accounts File\n");
            exit(0);
          }
          processValidAccount(usernamePtr, passwordPtr, saltPtr);
          memset(&username, 0, 1024);
          memset(&password, 0, 1024);
          memset(&salt, 0, 1024);
        }
        fclose(accountsFile);
      }
    }  */
     
    //OPEN DATABASE
        if ((dbResult = sqlite3_open(argArray[3], &database))){
          fprintf(stderr, "Error opening database: %s\n", sqlite3_errmsg(database));
          exit(0);
        }
      //GET ACCOUNT INFO 
        sql = "SELECT * FROM ACCOUNTS";
        if ((dbResult = sqlite3_exec(database, sql, callback, 0, &dbErrorMessage)) != SQLITE_OK){
          fprintf(stderr, "SQL Error: %s\n", dbErrorMessage);
          sqlite3_free(dbErrorMessage);
          exit(0);
        } 
    }
  /*********************************************/
  /*  DB PATH BUT NO 5TH ELEMENT -C FLAG      */
  /*******************************************/
  
    //database = initDataBase(database);
    //OPEN DATABASE
    else{
    if ((dbResult = sqlite3_open("accounts.db", &database))){
      fprintf(stderr, "Error opening database: %s\n", sqlite3_errmsg(database));
      exit(0);
    }
    //NO PREVIOUS DB, CREATE NEW ACCOUNTS
    sql = "CREATE TABLE ACCOUNTS("                        \
          "username     CHAR(1025) PRIMARY KEY NOT NULL," \
          "password     CHAR(1025) NOT NULL,"             \
          "salt         CHAR(1025) NOT NULL);";
    if ((dbResult = sqlite3_exec(database, sql, callback, 0, &dbErrorMessage)) != SQLITE_OK){
      fprintf(stderr, "SQL Error: %s\n", dbErrorMessage);
      sqlite3_free(dbErrorMessage);
      //exit(0);
    }
  }

  /***************************************/
  /* EXTRACT ARGV FOR IMPORTANT STRINGS */
  /*************************************/
  portNumber = argArray[1];   
  strcpy(messageOfTheDay, argArray[2]); 


  /***********************************/
  /* PROGRAM THREAD ARR AND SIGNALS  */  
  /**********************************/
  int threadStatus,threadNum=0;
  pthread_t threadId[1026];  
  signal(SIGINT,killServerHandler); 
  commThreadId=-1;

  /*******************/
  /* Create Socket  */ 
  /*****************/ 
  int connfd;    
  if ((serverFd = createBindListen(portNumber, serverFd))<0){
    fprintf(stderr, ERROR PROTOCOL_ERR100 DEFAULT);
    exit(EXIT_FAILURE);        
  }else{
    printf("Currently listening on port %s\n", portNumber);  
  }

  /**********************************************************************/
  /*  IMPLEMENT POLL: USED LATER IN COMMUNICATION/LOGIN                */
  /********************************************************************/ 
  /* Initialize Polls Interface*/
  memset(pollFds, 0 , sizeof(pollFds));
  pollNum =1;

  //CREATE GLOBAL SOCKET-READ PIPE PAIR
  int socketPair[2]={0,0};
  createSocketPair(socketPair,2);
  if(socketPair[0]>0 && socketPair[1]>0){
    makeNonBlocking(socketPair[0]);
    makeNonBlocking(socketPair[1]);

    //ADD TO WATCHED FD SET
    pollFds[0].fd= socketPair[0];
    pollFds[0].events = POLLIN;
    globalSocket = socketPair[1];
  }

  /******************************************/
  /*        IMPLEMENT ACCEPT POLL          */
  /****************************************/ 
  /* Initialize Polls Interface*/
  struct pollfd acceptPollFds[2];
  memset(acceptPollFds, 0 , sizeof(acceptPollFds));

  /* Set poll for serverFd */  
  acceptPollFds[0].fd = serverFd;
  acceptPollFds[0].events = POLLIN;
 
  /* Set poll for stdin */
  makeNonBlocking(0);
  acceptPollFds[1].fd = 0;
  acceptPollFds[1].events = POLLIN;


  /********************/
  /* ETERNAL POLL    */
  /******************/
  int acceptPollStatus;
  while(1){
    acceptPollStatus = poll(acceptPollFds, 2, -1);
    if(acceptPollStatus<0){
      fprintf(stderr,"poll():%s",strerror(errno)); 
      break; // exit due to poll error
    }

    /******************************************************/
    /* EVENT TRIGGERED: CHECK ALL POLLS FOR OCCURED EVENT */
    /*****************************************************/
    int i;
    for(i=0;i<2;i++){
      if(acceptPollFds[i].revents==0) continue;
      if(acceptPollFds[i].revents!=POLLIN){ 
        fprintf(stderr,"poll.revents:%s",strerror(errno));
        break;
      }
      /***********************************/  
      /*   POLLIN FROM SERVERFD         */
      /*********************************/
      if(acceptPollFds[i].fd == serverFd){

        while(1){
          /************* STORE INCOMING CONNECTS ****/
          struct sockaddr_storage newClientStorage;
          socklen_t addr_size = sizeof(newClientStorage);

          //MALLOC AND STORE THE CONNFD, IF VALID CONNFD
          int* connfdPtr = malloc(sizeof(int));
          connfd = accept(serverFd,(struct sockaddr *) &newClientStorage, &addr_size);
            
          if(connfd<0){
            if(errno!=EWOULDBLOCK){
              fprintf(stderr,"accept() in poll loop: %s\n",strerror(errno));
              close(connfd); 
            }
            break; 
          }
          *connfdPtr=connfd; // pass pointer to malloced int to login thread 

          /************** LOGIN THREAD FOR EVERY CONNFD *******/
          threadStatus = pthread_create(&threadId[threadNum++], NULL, &loginThread, connfdPtr);
          pthread_setname_np(threadId[threadNum-1],"LOGIN THREAD");
          if(threadStatus<0){
            close(connfd);
            free(connfdPtr);
            fprintf(stderr,"Error spawning login thread for descriptor %d\n",connfd);
          }
        }
      }
      /***********************************/
      /*   POLLIN FROM STDIN            */
      /*********************************/
      else if(acceptPollFds[i].fd == 0){
        int bytes=0;
        char stdinBuffer[1024];
        memset(&stdinBuffer,0,1024);

        /*******************************/
        /* EXECUTE STDIN COMMANDS     */
        /*****************************/
        while( (bytes=read(0,&stdinBuffer,1024))>0){  
            if (strcmp(stdinBuffer, "/shutdown\n")==0){
              /*char account[] = "accounts";
              char * accounts;
              if (argCounter == 5)
                accounts = argArray[3];
              else
                accounts = account;
              saveAccounts(accounts);*/
              
              // ATTEMPT SQL SHUTDOWN
                char sqlInsert[1024];
                Account * accountPtr;
                sql = "DELETE FROM ACCOUNTS;";
              //ATTEMPT TO CLEAR USERS
                if ((dbResult = sqlite3_exec(database, sql, callback, 0, &dbErrorMessage)) != SQLITE_OK){
                    fprintf(stderr, "SQL Error: %s\n", dbErrorMessage);
                    //SHUTDOWN PROCEDURES
                    sqlite3_free(dbErrorMessage);
                }
                for (accountPtr = accountHead; accountPtr!=NULL; accountPtr = accountPtr->next){
                    memset(&sqlInsert, 0, 1024);
                    sql = createSQLInsert(accountPtr, sqlInsert);
                    printf("sql = %s\n", sql);
                    if ((dbResult = sqlite3_exec(database, sql, callback, 0, &dbErrorMessage)) != SQLITE_OK){
                        fprintf(stderr, "SQL Error: %s\n", dbErrorMessage);
                  //SHUTDOWN PROCEDURES
                        sqlite3_free(dbErrorMessage);
                    }
                }
                sqlite3_close(database);
                /*FILE *accountsFile;
                if (argCounter == 5){
                    accountsFile = fopen(argArray[3], "w");
                }
                else{
                  accountsFile = fopen("accounts", "w");
                }
                if (accountsFile != NULL){
                  Account *accountPtr;
                  for (accountPtr = accountHead; accountPtr!=NULL; accountPtr = accountPtr->next){
                    printf("username is %s\n", accountPtr->userName);
                    fprintf(accountsFile, "%s\n", accountPtr->userName);
                    fprintf(accountsFile, "%s\n", accountPtr->password);
                    fprintf(accountsFile, "%s\n", accountPtr->salt);
                  }
                fclose(accountsFile);
                }*/
              processShutdown();
          }
          //EXECUTE OTHER COMMANDS
          recognizeAndExecuteStdin(stdinBuffer);
          memset(&stdinBuffer,0,strnlen(stdinBuffer,1024));
        }  
      }
    }
  }

  /************************/
  /* FINAL EXIT CLEANUP  */
  /**********************/ 
  disconnectAllUsers();
  return 0;
}    
 
/**********************/
/*     LOGIN THREAD  */ 
/********************/
 
void* loginThread(void* args){ 
  //CONNFD COPY OF THE ONE ON THE HEAP
  int connfd = *(int *)args; 

  //USER INFO
  int user = 0;      
  int *newUser = &user; 
  char username[1024]; 
  memset(&username, 0, 1024); 
  char password[1024];
  memset(&password, 0, 1024);

  /*************** NONBLOCK CONNFD SET TO GLOBAL CLIENT LIST *********/
  if (performLoginProcedure(connfd, username, password, newUser)){

    /****  CREATE AND PROCESS CLIENT ****/
    if (makeNonBlocking(connfd)<0){
      fprintf(stderr, "Error making connection socket nonblocking.\n");
    }   
    pollFds[pollNum].fd = connfd;
    pollFds[pollNum].events = POLLIN;
    pollNum++; 


    /***********************************/
    /* NEW USER NEEDS ACCOUNT CREATED */
    /*********************************/
    if (user){
      unsigned char saltBuffer[1024];
      unsigned char passwordHash[1024]; 
      memset(&saltBuffer, 0, 1024);
      memset(&passwordHash, 0, 1024);

      //CREATE RANDOM NUMBER SALT 
      if (RAND_bytes(saltBuffer, 10) == 0){
        fprintf(stderr, "LoginThread(): Error creating salt\n");
      }

      //APPEND SALT AND HASH IT 
      strcat(password, (char*)saltBuffer);
      sha256(password, passwordHash); 

      //STORE IT IN NEW ACCOUNT STRUCT INTO GLOBAL LIST
      processValidAccount(username, (char *)passwordHash, (char *)saltBuffer);
    }
    //ADD CLIENT TO ACTIVE LIST OF USERS
    processValidClient(username,connfd);

     /****************************************************/
    /* IF COMMUNICATION THREAD NEEDED FOR MULTIPLEXING  */
    /***************************************************/
    if(pollNum<3){
      if(pthread_create(&commThreadId, NULL, &communicationThread, NULL)<0){
        close(connfd);
        fprintf(stderr,"Error spawning communicationThread for descriptor %d\n",connfd);
        return NULL;
      }else{
        pthread_setname_np(commThreadId,"COMMUNICATION THREAD");
      }
    }

    /* Trigger the poll forward in the communication thread*/
    if(globalSocket>0){
      write(globalSocket," ",1);
    }
  }
  /**********************/
  /* USER FAILED LOGIN */
  /********************/
  else {
    writeToGlobalSocket();
    close(connfd);
    free( ((int *)args)  ); // FREE MALLOCED THE HEAP INT 
  }
  //EXIT THREAD 
  return NULL; 
}

/********************************************/
/*  COMMUNICATION THREAD                    */
/********************************************/
void* communicationThread(void* args){
  int pollStatus, compactDescriptors=0; //@todo compactDescriptors should it be global
  /*********************/
  /*   ETERNAL WHILE  */
  /*******************/
  while(1){
    pollStatus = poll(pollFds, pollNum, -1);
    if(pollStatus<0){
      fprintf(stderr,"poll():%s",strerror(errno)); 
      break; // exit due to poll error
    }

    /******************************************************/
    /* EVENT TRIGGERED: CHECK ALL POLLS FOR OCCURED EVENT */
    /*****************************************************/
    int i;
    for(i=0;i<pollNum;i++){
      if(pollFds[i].revents==0) continue;
      if(pollFds[i].revents!=POLLIN){ 
        fprintf(stderr,"poll.revents:%s",strerror(errno));
        break;
      }
      /********************************************/
      /* LOOP FORWARD DUE TO GLOBAL SOCKET WRITE */
      /******************************************/
      else if(pollFds[i].fd == pollFds[0].fd ){
        char globalByte;
        read(pollFds[0].fd,&globalByte,1);
      }
      /**************************************/
      /*   POLLIN: PREVIOUS CLIENT         */
      /************************************/
      else{
        int doneReading=0;
        //int bytes;
        char clientMessage[1024];

        /***********************/  
        /* READ FROM CLIENT   */
        /*********************/

        /*** STORE CLIENT BYTES INTO BUFFER ****/
        memset(&clientMessage,0, 1024);
        if(ReadNonBlockedSocket(pollFds[i].fd ,clientMessage)==false){
          fprintf(stderr,"ReadNonBlockedSocket(): Detected Socket Closed\n");
          memset(&clientMessage,0, 1024);
          ReadNonBlockedSocket(pollFds[i].fd ,clientMessage);
          doneReading=1;
        }

        /****************************/ 
        /* CLIENT BUILT IN REQUESTS */ 
        /***************************/
        if (checkVerb(PROTOCOL_TIME, clientMessage)){  
          if (verbose){
            printf(VERBOSE "%s" DEFAULT, clientMessage);
          }
          char sessionlength[1024];
          memset(&sessionlength, 0, 1024);
          if (sessionLength(getClientByFd(pollFds[i].fd), sessionlength)){ 
            protocolMethod(pollFds[i].fd, EMIT, sessionlength, NULL,NULL, verbose); 
          }    
        }  
        else if (checkVerb(PROTOCOL_LISTU, clientMessage)){
          if (verbose){
            printf(VERBOSE "%s" DEFAULT, clientMessage);
          }
          char usersBuffer[1024];
          memset(&usersBuffer, 0, 1024); 
          if (buildUtsilArg(usersBuffer)){   
            protocolMethod(pollFds[i].fd, UTSIL, usersBuffer,NULL,NULL, verbose);  
          }
        } 
        else if (checkVerb(PROTOCOL_BYE, clientMessage)){ 
          if (verbose){
            printf(VERBOSE "%s" DEFAULT, clientMessage);
          }               
          doneReading=1;  
        }
        else if(extractArgAndTestMSG(clientMessage,NULL,NULL,NULL) ){
          if (verbose){
            printf(VERBOSE "%s" DEFAULT, clientMessage);
          }                
          char msgToBuffer[1024];
          char msgFromBuffer[1024];
          char msgBuffer[1024];

          memset(&msgToBuffer,0,1024);
          memset(&msgFromBuffer,0,1024); 
          memset(&msgBuffer,0,1024);

          extractArgAndTestMSG(clientMessage,msgToBuffer,msgFromBuffer,msgBuffer);
          Client* toUser= getClientByUsername(msgToBuffer);
            
          //CHECK IF THE TO-USER IS STILL LOGGED ON, OR IF THEY EXIST, USER NOT MESSAGING SELF
          if( toUser!=NULL && strcmp(msgToBuffer, msgFromBuffer)!=0 ){
            //SEND MESSAGE BACK
            char messageResponse[1024];
            memset(&messageResponse,0,1024);
            buildMSGProtocol(messageResponse,msgToBuffer,msgFromBuffer,msgBuffer);
            //SEND TO BOTH USERS 
            send(toUser->session->commSocket,messageResponse,strnlen(messageResponse,1023),0);
            send(pollFds[i].fd,messageResponse,strnlen(messageResponse,1023),0);
          }else{ 
            //FAILED SEND, ERROR BACK TO USER
            protocolMethod(pollFds[i].fd,ERR1,NULL,NULL,NULL, verbose);
          } 
        }
        /*******************************************************/
        /* OUTPUT MESSAGE FROM CLIENT : DELETE AFTER TESTING  */
        /*****************************************************/
 
        /********************************/
        /* IF CLIENT LOGGED OFF        */ 
        /******************************/ 
        if(doneReading){ 
          char username[1024];
          memset(&username, 0, 1024);
          strcpy(username, getClientByFd(pollFds[i].fd)->userName);
          disconnectUser(username); 
          //NOTIFY EVERY USER THAT THE USER LOGGED OFF
          notifyAllUsersUOFF(username);

          //CHANGE POLL TO REFLECT USER LOGGED OFF
          pollFds[i].fd=-1;
          compactDescriptors=1;
        } 
      }
    }// MOVE ON TO NEXT POLL FD EVENT
      
    /* COMPACT POLLS ARRAY BEFORE NEXT CHECK OF FORLOOP*/
    if(compactDescriptors){
      compactDescriptors=0;
      compactPollDescriptors();
    } 
    /* LEAVE THREAD IF THERE'S NO CLIENTS LEFT */
    if(pollNum<=1){
      break;
    }
  }/* FOREVER RUNNING LOOP */ 

  /*******************/
  /* if exit, reset */
  /******************/
  commThreadId=-1; 
  return NULL;
}