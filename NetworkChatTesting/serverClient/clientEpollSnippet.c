 /*****************************************/
  /*        Initialize Epolls Interface   */
  /***************************************/
  struct epoll_event epollEvent, * allEpollEvents;
  allEpollEvents = calloc(1024,sizeof(epollEvent));

  int epollFd,eStatus,numEpollFds;
  epollFd = epoll_create(1024);

  /* Set epollEvent for serverFd */
  epollEvent.data.fd = clientFd;
  epollEvent.events = EPOLLIN|EPOLLET;
  eStatus = epoll_ctl(epollFd,EPOLL_CTL_ADD,clientFd,&epollEvent);
  if(eStatus<0){
    fprintf(stderr,"epoll_ctl() clientFd :%s",strerror(errno));
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

     int i;
     for(i=0;i<numEpollFds;i++){
       printf("numEpollFds = %d\n",numEpollFds);

       /****************** IF ERROR WAS DETECTED **********************/
       if(allEpollEvents[i].events & epollErrors){
         /* Check for erros */
         fprintf(stderr,"epollErrors after waiting:%s\n",strerror(errno));

       /*************** WHEN SERVER TALKS TO THIS CLIENT ************/
       }else if(allEpollEvents[i].data.fd==clientFd){
         printf("Server is talking to this client\n");
         int serverBytes =0;
         while( (serverBytes = recv(clientFd, message, 1024, 0))>0){
            printf("Data received: %s\n",message);
            memset(&message,0,1024);   
         }
       /************** WHEN CLIENT STDIN IS ACTIVE ********************/
       }else if(allEpollEvents[i].data.fd==0){
         printf("STDIN has something to say\n");

         int bytes=0;
         char stdinBuffer[1024];
         memset(&stdinBuffer,0,1024);
         while( (bytes=read(0,&stdinBuffer,1024))>0){
            printf("reading from client STDIN...\n");

            /****** @TODO Make logout connected to BYE\r\n\r\n **********/
            if(strcmp(stdinBuffer,"/logout\n")==0){
              close(clientFd);
              exit(EXIT_SUCCESS);
              break;
            }
            send(clientFd,stdinBuffer,(strlen(stdinBuffer)),0);
            printf("sent string :%s from client to server\n",stdinBuffer);
            memset(&stdinBuffer,0,strlen(stdinBuffer));
         }
       /**************** UNPREDICTABLE BEHAVIOR *****************/
       }else{
         /* Connected client has something to say */
         printf("This shouldn't be happening descriptor: %d\n",allEpollEvents[i].data.fd);

       }
     }
  }