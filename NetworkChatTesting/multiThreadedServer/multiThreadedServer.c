#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <string.h>
#include <errno.h> 
#include <netdb.h>
#include <sys/types.h>
#include <pthread.h>
#include <sys/epoll.h>
#include <sys/poll.h>
#include <sys/fcntl.h>  
#include <signal.h>
#include "../../hw5/serverHeader.h" 
#include "../../hw5/loginHeader.h"    



int main(int argc, char* argv[]){ 
  int argCounter; 
  bool verbose;
  char *portNumber;
  char * argArray[1024];
  char * flagArray[1024];
  memset(&argArray, 0, sizeof(argArray));
  memset(&flagArray, 0, sizeof(flagArray));
  argCounter = initArgArray(argc, argv, argArray);
  if (argCounter < 3 || argCounter > 4){
    USAGE("./server");
    exit(EXIT_FAILURE); 
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

  // threadStatus = pthread_create(&tid[0], NULL, &acceptThread, NULL);
  // pthread_join(tid[0],NULL);

  int serverFd, connfd;    

  /*******************/
  /* Create Socket  */ 
  /*****************/ 
  if ((serverFd = createBindListen(portNumber, serverFd))<0){
    printf("error createBindListen\n");
    exit(0);       
  } else 
    printf("Listening\n"); 

    /******************************************/
    /*        IMPLEMENT POLL                 */
    /****************************************/
    /* Initialize Polls Interface*/
    memset(pollFds, 0 , sizeof(pollFds));
    pollNum = 2;
    int pollStatus,compactDescriptors=0;

    /* Set poll for serverFd */
    pollFds[0].fd = serverFd;
    pollFds[0].events = POLLIN;

    /* Set poll for stdin */
    fcntl(0,F_SETFL,O_NONBLOCK); 
    pollFds[1].fd = 0;
    pollFds[1].events = POLLIN;

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
          printf("\n/***********************************/\n");
          printf("/*   Server is taking in clients   */\n");
          printf("/***********************************/\n");

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
            if(threadStatus<0){
              printf("Error spawning login thread for descriptor %d\n",connfd);
            }
          }
        }

        /***********************************/
        /*   POLLIN FROM STDIN            */
        /*********************************/
        else if(pollFds[i].fd == 0){
          printf("\n/***********************************/\n");
          printf("/*   STDIN INPUT :                  */\n");
          printf("/***********************************/\n");
          int bytes=0;
          char stdinBuffer[1024];
          memset(&stdinBuffer,0,1024);
          while( (bytes=read(0,&stdinBuffer,1024))>0){
            printf("reading from server's STDIN...\n");
            
            /************* SEND TO CLIENT 
            send(clientFd,stdinBuffer,(strlen(stdinBuffer)),0);
            printf("sent string :%s from client to server\n",stdinBuffer); */

            /*************** EXECUTE STDIN COMMANDS ***********/
            recognizeAndExecuteStdin(stdinBuffer);
            memset(&stdinBuffer,0,strlen(stdinBuffer));
          }
        }


        /**************************************/
        /*   POLLIN: PREVIOUS CLIENT         */
        /************************************/
        else{
          printf("\n/***********************************/\n");
          printf("/*   CLIENT NUMBER %d SAYS:        */\n",pollFds[i].fd);
          printf("/***********************************/\n");
          int bytes,doneReading=0,writeStatus=-1;
          char clientMessage[1024];

          /***********************/
          /* READ FROM CLIENT   */
          /*********************/
          while(1){ 
            memset(&clientMessage,0,strlen(clientMessage));
            bytes = read(pollFds[i].fd,clientMessage,sizeof(clientMessage));
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
            if (checkVerb(PROTOCOL_TIME, clientMessage)){
              char sessionlength[1024];
              memset(&sessionlength, 0, 1024);
              if (sessionLength(getClientByFd(pollFds[i].fd), sessionlength))
                protocolMethod(pollFds[i].fd, EMIT, sessionlength);
            }
            if (checkVerb(PROTOCOL_LISTU, clientMessage)){
              char usersBuffer[1024];
              memset(&usersBuffer, 0, 1024);
              if (buildUtsilArg(usersBuffer))
                protocolMethod(pollFds[i].fd, UTSIL, usersBuffer);
            }
            /*********************************/
            /* OUTPUT MESSAGE FROM CLIENT   */
            /*******************************/
            writeStatus = write(1,clientMessage,bytes);
            if(writeStatus<0){
              fprintf(stderr,"Error writing client message %d\n",pollFds[i].fd);
            }
            /**************************************/
            /* SEND RESPONSE MESSAGE TO CLIENT   */
            /************************************/
         }
         /********************************/
         /* IF CLIENT LOGGED OFF */
         /******************************/
         if(doneReading){
           printf("closing client descriptor %d\n",pollFds[i].fd);
           close(pollFds[i].fd);
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
  int connfd = *(int *)args;
  char username[1024];
  memset(&username, 0, 1024);
  printf("Accepted new client in loginThread! Client CONNFD IS %d\n", connfd);
  /*************** NONBLOCK CONNFD SET TO GLOBAL CLIENT LIST *********/
  if (makeNonBlocking(connfd)<0){
    fprintf(stderr, "Error making connection socket nonblocking.\n");
  } 
  if ((performLoginProcedure(connfd, username))!= NULL){
    pollFds[pollNum].fd = connfd;
    pollFds[pollNum].events = POLLIN;
    pollNum++;

  /**** IF CLIENT FOLLOWED PROTOCOL, CREATE AND PROCESS CLIENT ****/
    processValidClient(username, connfd);
  }
  write(connfd, "\n", 1);
  return NULL; 
}

