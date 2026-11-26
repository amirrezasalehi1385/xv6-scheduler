#include "kernel/types.h"
#include "kernel/param.h"
#include "kernel/stat.h"
#include "user/user.h"
#include "kernel/pinfo.h"

#define MAXP 64

static struct pinfo buf[MAXP];

static int
find_proc(int n, int pid)
{
  for (int i = 0; i < n; i++)
    if (buf[i].pid == pid)
      return i;
  return -1;
}

static void
busy(int t)
{
  int start = uptime();
  while (uptime() - start < t)
    ;
}

static int
read_order(int fd, char *expect, int n)
{
  char got[8];
  int ok = (read(fd, got, n) == n);

  printf("  finish order:");
  for (int i = 0; ok && i < n; i++) {
    printf(" %d", got[i]);
    if (got[i] != expect[i])
      ok = 0;
  }
  printf("\n");
  return ok;
}

static void
test_entry(void)
{
  printf("\n== TEST 1: new processes start in Q3, init/sh in Q1 ==\n");
  int pids[4], pass = 1;

  for (int i = 0; i < 4; i++) {
    pids[i] = fork();
    if (pids[i] == 0) {
      busy(10);
      exit(0);
    }
  }

  int n = getprocs(buf, MAXP);
  for (int i = 0; i < 4; i++) {
    int idx = find_proc(n, pids[i]);
    if (idx < 0 || buf[idx].queue != 3) {
      printf("  FAIL: pid %d not in Q3\n", pids[i]);
      pass = 0;
    }
  }
  for (int i = 0; i < n; i++)
    if ((buf[i].pid == 1 || buf[i].pid == 2) && buf[i].queue != 1) {
      printf("  FAIL: pid %d not in Q1\n", buf[i].pid);
      pass = 0;
    }

  while (wait(0) > 0)
    ;
  printf(pass ? ">> TEST 1 PASSED\n" : ">> TEST 1 FAILED\n");
}

static void
test_fcfs(void)
{
  printf("\n== TEST 2: FCFS in Q3 (finish order = arrival order) ==\n");
  int fd[2];
  pipe(fd);

  for (int i = 0; i < 3; i++) {
    if (fork() == 0) {
      char c = i;
      busy(20);
      write(fd[1], &c, 1);
      exit(0);
    }
    busy(2);
  }

  while (wait(0) > 0)
    ;
  char expect[] = {0, 1, 2};
  printf(read_order(fd[0], expect, 3) ? ">> TEST 2 PASSED\n"
                                       : ">> TEST 2 FAILED\n");
}

static void
test_aging(void)
{
  printf("\n== TEST 3: aging (AGING_LIMIT=%d) ==\n", AGING_LIMIT);
  int me = getpid(), seen_q2 = 0, seen_q1 = 0;

  setmlfq(0, 1, 10, 100);

  for (int i = 0; i < 4; i++) {
    if (fork() == 0) {
      busy(80);
      exit(0);
    }
  }

  int start = uptime();
  while (uptime() - start < 350) {
    int n = getprocs(buf, MAXP);
    for (int i = 0; i < n; i++) {
      if (buf[i].ppid != me)
        continue;
      if (buf[i].queue == 2) seen_q2 = 1;
      if (buf[i].queue == 1) seen_q1 = 1;
    }
    pause(10);
  }

  while (wait(0) > 0)
    ;
  printf("  reached Q2: %d, reached Q1: %d\n", seen_q2, seen_q1);
  printf(seen_q2 ? ">> TEST 3 PASSED\n" : ">> TEST 3 FAILED\n");
}

static void
test_rr(void)
{
  printf("\n== TEST 4: RR in Q1 (children finish close together) ==\n");
  int fd[2], pids[3];
  pipe(fd);
  int t0 = uptime();

  for (int i = 0; i < 3; i++) {
    pids[i] = fork();
    if (pids[i] == 0) {
      volatile unsigned long x = 0;
      for (unsigned long j = 0; j < 600000000UL; j++) x += j;
      char c = uptime() - t0;
      write(fd[1], &c, 1);
      exit(0);
    }
  }
  for (int i = 0; i < 3; i++)
    setmlfq(pids[i], 1, 10, 100);

  while (wait(0) > 0)
    ;

  unsigned char t[3];
  read(fd[0], t, 3);
  int mn = 255, mx = 0;
  for (int i = 0; i < 3; i++) {
    if (t[i] < mn) mn = t[i];
    if (t[i] > mx) mx = t[i];
  }
  printf("  finish ticks: %d %d %d (spread %d)\n", t[0], t[1], t[2], mx - mn);
  printf(mx - mn <= 3 * QUANTUM_RR ? ">> TEST 4 PASSED\n" : ">> TEST 4 FAILED\n");
}

static void
test_sjf(void)
{
  printf("\n== TEST 5: SJF in Q2 with conf=100 (shortest burst first) ==\n");
  int fd[2], pids[4];
  int bursts[] = {20, 5, 15, 8};
  pipe(fd);

  for (int i = 0; i < 4; i++) {
    pids[i] = fork();
    if (pids[i] == 0) {
      char c = bursts[i];
      busy(20);
      write(fd[1], &c, 1);
      exit(0);
    }
  }
  for (int i = 0; i < 4; i++)
    setmlfq(pids[i], 2, bursts[i], 100);

  while (wait(0) > 0)
    ;
  char expect[] = {5, 8, 15, 20};
  printf(read_order(fd[0], expect, 4) ? ">> TEST 5 PASSED\n"
                                       : ">> TEST 5 FAILED\n");
}

static void
test_lifecycle(void)
{
  printf("\n== TEST 6: fork/exit/wait/pause/kill ==\n");
  int pass = 1, status;

  int pid = fork();
  if (pid == 0)
    exit(42);
  if (wait(&status) != pid || status != 42) {
    printf("  FAIL: fork/exit/wait\n");
    pass = 0;
  }

  int t0 = uptime();
  pause(10);
  if (uptime() - t0 < 9) {
    printf("  FAIL: pause\n");
    pass = 0;
  }

  pid = fork();
  if (pid == 0)
    for (;;)
      busy(100);
  busy(3);
  if (kill(pid) != 0) {
    printf("  FAIL: kill\n");
    pass = 0;
  }
  wait(0);

  printf(pass ? ">> TEST 6 PASSED\n" : ">> TEST 6 FAILED\n");
}

int
main(int argc, char *argv[])
{
  printf("MLFQ tests: QUANTUM_RR=%d AGING_LIMIT=%d\n", QUANTUM_RR, AGING_LIMIT);

  int t = argc > 1 ? atoi(argv[1]) : 0;
  if (t == 0 || t == 1) test_entry();
  if (t == 0 || t == 2) test_fcfs();
  if (t == 0 || t == 3) test_aging();
  if (t == 0 || t == 4) test_rr();
  if (t == 0 || t == 5) test_sjf();
  if (t == 0 || t == 6) test_lifecycle();

  exit(0);
}