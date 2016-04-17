#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <stdbool.h>
#include <fcntl.h>
#include <netinet/in.h>
#include <pthread.h>
#include <time.h>
#include <arpa/inet.h>
#include <sys/fcntl.h>





#define USAGE(name) do {                                                                    \
        fprintf(stderr,                                                                         \
            "\n%s UNIX_SOCKET_FD\n"                                                             \
            "UNIX_SOCKET_FD       The Unix Domain File Descriptor number.\n"                  \
            ,(name)                                                                             \
        );                                                                                      \
    } while(0)


						/***********************************************************************/
						/*                    CHAT PROGRAM FUNCTIONS                         */
						/**********************************************************************/


void spawnChatWindow();

void openChatWindow();

void closeChatWindow();

void executeXterm();

void processGeometry(int width, int height);

