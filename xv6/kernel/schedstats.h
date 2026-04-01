#ifndef XV6_SCHEDSTATS_H
#define XV6_SCHEDSTATS_H

struct schedstats {
  int pid;
  uint run_ticks;
  uint recent_burst_ticks;
  uint times_scheduled;
};

#endif
