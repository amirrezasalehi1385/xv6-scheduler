#include "kernel/types.h"
#include "kernel/param.h"
#include "user/user.h"

char *
state_name(int state)
{
  switch (state) {
  case 1:
    return "USED";
  case 2:
    return "SLEEPING";
  case 3:
    return "RUNNABLE";
  case 4:
    return "RUNNING";
  case 5:
    return "ZOMBIE";
  default:
    return "UNKNOWN";
  }
}

int
main(void)
{
  struct pinfo processes[NPROC];

  int count = getprocs(processes, NPROC);

  if (count < 0) {
    printf("ps: getprocs failed\n");
    exit(1);
  }

  printf("PID\tPPID\tSTATE\tSIZE\tNAME\n");

  for (int i = 0; i < count; i++) {
    printf("%d\t%d\t%s\t%lu\t%s\n",
           processes[i].pid,
           processes[i].ppid,
           state_name(processes[i].state),
           processes[i].sz,
           processes[i].name);
  }

  exit(0);
}
