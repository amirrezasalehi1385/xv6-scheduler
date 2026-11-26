#include "kernel/types.h"
#include "kernel/stat.h"
#include "user/user.h"
#include "kernel/pinfo.h"

char *states[] = {
  [0] "UNUSED  ",
  [1] "USED    ",
  [2] "SLEEPING",
  [3] "RUNNABLE",
  [4] "RUNNING ",
  [5] "ZOMBIE  "
};

int
main(int argc, char *argv[])
{
  static struct pinfo buf[64];
  int n = getprocs(buf, 64);
  if (n < 0) {
    printf("getprocs failed\n");
    exit(1);
  }

  printf("PID\tPPID\tSTATE\t\tQ\tWAIT\tBURST\tCONF\tQENT\tNAME\n");
  for (int i = 0; i < n; i++) {
    printf("%d\t%d\t%s\t%d\t%u\t%d\t%d\t%u\t%s\n",
           buf[i].pid,
           buf[i].ppid,
           states[buf[i].state],
           buf[i].queue,
           buf[i].wait,
           buf[i].burst,
           buf[i].conf,
           buf[i].qenter,
           buf[i].name);
  }
  exit(0);
}