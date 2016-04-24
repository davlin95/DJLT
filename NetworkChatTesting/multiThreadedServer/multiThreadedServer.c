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
  
  //DATABASE VARIABLES
  sqlite3 *db;
  char *zErrMsg = 0;
  int rc;
  char *sql;
  argCounter = initArgArray(argc, argv, argArray);
  if (argCounter < 4 || argCounter > 5){
    USAGE("./server"); 
    exit(EXIT_FAILURE);  
  } else if (argCounter == 5){

    if ((rc = sqlite3_open(argArray[3], &db))){
      fprintf(stderr, "Error opening database: %s\n", sqlite3_errmsg(db));
      exit(0);
    }
    sql = "SELECT * FROM ACCOUNTS";
    if ((rc = sqlite3_exec(db, sql, callback, 0, &zErrMsg)) != SQLITE_OK){
      fprintf(stderr, "SQL Error: %s\n", zErrMsg);
      sqlite3_free(zErrMsg);
      exit(0);
    }
  } else{
    if ((rc = sqlite3_open("accounts.db", &db))){
      fprintf(stderr, "Error opening database: %s\n", sqlite3_errmsg(db));
      exit(0);
    }
    sql = "CREATE TABLE ACCOUNTS("                        \
          "username     CHAR(1024) PRIMARY KEY NOT NULL," \
          "password     CHAR(1024) NOT NULL,"             \
          "salt         CHAR(1024) NOT NULL);";
    if ((rc = sqlite3_exec(db, sql, callback, 0, &zErrMsg)) != SQLITE_OK){
      fprintf(stderr, "SQL Error: %s\n", zErrMsg);
      sqlite3_free(zErrMsg);
      exit(0);
    }
  }
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
  portNumber = argArray[1];   
  strcpy(messageOfTheDay, argArray[2]); 
  int threadStatus,threadNum=0;
  pthread_t threadId[1026];  
  signal(SIGINT,killServerHandler); 

  int serverFd = 0, connfd;    

  /*******************/
  /* Create Socket  */ 
  /*****************/ 
  if ((serverFd = createBindListen(portNumber, serverFd))<0){
    printf("error createBindListen\n");
    exit(EXIT_FAILURE);       
  } else 
    printf("Listening\n"); 

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

    int socketPair[2]={0,0};
    createSocketPair(socketPair,2);
    if(socketPair[0]>0 && socketPair[1]>0){
      makeNonBlocking(socketPair[0]);

      pollFds[2].fd= socketPair[0];
      pollFds[2].events = POLLIN;
      globalSocket = socketPair[1];
      printf("twin socket pairs are %d , %d",socketPair[0],socketPair[1]);
    }

    while(1){
      pollStatus = poll(pollFds, pollNum, -1);
      if(pollStatus<0){
        fprintf(stderr,"poll():%s",strerror(errno)); 
        break;
      }
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
                fprintf(stderr,"accept(): %s\n",strerror(errno));
                close(connfd); 
              }
              break;
            }
            *connfdPtr=connfd; 

            /************** LOGIN THREAD FOR EVERY CONNFD *******/
            threadStatus = pthread_create(&threadId[threadNum++], NULL, &loginThread, connfdPtr);
            pthread_setname_np(threadId[threadNum-1],"LOGIN THREAD");
            if(threadStatus<0){
              printf("Error spawning login thread for descriptor %d\n",connfd);
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
          while( (bytes=read(0,&stdinBuffer,1024))>0){  

            /*******************************/
            /* EXECUTE STDIN COMMANDS     */
            /*****************************/

            /*************** EXECUTE STDIN COMMANDS ***********/
            if (strcmp(stdinBuffer, "/shutdown\n")==0){  
              char sqlInsert[1024];
              Account * accountPtr;
              sql = "DELETE FROM ACCOUNTS;";
              if ((rc = sqlite3_exec(db, sql, callback, 0, &zErrMsg)) != SQLITE_OK){
                  fprintf(stderr, "SQL Error: %s\n", zErrMsg);
                  sqlite3_free(zErrMsg);
                  exit(0);
              }
              for (accountPtr = accountHead; accountPtr; accountPtr = accountPtr->next){
                memset(&sqlInsert, 0, 1024);
                sql = createSQLInsert(accountPtr, sqlInsert);
                printf("sql = %s\n", sql);
                if ((rc = sqlite3_exec(db, sql, callback, 0, &zErrMsg)) != SQLITE_OK){
                  fprintf(stderr, "SQL Error: %s\n", zErrMsg);
                  sqlite3_free(zErrMsg);
                  exit(0);
                }
              }
              sqlite3_close(db);
              processShutdown();
            }
            recognizeAndExecuteStdin(stdinBuffer);
            memset(&stdinBuffer,0,strnlen(stdinBuffer,1024));
          }  
        }else if(pollFds[i].fd == pollFds[2].fd ){
          printf("global socket triggered poll\n");
          char blank[1024];
          read(pollFds[2].fd,&blank,1023);
        }
        /**************************************/
        /*   POLLIN: PREVIOUS CLIENT         */
        /************************************/
        else{
          printStarHeadline("COMMUNICATION FROM CLIENT",pollFds[i].fd);
          int bytes,doneReading=0,writeStatus=-1;
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
            }else if(bytes==0){  
              doneReading=1;
              break; 
            }  
            if (verbose)
              printf(VERBOSE "%s" DEFAULT, clientMessage);
 
            /****************************/ 
            /* CLIENT BUILT IN REQUESTS */ 
            /***************************/
 
            if (checkVerb(PROTOCOL_TIME, clientMessage)){  
              char sessionlength[1024];
              memset(&sessionlength, 0, 1024);
              if (sessionLength(getClientByFd(pollFds[i].fd), sessionlength)) 
                protocolMethod(pollFds[i].fd, EMIT, sessionlength, NULL,NULL, verbose);     
            }  
            else if (checkVerb(PROTOCOL_LISTU, clientMessage)){   
              char usersBuffer[1024];
              memset(&usersBuffer, 0, 1024); 
              if (buildUtsilArg(usersBuffer))   
                protocolMethod(pollFds[i].fd, UTSIL, usersBuffer,NULL,NULL, verbose);  
            } 
            else if (checkVerb(PROTOCOL_BYE, clientMessage)){ 
              doneReading=1; 
              printf("BYE PROTOCOL, Client said: %s", clientMessage);
              break; 
            }else if(extractArgAndTestMSG(clientMessage,NULL,NULL,NULL) ){
              printStarHeadline("GOT MSG!!!",-1);
              char msgToBuffer[1024];
              char msgFromBuffer[1024];
              char msgBuffer[1024];

              memset(&msgToBuffer,0,1024);
              memset(&msgFromBuffer,0,1024); 
              memset(&msgBuffer,0,1024);
 
              extractArgAndTestMSG(clientMessage,msgToBuffer,msgFromBuffer,msgBuffer);
              printf("SERVER RECEIVED MSG: to %s from %s  message: %s",msgToBuffer,msgFromBuffer,msgBuffer);
              Client* toUser= getClientByUsername(msgToBuffer);
              printf("DONE SEARCHING FOR USER\n"); 
              
              //CHECK IF THE TO-USER IS STILL LOGGED ON, OR IF THEY EXIST, USER NOT MESSAGING SELF
              if( toUser!=NULL && strcmp(msgToBuffer, msgFromBuffer)!=0 ){
                //SEND MESSAGE BACK
                char messageResponse[1024];
                memset(&messageResponse,0,1024);
                buildMSGProtocol(messageResponse,msgToBuffer,msgFromBuffer,msgBuffer);
                send(toUser->session->commSocket,messageResponse,strnlen(messageResponse,1023),0);
                send(pollFds[i].fd,messageResponse,strnlen(messageResponse,1023),0);
              
              }else{ 
                //ERROR BACK TO USER
                protocolMethod(pollFds[i].fd,ERR1,NULL,NULL,NULL, verbose);
              }

            }
            /*********************************/
            /* OUTPUT MESSAGE FROM CLIENT   */
            /*******************************/
            writeStatus = write(1,clientMessage,bytes);
            if(writeStatus<0){
              fprintf(stderr,"Error writing client message %d\n",pollFds[i].fd);
            }

         }
         /********************************/
         /* IF CLIENT LOGGED OFF */
         /******************************/
         if(doneReading){ 
           char username[1024];
           memset(&username, 0, 1024);
           Client * clientPtr;
           strcpy(username, getClientByFd(pollFds[i].fd)->userName);
           printf("Closing session with client: %d\n",pollFds[i].fd);
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
  if(serverFd>0){  
    close(serverFd);  
    printf("closed serverFd\n");  
  }  
  if(connfd>0){
    close(connfd); 
    printf("closed connfd\n");  
  }  
  return 0;
}    
 
