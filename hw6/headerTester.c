#include "WolfieChatHeader.h"

int main(int argc, char ** argv, char **envp) {
  USAGEserver("./server");
  USAGEclient("./client");
  USAGEchat("./chat");

  printf("\nserver: /help\n");
  displayHelpMenu(serverHelpMenuStrings);

  printf("\nclient: /help\n");
  displayHelpMenu(clientHelpMenuStrings);

  Client *client1 = createClient(); 
  client1->userName = "Wilson";
  ((struct in_addr *)((Session *)(client1->session))->ipAddress)->s_addr = 324523451;
  startSession(client1->session);
  clientHead = client1;

  sleep(5);

  Client *client2 = createClient(); 
  client2->userName = "David";
  ((struct in_addr *)((Session *)(client1->session))->ipAddress)->s_addr = 324343451;
  startSession(client2->session);
  client1->next = client2;
  client2->prev = client1;

  Account *account1 = createAccount();
  account1->userName = "Wilson";
  account1->password = "wilson";
  accountHead = account1;

  Account *account2 = createAccount();
  account2->userName = "David";
  account2->password = "david";
  account1->next = account2;
  account2->prev = account1;

  printf("\nserver: /accts\n");
  processAcctsRequest();
  printf("\nserver: /users\n");
  processUsersRequest();

  sleep(5);

  printf("\ntest client1:/time\n");
  displayClientConnectedTime(sessionLength(client1->session));

  printf("\ntest client2:/time\n");
  displayClientConnectedTime(sessionLength(client2->session));

  char *password1 = "hel";
  char *password2 = "hello";
  char *password3 = "Hello";
  char *password4 = "Hello1";
  char *password5 = "Hello1!";
  char *password6 = "hel lo";

  printf("\nvalidPassword(%s): %d\n", password1, validPassword(password1));
  printf("validPassword(%s): %d\n", password2, validPassword(password2));
  printf("validPassword(%s): %d\n", password3, validPassword(password3));
  printf("validPassword(%s): %d\n", password4, validPassword(password4));
  printf("validPassword(%s): %d\n", password5, validPassword(password5));
  printf("validPassword(%s): %d\n", password6, validPassword(password6));

  return 0;
}