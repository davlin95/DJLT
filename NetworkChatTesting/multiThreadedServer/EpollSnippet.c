 /******************************************/
    /*        IMPLEMENT EPOLLS               */
    /****************************************/

  /* Initialize Epolls Interface*/
  struct epoll_event epollEvent, * allEpollEvents;
  allEpollEvents = calloc(1024,sizeof(epollEvent));
  int epollFd,eStatus,numEpollFds;
  epollFd = epoll_create(1024);

  /* Set epollEvent for serverFd */
  epollEvent.data.fd = serverFd;
  epollEvent.events = EPOLLIN|EPOLLET;
  eStatus = epoll_ctl(epollFd,EPOLL_CTL_ADD,serverFd,&epollEvent);
  if(eStatus<0){
    fprintf(stderr,"epoll_ctl() serverFd :%s",strerror(errno));
  }

  /* Set epollEvent for server stdin */
  fcntl(0,F_SETFL,O_NONBLOCK); 
  epollEvent.data.fd = 0;
  epollEvent.events = EPOLLIN|EPOLLET;
  eStatus = epoll_ctl(epollFd,EPOLL_CTL_ADD,0,&epollEvent);
  if(eStatus<0){
    fprintf(stderr,"epoll_ctl() stdin :%s",strerror(errno));
  }

  /* Waiting for File Descriptors */
  uint32_t epollErrors = EPOLLERR;
  while(1){
     printf("epolls is waiting\n");
     numEpollFds= epoll_wait(epollFd,allEpollEvents,1024,-1);
     if(numEpollFds<0){
       fprintf(stderr,"epoll_wait():%s",strerror(errno));
     }

     /********************* AN EVENT WAS DETECTED:  **********/
     int i;
     for(i=0;i<numEpollFds;i++){
       printf("numEpollFds = %d\n",numEpollFds);

       /************** ERROR EVENT ***************/
       if(allEpollEvents[i].events &epollErrors){
         fprintf(stderr,"epollErrors after waiting:%s\n",strerror(errno));

       }
       /***************  Server has incoming new client ************/
       else if(allEpollEvents[i].data.fd==serverFd){
          printf("Server is taking in a client\n");
          struct sockaddr_storage newClientStorage;
          socklen_t addr_size = sizeof(newClientStorage);
          connfd = accept(serverFd, (struct sockaddr *) &newClientStorage, &addr_size);
          if(connfd<0){
             fprintf(stderr,"accept(): %s\n",strerror(errno));
             close(connfd); 
             continue;
          }             
          printf("Accepted new client! %d\n", connfd);
          /* Add the connfd into the listening set */
          fcntl(connfd,F_SETFL,O_NONBLOCK); 
          epollEvent.data.fd = connfd;
          epollEvent.events = EPOLLIN|EPOLLET;
          eStatus = epoll_ctl(epollFd,EPOLL_CTL_ADD,connfd,&epollEvent);
          if(eStatus<0){
             fprintf(stderr,"epoll_ct() on new client: %s\n",strerror(errno));
          }

          // printf("Accepted!:%s\n",serverStorage.sin_addr.s_addr);
          printf("GREETING MESSAGE 1 to CLIENT: %d\n",epollEvent.data.fd);
          strcpy(messageOfTheDay,"Hello World ...");
          send(epollEvent.data.fd,messageOfTheDay,(strlen(messageOfTheDay)+1),0);
          
       }
       /******************* SERVER'S STDIN IS ACTIVE ***************/
       else if(allEpollEvents[i].data.fd==0){
         printf("STDIN has something to say\n");
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
       /*********************** CONNECTED CLIENT SENT MESSAGE *********/
       else{
         printf("One client has something to say %d\n",allEpollEvents[i].data.fd);
         int bytes,doneReading,writeStatus=-1;
         char clientMessage[1024];

         /***********************/
         /* READ FROM CLIENT   */
         /*********************/
         while(1){ 
            memset(&clientMessage,0,strlen(clientMessage));
            bytes = read(allEpollEvents[i].data.fd,clientMessage,sizeof(clientMessage));
            if(bytes<0){
              if(errno!=EAGAIN){
                doneReading=1;
                fprintf(stderr,"Error reading from client %d\n",allEpollEvents[i].data.fd);
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
              fprintf(stderr,"Error writing client message %d\n",allEpollEvents[i].data.fd);
            }
            /**************************************/
            /* SEND RESPONSE MESSAGE TO CLIENT   */
            /************************************/
            printf("RESPONSE MESSAGE 1 to CLIENT: %d\n",epollEvent.data.fd);
            strcpy(messageOfTheDay,"Dear Client ... from server\n");
            send(epollEvent.data.fd,messageOfTheDay,(strlen(messageOfTheDay)+1),0);
            if(strcmp(clientMessage,"close\n")==0){
              doneReading=1;
            }
         }
         /*******************************/
         /*   EXIT READING FROM CLIENT */
         /******************************/
         if(doneReading){
           printf("closing client descriptor %d\n",allEpollEvents[i].data.fd);
           close(allEpollEvents[i].data.fd);
         }

       }
     }
  }
