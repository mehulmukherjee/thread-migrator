// Sample Application 3
//////////////////////////
//
// DO NOT EDIT THIS FILE !!!
//
//////////////////////////
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include "psu_thread.h"

char REMOTE_HOSTNAME[255];

void print_hostname()
{
  char buffer[100];
  int ret;
  if ((ret = gethostname(buffer, sizeof(buffer))) == -1) {
    perror("gethostname");
    exit(1);
  }
  printf("Hostname: %s\n", buffer);
}

void* bar(void* arg)
{
  int a = (int *)arg;
  print_hostname();
  printf("Bar: Value of A = %d\n",a);
  if(a == 3){
    psu_thread_migrate(REMOTE_HOSTNAME);
  }

  if(a == 5){
    print_hostname();
    printf("Bar: Exit Value of A = %d\n",a);
    return NULL;
  }
  else{
    bar((void *)(a+1));
    print_hostname();
    printf("Bar: Exit Value of A = %d\n",a);
    return NULL;
  }
  
}

int main(int argc, char* argv[])
{
  if(argc < 3) {
    printf("Waiting for goodbye\n");
    return 0;
  }
  psu_thread_setup_init(atoi(argv[2]));

  strcpy(REMOTE_HOSTNAME, argv[1]);
  psu_thread_create(bar, (void *)1);

  return 0;
}
