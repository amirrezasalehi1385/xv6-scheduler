#include "kernel/types.h"
#include "kernel/pinfo.h"
#include "user/user.h"

#define NSLEEP 2                 // children that sleep
#define NBUSY  2                 // children that keep counting
#define NCHILD (NSLEEP + NBUSY)
#define MAXP   64

static int kids[NCHILD];
static int nkids = 0;
static struct pinfo buf[MAXP];

// kill all children and wait for them
static void
cleanup(void)
{
  for(int i = 0; i < nkids; i++)
    kill(kids[i]);
  for(int i = 0; i < nkids; i++)
    wait(0);
}

static void
check(int cond, char *msg)
{
  if(!cond){
    printf("FAIL: %s\n", msg);
    cleanup();
    exit(1);
  }
  printf("ok: %s\n", msg);
}

static struct pinfo *
find(int n, int pid)
{
  for(int i = 0; i < n; i++)
    if(buf[i].pid == pid)
      return &buf[i];
  return 0;
}

static void
create_child(int sleeper)
{
  int pid = fork();
  if(pid < 0){
    printf("fork failed\n");
    cleanup();
    exit(1);
  }
  if(pid == 0){
    if(sleeper){
      pause(100000);             // sleep for a long time
    } else {
      volatile uint64 count = 0;
      for(;;)
        count++;                 // keep using the CPU
    }
    exit(0);
  }
  kids[nkids++] = pid;
  printf("created %s child %d\n", sleeper ? "sleeping" : "busy", pid);
}

// 1 if every sleeping child is currently SLEEPING
static int
all_sleeping(void)
{
  int n = getprocs(buf, MAXP);
  for(int i = 0; i < NSLEEP; i++){
    struct pinfo *p = find(n, kids[i]);
    if(p == 0 || p->state != 2)
      return 0;
  }
  return 1;
}

int
main(void)
{
  int me = getpid();

  // 1) sleeping children
  for(int i = 0; i < NSLEEP; i++)
    create_child(1);

  int start = uptime();
  int ok = 0;
  while(!ok && uptime() - start < 50)
    ok = all_sleeping();
  check(ok, "sleeping children are SLEEPING");

  // 2) busy children
  for(int i = 0; i < NBUSY; i++)
    create_child(0);

  // 3) check PID and PPID
  int n = getprocs(buf, MAXP);
  check(n > 0, "getprocs succeeded");
  for(int i = 0; i < NCHILD; i++){
    struct pinfo *p = find(n, kids[i]);
    check(p != 0, "child PID found");
    check(p->ppid == me, "child PPID is correct");
    if(i >= NSLEEP)
      check(p->state == 3 || p->state == 4, "busy child is RUNNABLE/RUNNING");
  }

  // 4) max smaller and larger than the number of processes
  printf("number of processes: %d\n", n);
  check(getprocs(buf, n - 1) == n - 1, "smaller max returns max entries");
  check(getprocs(buf, MAXP) == n, "larger max returns all processes");

  // 5) show ps output
  printf("\n--- ps ---\n");
  int pid = fork();
  if(pid == 0){
    char *argv[] = {"ps", 0};
    exec("ps", argv);
    exit(1);
  }
  check(pid > 0, "fork for ps");
  wait(0);

  // 6) time to compare with Ctrl+P
  printf("\n--- press Ctrl+P now (10 seconds) ---\n");
  pause(100);

  cleanup();
  printf("ALL TESTS PASSED\n");
  exit(0);
}