#include "types.h"
#include "param.h"
#include "memlayout.h"
#include "riscv.h"
#include "spinlock.h"
#include "defs.h"

#define LCG_A    1664525U
#define LCG_C    1013904223U
#define LCG_SEED 123456789U

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
  uint r;

  acquire(&lcg_lock);
  lcg_state = LCG_A * lcg_state + LCG_C;
  r = lcg_state % 100;
  release(&lcg_lock);

  return r;
}