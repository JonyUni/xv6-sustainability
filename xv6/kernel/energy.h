#ifndef XV6_ENERGY_H
#define XV6_ENERGY_H

#define ENERGY_LOG_SIZE 16

struct energy_record {
  int pid;
  char name[16];
  uint64 cpu_ticks;
  uint64 energy_used;
};

#endif
