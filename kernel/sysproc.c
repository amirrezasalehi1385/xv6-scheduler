#include "types.h"
#include "riscv.h"
#include "defs.h"
#include "param.h"
#include "memlayout.h"
#include "spinlock.h"
#include "proc.h"
#include "vm.h"
#include "pinfo.h"
uint64
sys_exit(void)
{
  int n;
  argint(0, &n);
  kexit(n);
  return 0; // not reached
}

uint64
sys_getpid(void)
{
  return myproc()->pid;
}

uint64
sys_fork(void)
{
  return kfork();
}

uint64
sys_wait(void)
{
  uint64 p;
  argaddr(0, &p);
  return kwait(p);
}

uint64
sys_getprocs(void)
{
  uint64 user_buf;
  int max, count;
  struct pinfo *kbuf;
  struct proc *p = myproc();

  argaddr(0, &user_buf);
  argint(1, &max);

  if (max <= 0)
    return -1;
  if (max > NPROC)
    max = NPROC;
  if (max > PGSIZE / sizeof(struct pinfo))
    max = PGSIZE / sizeof(struct pinfo);

  kbuf = (struct pinfo *)kalloc();
  if (kbuf == 0)
    return -1;

  count = proc_getprocs(kbuf, max);

  if (count < 0 ||
      (count > 0 &&
       copyout(p->pagetable, p->sz, user_buf,
               (char *)kbuf, count * sizeof(struct pinfo)) < 0)) {
    kfree(kbuf);
    return -1;
  }

  kfree(kbuf);
  return count;
}

uint64
sys_setmlfq(void)
{
  int pid, queue, burst, conf;
  struct proc *p;

  argint(0, &pid);
  argint(1, &queue);
  argint(2, &burst);
  argint(3, &conf);

  if (queue < 1 || queue > 3)
    return -1;
  if (burst < 1)
    return -1;
  if (conf < 0 || conf > 100)
    return -1;

  if (pid == 0) {
    p = myproc();
    acquire(&p->lock);
  } else {
    for (p = proc; p < &proc[NPROC]; p++) {
      acquire(&p->lock);
      if (p->pid == pid)
        goto found;
      release(&p->lock);
    }
    return -1;
  }

found:
  p->queue  = queue;
  p->burst  = burst;
  p->conf   = conf;
  p->wait   = 0;
  p->qticks = 0;
  p->qenter = ticks;         
  release(&p->lock);
  return 0;
}

uint64
sys_sbrk(void)
{
  uint64 addr;
  int t;
  int n;

  argint(0, &n);
  argint(1, &t);
  addr = myproc()->sz;

  if (t == SBRK_EAGER || n < 0) {
    if (growproc(n) < 0) {
      return -1;
    }
  } else {
    // Lazily allocate memory for this process: increase its memory
    // size but don't allocate memory. If the processes uses the
    // memory, vmfault() will allocate it.
    if (addr + n < addr)
      return -1;
    if (addr + n > TRAPFRAME)
      return -1;
    myproc()->sz += n;
  }
  return addr;
}

uint64
sys_pause(void)
{
  int n;
  uint ticks0;

  argint(0, &n);
  if (n < 0)
    n = 0;
  acquire(&tickslock);
  ticks0 = ticks;
  while (ticks - ticks0 < n) {
    if (killed(myproc())) {
      release(&tickslock);
      return -1;
    }
    sleep_prepare(&ticks);
    release(&tickslock);
    sleep();
    acquire(&tickslock);
  }
  release(&tickslock);
  return 0;
}

uint64
sys_kill(void)
{
  int pid;

  argint(0, &pid);
  return kkill(pid);
}

// return how many clock tick interrupts have occurred
// since start.
uint64
sys_uptime(void)
{
  uint xticks;

  acquire(&tickslock);
  xticks = ticks;
  release(&tickslock);
  return xticks;
}
