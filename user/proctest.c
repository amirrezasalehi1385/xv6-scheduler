#include "kernel/types.h"
#include "kernel/pinfo.h"
#include "user/user.h"

#define NCHILD 3
#define MAXP   64   // same as NPROC in xv6

// Print ok/FAIL for one check and abort on the first failure.
static void
check(int cond, char *msg)
{
  if(!cond){
    printf("FAIL: %s\n", msg);
    exit(1);
  }
  printf("ok: %s\n", msg);
}

// Linear search for a pid inside the buffer returned by getprocs().
static struct pinfo *
find(struct pinfo *b, int n, int pid)
{
  for(int i = 0; i < n; i++)
    if(b[i].pid == pid)
      return &b[i];
  return 0;
}

int
main(void)
{
  static struct pinfo buf[MAXP];
  int kids[NCHILD];
  int me = getpid();

  // Pipe used only to keep children blocked (truly SLEEPING).
  // Children block in read() because nobody writes to it.
  // pause() is NOT used for this: it sleeps on &ticks and every
  // child is woken up on each timer tick, so its state would flip
  // between SLEEPING and RUNNABLE.
  int hold[2];
  if(pipe(hold) < 0){
    printf("pipe failed\n");
    exit(1);
  }

  // Create children that block on the pipe.
  for(int i = 0; i < NCHILD; i++){
    int pid = fork();
    if(pid < 0){
      printf("fork failed\n");
      exit(1);
    }
    if(pid == 0){
      close(hold[1]);          // child must not keep the write end open
      char c;
      read(hold[0], &c, 1);    // sleeps on the pipe until EOF
      exit(0);                 // reached after parent closes hold[1]
    }
    kids[i] = pid;             // parent: remember the child's pid
  }
  close(hold[0]);              // parent only needs the write end

  // Give the children time to reach read() and go to sleep.
  pause(10);

  // ---- 1) Normal case: max is large enough ----
  int n = getprocs(buf, MAXP);
  check(n > 0, "getprocs returns a positive count");

  // Every child must be listed with correct ppid, state and size.
  for(int i = 0; i < NCHILD; i++){
    struct pinfo *p = find(buf, n, kids[i]);
    check(p != 0, "child is present in the result");
    check(p->ppid == me, "child's ppid equals parent's pid");
    check(p->state == 2 /* SLEEPING */, "child is SLEEPING");
    check(p->sz > 0, "child's size is greater than 0");
  }

  // The calling process itself must be listed and RUNNING.
  struct pinfo *self = find(buf, n, me);
  check(self != 0, "calling process is present in the result");
  check(self->state == 4 /* RUNNING */, "calling process is RUNNING");
  check(strcmp(self->name, "proctest") == 0, "calling process name is correct");

  // init (pid 1) has no parent, so its ppid must be 0.
  struct pinfo *init = find(buf, n, 1);
  check(init != 0 && init->ppid == 0, "init has ppid 0");

  // ---- 2) max smaller than the number of processes ----
  int m = getprocs(buf, 2);
  check(m == 2, "max=2 returns exactly 2 entries");

  // ---- 3) max larger than the number of processes ----
  int big = getprocs(buf, MAXP);
  check(big == n && big < MAXP, "large max returns the actual count only");

  // ---- 4) Invalid arguments must return -1 ----
  check(getprocs(buf, 0) == -1, "max=0 returns -1");
  check(getprocs(buf, -1) == -1, "max<0 returns -1");
  check(getprocs((struct pinfo *)0xFFFFFFFFFFFFull, 2) == -1,
        "invalid buf address returns -1");

  // ---- 5) Show the table using the ps program ----
  // Run ps in a separate child so we can wait for it.
  int pid = fork();
  if(pid < 0){
    printf("fork failed\n");
    exit(1);
  }
  if(pid == 0){
    close(hold[1]);                 // ps child must not hold the write end
    char *argv[] = {"ps", 0};
    exec("ps", argv);
    printf("exec ps failed\n");     // only reached if exec fails
    exit(1);
  }
  wait(0);  // wait for ps

  // Release the children: EOF on their read() makes them exit.
  close(hold[1]);

  // Reap the sleeping children.
  for(int i = 0; i < NCHILD; i++)
    wait(0);

  printf("ALL TESTS PASSED\n");
  exit(0);
}