#include "kernel/types.h"
#include "user/user.h"

int
main(void)
{
  for(int i = 0; i < 3; i++){
    if(fork() == 0){
      volatile unsigned long x = 0;
      for(unsigned long j = 0; j < 400000000UL; j++) x += j;
      printf("child %d (pid %d) done\n", i, getpid());
      exit(0);
    }
  }
  for(int i = 0; i < 3; i++) wait(0);
  exit(0);
}
