#include "types.h"
#include "param.h"
#include "memlayout.h"
#include "riscv.h"
#include "spinlock.h"
#include "defs.h"

#define LCG_A 41
#define LCG_C 3
#define LCG_M 100
#define LCG_SEED 7

static uint lcg_state = LCG_SEED;
static struct spinlock lcg_lock;

void
lcg_init(void)
{
  initlock(&lcg_lock, "lcg");
}

int
lcg_rand100(void)
{
  int r;

  acquire(&lcg_lock);
  lcg_state = (LCG_A * lcg_state + LCG_C) % LCG_M;
  r = lcg_state;
  release(&lcg_lock);
  return r;
}