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
#include "../../hw5/serverHeader.h" 
#include "../../hw5/loginHeader.h"    


int main(int argc, char* argv[]){ 
  int argCounter; 
  char *portNumber;
  char * argArray[1024];
  char * flagArray[1024]; 
  memset(&argArray, 0, sizeof(argArray));
  memset(&flagArray, 0, sizeof(flagArray));
  
  //INITIALIZE DATABASE VARIABLES
  sqlite3 *database;
  char *dbErrorMessage = 0;
  int dbResult;
  char *sql;

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
  else{
    //OPEN DATABASE
    if ((dbResult = sqlite3_open("accounts.db", &database))){
      fprintf(stderr, "Error opening database: %s\n", sqlite3_errmsg(database));
      exit(0);
    }
    //NO PREVIOUS DB, CREATE NEW ACCOUNTS
    sql = "CREATE TABLE ACCOUNTS("                        \
          "username     CHAR(1024) PRIMARY KEY NOT NULL," \
          "password     CHAR(1024) NOT NULL,"             \
          "salt         CHAR(1024) NOT NULL);";
    if ((dbResult = sqlite3_exec(database, sql, callback, 0, &dbErrorMessage)) != SQLITE_OK){
      fprintf(stderr, "SQL Error: %s\n", dbErrorMessage);
      sqlite3_free(dbErrorMessage);
      exit(0);
    }
  }

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


  /*******************/
  /* Create Socket  */ 
  /*****************/ 
  int connfd;    
  if ((serverFd = createBindListen(portNumber, serverFd))<0){
    printf("error createBindListen\n");
    exit(EXIT_FAILURE);       
  }else{
    printf("Listening\n"); 
  }

    /******************************************/
    /*        IMPLEMENT POLL                 */
    /****************************************/ 
    /* Initialize Polls Interface*/
    memset(pollFds, 0 , sizeof(pollFds));
    pollNum = 3;
    int pollStatus,compactDescriptors=0;

    /* Set poll for serverFd */
    pollFds[0].fd = serverFd;
    pollFds[0].events = POLLIN;

    /* Set poll for stdin */
    makeNonBlocking(0);
    pollFds[1].fd = 0;
    pollFds[1].events = POLLIN;

    //CREATE GLOBAL SOCKET-READ PIPE PAIR
    int socketPair[2]={0,0};
    createSocketPair(socketPair,2);
    if(socketPair[0]>0 && socketPair[1]>0){
      makeNonBlocking(socketPair[0]);
      makeNonBlocking(socketPair[1]);

      //ADD TO WATCHED FD SET
      pollFds[2].fd= socketPair[0];
      pollFds[2].events = POLLIN;
      globalSocket = socketPair[1];
    }

    /********************/
    /* ETERNAL POLL    */
    /******************/
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
        /***********************************/  
        /*   POLLIN FROM SERVERFD         */
        /*********************************/
        if(pollFds[i].fd == serverFd){
          printStarHeadline("SERVER IS TAKING IN CLIENTS",-1);

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
        else if(pollFds[i].fd == 0){
          printStarHeadline("STDIN INPUT",-1);
          int bytes=0;
          char stdinBuffer[1024];
          memset(&stdinBuffer,0,1024);

          /*******************************/
          /* EXECUTE STDIN COMMANDS     */
          /*****************************/
          while( (bytes=read(0,&stdinBuffer,1024))>0){  
            if (strcmp(stdinBuffer, "/shutdown\n")==0){
              // ATTEMPT SQL SHUTDOWN
              char sqlInsert[1024];
              Account * accountPtr;
              sql = "DELETE FROM ACCOUNTS;";

              //@QUESTION: ISN'T DELETE FROM ACCOUNTS EXECUTED WITHOUT THE USERNAME ATTACHED IN NEXT BLOCK
              //ATTEMPT TO CLEAR USERS
              if ((dbResult = sqlite3_exec(database, sql, callback, 0, &dbErrorMessage)) != SQLITE_OK){
                  fprintf(stderr, "SQL Error: %s\n", dbErrorMessage);
                  //SHUTDOWN PROCEDURES
                  sqlite3_free(dbErrorMessage);
                  processShutdown();
                  exit(0);
              }
              for (accountPtr = accountHead; accountPtr!=NULL; accountPtr = accountPtr->next){
                memset(&sqlInsert, 0, 1024);
                sql = createSQLInsert(accountPtr, sqlInsert);
                printf("sql = %s\n", sql);
                if ((dbResult = sqlite3_exec(database, sql, callback, 0, &dbErrorMessage)) != SQLITE_OK){
                  fprintf(stderr, "SQL Error: %s\n", dbErrorMessage);
                  //SHUTDOWN PROCEDURES
                  sqlite3_free(dbErrorMessage);
                  processShutdown();
                  exit(0);
                }
              }
              sqlite3_close(db);
              processShutdown();
            }
            //EXECUTE OTHER COMMANDS
            recognizeAndExecuteStdin(stdinBuffer);
            memset(&stdinBuffer,0,strnlen(stdinBuffer,1024));
          }  
        }

        /********************************************/
        /* LOOP FORWARD DUE TO GLOBAL SOCKET WRITE */
        /******************************************/
        else if(pollFds[i].fd == pollFds[2].fd ){
          printf("global socket triggered poll\n");
          char globalByte;
          read(pollFds[2].fd,&globalByte,1);
        }
        /**************************************/
        /*   POLLIN: PREVIOUS CLIENT         */
        /************************************/
        else{
          printStarHeadline("COMMUNICATION FROM CLIENT",pollFds[i].fd);
          int bytes,doneReading=0;
          char clientMessage[1024];

          /***********************/  
          /* READ FROM CLIENT   */
          /*********************/
          while(1){ 

            /*** STORE CLIENT BYTES INTO BUFFER ****/
            memset(&clientMessage,0, 1024);
            bytes = read(pollFds[i].fd,clientMessage,strnlen(clientMessage,1023));
            if(bytes<0){
              if(errno!=EAGAIN){
                doneReading=1;
                fprintf(stderr,"Error reading from client %d\n",pollFds[i].fd);
              }
              break;
            }
            //DETECTED THAT CLIENT FD IS CLOSED
            else if(bytes==0){
              doneReading=1;
              break; 
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
              if (sessionLength(getClientByFd(pollFds[i].fd), sessionlength)) 
                protocolMethod(pollFds[i].fd, EMIT, sessionlength, NULL,NULL, verbose);     
            }  
            else if (checkVerb(PROTOCOL_LISTU, clientMessage)){
              if (verbose){
                printf(VERBOSE "%s" DEFAULT, clientMessage);
              }
              char usersBuffer[1024];
              memset(&usersBuffer, 0, 1024); 
              if (buildUtsilArg(usersBuffer))   
                protocolMethod(pollFds[i].fd, UTSIL, usersBuffer,NULL,NULL, verbose);  
            } 
            else if (checkVerb(PROTOCOL_BYE, clientMessage)){ 
              if (verbose){
                printf(VERBOSE "%s" DEFAULT, clientMessage);
              }              doneReading=1; 
              break; 
            }
            else if(extractArgAndTestMSG(clientMessage,NULL,NULL,NULL) ){
              printStarHeadline("GOT MSG!!!",-1);
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
              printf("SERVER RECEIVED MSG: to %s from %s  message: %s",msgToBuffer,msgFromBuffer,msgBuffer);
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
            printf("%s\n",clientMessage);
         }  

         /********************************/
         /* IF CLIENT LOGGED OFF        */
         /******************************/
         if(doneReading){ 
           char username[1024];
           memset(&username, 0, 1024);
           Client * clientPtr;
           strcpy(username, getClientByFd(pollFds[i].fd)->userName);
           disconnectUser(username); 
           for (clientPtr = clientHead; clientPtr; clientPtr = clientPtr->next){
            protocolMethod(getClientByUsername(clientPtr->userName)->session->commSocket, UOFF, username, NULL, NULL, verbose);
           }
           pollFds[i].fd=-1;
           compactDescriptors=1;
         } 
        }
      }// MOVE ON TO NEXT POLL FD EVENT
      
      /* COMPACT POLLS ARRAY */
      if (compactDescriptors){
        compactDescriptors=0;
        compactPollDescriptors();
      } 
    }/* FOREVER RUNNING LOOP */ 

   
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
  free( ((int *)args)  ); // FREE MALLOCED THE HEAP INT 

  //USER INFO
  int user = 0;      
  int *newUser = &user; 
  char username[1024]; 
  memset(&username, 0, 1024); 
  char password[1024];
  memset(&password, 0, 1024);

  /*************** NONBLOCK CONNFD SET TO GLOBAL CLIENT LIST *********/
  printf("Encountered new client in loginThread! Client CONNFD IS %d\n", connfd);
  if (performLoginProcedure(connfd, username, password, newUser)){
    pollFds[pollNum].fd = connfd;
    pollFds[pollNum].events = POLLIN;
    pollNum++; 
    if (makeNonBlocking(connfd)<0){
      fprintf(stderr, "Error making connection socket nonblocking.\n");
    } 
    /**** IF CLIENT FOLLOWED PROTOCOL, CREATE AND PROCESS CLIENT ****/

    /***********************************/
    /* NEW USER NEEDS ACCOUNT CREATED */
    /*********************************/
    if (user){
      printf("Login thread creating new account for logged in user: %s\n",username);
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
      processValidAccount(username, (char *)passwordHash, (char *)saltBuffer);
    }
    processValidClient(username,connfd);
    if(globalSocket>0){
      write(globalSocket," ",1);
      printf("\nwrote to globalSocket\n");
    }
  }
  /**********************/
  /* USER FAILED LOGIN */
  /********************/
  else {
    writeToGlobalSocket();
    printf("Client %d failed to login",connfd);
    close(connfd);
  }
  //EXIT THREAD 
  return NULL; 
}

