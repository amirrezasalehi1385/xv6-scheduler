#ifndef PINFO_H
#define PINFO_H

struct pinfo {
  int pid;
  int ppid;
  int state;
  uint64 sz;
  char name[16];
  int queue;
  uint qenter;
  uint wait;
  int burst;
  int conf;
  int qticks;
};

#endif