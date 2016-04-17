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
#include "../../hw5/serverHeader.h"
#include "WolfieProtocolVerbs.h"

/*******************************/
/*      ACCEPT THREAD         */
/*****************************/
void* acceptThread(void* args){
  char messageOfTheDay[1024];
  int serverFd, connfd;
  char *portNumber = "1234";
  if ((serverFd = createBindListen(portNumber, serverFd))<0){
    printf("error createBindListen\n");
  } else
    printf("Listening\n");

    /******************************************/
    /*        IMPLEMENT POLL                 */
    /****************************************/
    /* Initialize Polls Interface*/
    struct pollfd pollFds[1024];
    memset(pollFds, 0 , sizeof(pollFds));
    int pollStatus,pollNum=2;

    /* Set poll for serverFd */
    pollFds[0].fd = serverFd;
    pollFds[0].events = POLLIN;

    /* Set poll for stdin */
    pollFds[1].fd = 0;
    pollFds[1].events = POLLIN;
    fcntl(serverFd,F_SETFL,O_NONBLOCK); 

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

            /************* STORING ALL INCOMING CONNECTS ****/
            struct sockaddr_storage newClientStorage;
            socklen_t addr_size = sizeof(newClientStorage);

            connfd = accept(serverFd,(struct sockaddr *) &newClientStorage, &addr_size);
            if(connfd<0){
              if(errno!=EWOULDBLOCK){
                fprintf(stderr,"accept(): %s\n",strerror(errno));
                close(connfd); 
              }
              break;
            }
            printf("Accepted new client! %d\n", connfd);
            fcntl(connfd,F_SETFL,O_NONBLOCK); 
            pollFds[pollNum].fd = connfd;
            pollFds[pollNum].events = POLLIN;
            pollNum++;
            /***** SEND MESSAGE TO CLIENT ****/
            printf("connfd: %d   pollFds[pollNum]: %d\n",connfd,pollFds[pollNum-1].fd);
            printf("GREETING MESSAGE 1 to CLIENT: %d\n",pollFds[pollNum-1].fd);
            strcpy(messageOfTheDay,"Hello World ...");
            send(pollFds[pollNum].fd,messageOfTheDay,(strlen(messageOfTheDay)+1),0);
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

            printf("outputting from server's STDIN %s",stdinBuffer);
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
          int bytes,doneReading,writeStatus=-1;
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
              break;
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
            printf("RESPONSE MESSAGE 1 to CLIENT: %d\n",pollFds[i].fd);
            strcpy(messageOfTheDay,"Dear Client ... from server\n");
            send(pollFds[i].fd,messageOfTheDay,(strlen(messageOfTheDay)+1),0);

            if(strcmp(clientMessage,"close\n")==0){
              doneReading=1;
            }
         }
         /*******************************/
         /*   EXIT READING FROM CLIENT */
         /******************************/
         if(doneReading){
           printf("closing client descriptor %d\n",pollFds[i].fd);
           close(pollFds[i].fd);
         }
        }


        /* MOVE ON TO NEXT POLL FD */
      }

    /* FOREVER RUNNING LOOP */ 
    }

   
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
  return NULL;
}


int main(int argc, char* argv[]){
  int threadStatus;
  pthread_t tid[1026];

  threadStatus = pthread_create(&tid[0], NULL, &acceptThread, NULL);

  /**************
  printf("main thread");
  char str[1024];
  int readStatus=0;
  int status = read(0,&str,1024);
  write(1,str,strlen(str));
  ***************/
  pthread_join(tid[0],NULL);
  return 0;

}

void protocolMethod(int fd, int wolfieVerb, char* optionalString){
  char userName[1032];
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
                memset(&userName,0,1032);
                if(optionalString!=NULL){
                  strcpy(userName,PROTOCOL_IAM);
                  strcpy(userName," ");
                  strcpy(userName,optionalString);
                  strcpy(userName," ");
                  strcpy(userName,"\r\n\r\n");
                }
                send(fd,userName,strlen(userName),0); // MACRO NULL TERMINATED BY DEFAULT
                break;

  }

}


char* protocol_IAM_Helper(char* string,int stringLength){

    /****************** ABANDONED CODE *********************/
    /*char* anchor;
    char user[1024];
    memset(&user,0,1024);

    anchor = strstr(string,PROTOCOL_IAM);
    if(anchor==NULL){ //PROTOCOL DOESN'T EXIST IN STRING
        return NULL;
    } 
    else if(anchor!=string){ //PROTOCOL IS NOT STARTING AS THE VERY FIRST BYTE
        return NULL;
    } 
    anchor+=strlen(PROTOCOL_IAM)); // MOVE TO BYTE AFTER PROTOCOL
    if(*anchor!=" ")return NULL; // MISSING SPACE
    anchor++; // MOVE TO NEXT BYTE
    while(*anchor==" ") anchor++; //FIRST BYTE OF USERNAME */
    /********************************************************/

}

/*
bool buildProtocolString(char* buffer,char* protocol, char* middle){
  if(buffer==NULL) return 0;
  strcpy(buffer,protocol);
  strcpy(buffer,middle);
  strcpy(buffer,"\r\n\r\n");
  return 1;
}


bool performProtocolProcedure(int fd,char* userBuffer){
  char protocolBuffer[1024];
  memset(&protocolBuffer,0,1024);

  int bytes=-1;
  bytes = read(fd,&protocolBuffer,1024);
  if(strcmp(protocolBuffer,PROTOCOL_WOLFIE)!=0){
    return 0;
  }else{
    protocolMethod(fd,PROTOCOL_EIFLOW,NULL);
  }
  memset(&protocolBuffer,0,1024);
  bytes =-1;
  bytes = read(fd,&protocolMethod,1024);
  char* result = protocol_IAM_Helper(protocolBuffer);
  if(result==NULL) return 0;
  else{
    char welcomeProtocol[1024];
    memset(&welcomeProtocol,0,1024);
    buildProtocolString(welcomeProtocol,"HI ",result);
    send(fd,welcomeProtocol,strlen(welcomeProtocol),0); 
    return 1;
  }
}

void loginThread(void* args, int fd){


}*/