/**********************/
/*     LOGIN THREAD  */ 
/********************/
 
void* loginThread(void* args){   
  int user = false;      
  int *newUser = &user; 
  int connfd = *(int *)args; 
  char username[1024]; 
  memset(&username, 0, 1024); 
  char password[1024];
  memset(&password, 0, 1024);
  printf("Encountered new client in loginThread! Client CONNFD IS %d\n", connfd);
  /*************** NONBLOCK CONNFD SET TO GLOBAL CLIENT LIST *********/
  if (performLoginProcedure(connfd, username, password, newUser)){
    fprintf(stderr,"performLoginProcedure should be true\n");
    pollFds[pollNum].fd = connfd;
    pollFds[pollNum].events = POLLIN;
    pollNum++; 
    if (makeNonBlocking(connfd)<0){
      fprintf(stderr, "Error making connection socket nonblocking.\n");
    } 
    /**** IF CLIENT FOLLOWED PROTOCOL, CREATE AND PROCESS CLIENT ****/
    printf("user is %d\n", user);
    if (user){
      printf("creating new account\n");
      unsigned char saltBuffer[1024];
      unsigned char passwordHash[1024]; 
      memset(&saltBuffer, 0, 1024);
      memset(&passwordHash, 0, 1024);
      if (RAND_bytes(saltBuffer, 10) == 0){
        fprintf(stderr, "Error creating salt\n");
      }
      strcat(password, (char*)saltBuffer);
      sha256(password, passwordHash);
      processValidAccount(username, (char *)passwordHash, (char *)saltBuffer);
    }
    processValidClient(username,connfd);
    if(globalSocket>0){
      write(globalSocket," ",1);
      printf("\nwrote to globalSocket\n");
    }

  }else {
    writeToGlobalSocket();
    close(connfd);
    printf("Client %d failed to login",connfd);
  }
  return NULL; 
}

