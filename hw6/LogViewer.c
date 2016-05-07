#define press_to_cont() do {        \
printf("Press Enter to Continue");  \
while(getchar() != '\n');           \
printf("\n");                       \
} while(0)


#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdbool.h>
#include <unistd.h>
#include <fcntl.h>
#include <errno.h> 
#include <sys/poll.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <sys/wait.h>
#include <sys/stat.h>

char *fileName;
void printMenu();
void printInputLog();
void printOptions();
void printSortColumn();
int printAscDsc();
void printSort(char *column, char *choice);
void printFilterField();

void printMenu(){
  printf("\nWelcome to DJLT's Audit Log Viewer\n");
  press_to_cont();
  printInputLog();
}

void printInputLog(){
  char stdinBuffer[1024];
  memset(&stdinBuffer, 0, 1024);
  while (1){
    write(1, "Input an Audit File: ", 21);
    if (read(0, stdinBuffer, 1024)>0){
      stdinBuffer[strlen(stdinBuffer) - 1] = 0;
      struct stat statFile;
      if (stat(stdinBuffer, &statFile) >= 0){
        fileName = stdinBuffer;
        printOptions();
        break;
      }
    }else {
      printf("File does not exist\n");
      press_to_cont();
    }
  }
}
void forkAndGrep(char *string){
  pid_t pid;
  char *argArray[] = {"grep", string, fileName, NULL};
  int status;
  if ((pid=fork())==0){
      printf("%s", string);
      if (execvp("grep", argArray) < 0){
        fprintf(stderr, "error");
      }
  }
  else{
    wait(&status);
  }
}

void printFilter(char *string){
  char stdinBuffer[1024];
  memset(&stdinBuffer, 0, 1024);
  write(1, string, strlen(string));
  read(0, stdinBuffer, 1024);
  stdinBuffer[strlen(stdinBuffer) - 1] = 0;
  forkAndGrep(stdinBuffer);     
}
void printEvent(){
  char stdinBuffer[1024];
  memset(&stdinBuffer, 0, 1024);
  printf("\nEvents:\n"                    \
         "1 - LOGIN\n"       \
         "2 - CMD\n"      \
         "3 - MSG\n"   \
         "4 - LOGOUT\n"                   \
         "5 - Back\n"                     \
  );
  while (1){
    write(0, "Please choose an option: ", 25);
    if (read(0, stdinBuffer, 1024)>0){
      if (strncmp(stdinBuffer, "1", 1)==0){
        forkAndGrep("LOGIN");
        break;
      }
      if (strncmp(stdinBuffer, "2", 1)==0){
        forkAndGrep("CMD");
        break;
      }
      if (strncmp(stdinBuffer, "3", 1)==0){
        forkAndGrep("MSG");
        break;
      }
      if (strncmp(stdinBuffer, "4", 1)==0){
        forkAndGrep("LOGOUT");
        break;
      }
      if (strncmp(stdinBuffer, "5", 1)==0){
        printFilterField();
        break;
      }
    }
  }
}

void printFilterField(){
  char stdinBuffer[1024];
  memset(&stdinBuffer, 0, 1024);
  printf("\nOptions:\n"                    \
         "1 - Sort By Date/Time\n"       \
         "2 - Sort By Username\n"      \
         "3 - Sort By Event\n"   \
         "4 - Go Back\n"                   \
  );
  while (1){
    write(0, "Please choose an option: ", 25);
    if (read(0, stdinBuffer, 1024)>0){
      if (strncmp(stdinBuffer, "1", 1)==0){
        printFilter("Enter Date/time: ");
        break;
      }
      if (strncmp(stdinBuffer, "2", 1)==0){
        printFilter("Enter Username: ");
        break;
      }
      if (strncmp(stdinBuffer, "3", 1)==0){
        printEvent();
        break;
      }
      if (strncmp(stdinBuffer, "4", 1)==0){
        printInputLog();
        break;
      }
    }
  }
}

void printOptions(){
  char stdinBuffer[1024];
  memset(&stdinBuffer, 0, 1024);
  printf("\nOptions:\n"                    \
         "1 - Sort Logs by Column\n"       \
         "2 - Filter Logs by Field\n"      \
         "3 - Search Logs for Keyword\n"   \
         "4 - Go Back\n"                   \
  );
  while (1){
    write(0, "Please choose an option: ", 25);
    if (read(0, stdinBuffer, 1024)>0){
      if (strncmp(stdinBuffer, "1", 1)==0){
        printSortColumn();
        break;
      }
      if (strncmp(stdinBuffer, "2", 1)==0){
        printFilterField();
        break;
      }
      if (strncmp(stdinBuffer, "3", 1)==0){
        printFilter("Keyword: ");
        break;
      }
      if (strncmp(stdinBuffer, "4", 1)==0){
        printInputLog();
        break;
      }
    }
  }
}

void printSortColumn(){
  char stdinBuffer[1024];
  memset(&stdinBuffer, 0, 1024);
  int choice;
  printf("\nChoose a Column:\n"                       \
         "1 - Date Time\n"                  \
         "2 - Username\n"                   \
         "3 - Event\n"                      \
         "4 - Go Back\n"                    \
    );
  while (1){
    write(0, "Please choose an option: ", 25);
    if (read(0, stdinBuffer, 1024)>0){
      if (strncmp(stdinBuffer, "1", 1)==0){
        choice = printAscDsc();
        if (choice == 1){
          printSort("-k1", NULL);
          break;
        }
        else if( choice == 2){
          printSort("-k1", "-r");
          break;
        }
        else{
          printSortColumn();
          break;
        }
      }
      if (strncmp(stdinBuffer, "2", 1)==0){
        choice = printAscDsc();
        if (choice == 1){
          printSort("-k2", NULL);
          break;
        }
        else if( choice == 2){
          printSort("-k2", "-r");
          break;
        }
        else{
          printSortColumn();
          break;
        }
      }
      if (strncmp(stdinBuffer, "3", 1)==0){
        choice = printAscDsc();
        if (choice == 1){
          printSort("-k3", NULL);
          break;
        }
        else if( choice == 2){
          printSort("k3", "-r");
          break;
        }
        else{
          printSortColumn();
          break;
        }
      }
      if (strncmp(stdinBuffer, "4", 1)==0){
        printOptions();
        break;
      }
    }
  }
}

int printAscDsc(){
  char stdinBuffer[1024];
  memset(&stdinBuffer, 0, 1024);
  printf("\nSort By:\n"                       \
         "1 - Ascending\n"                  \
         "2 - Descending\n"                  \
         "3 - Go Back\n"                    \
    );
  while (1){
    write(0, "Please choose an option: ", 25);
    if (read(0, stdinBuffer, 1024)>0){
      if (strncmp(stdinBuffer, "1", 1)==0){
        return 1;
      }
      if (strncmp(stdinBuffer, "2", 1)==0){
        return 2;
      }
      if (strncmp(stdinBuffer, "3", 1)==0){
        return 3;
      }
    }
  }
}
void printSort(char *column, char *choice){
  pid_t pid;
  char *argArray[5];
  int status;
  argArray[0] = "sort";
  argArray[1] = column;
  if (choice == NULL){
    argArray[2] = fileName;
    argArray[3] = NULL;
  }
  else{
    argArray[2] = choice;
    argArray[3] = fileName;
    argArray[4] = NULL;
  }
  if ((pid=fork())==0){
      if (execvp("sort", argArray) < 0){
        fprintf(stderr, "error");
      }
  }
  else{
    wait(&status);
  }
}

int main(int argc, char ** argv) {
  while(1){
    printMenu();
  }
  return(0);
}