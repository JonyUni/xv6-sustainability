#include "kernel/types.h"
#include "kernel/stat.h"
#include "user/user.h"

static volatile uint sink;

static void
burn(int outer)
{
  int i, j;

  for(i = 0; i < outer; i++){
    for(j = 0; j < 20000; j++){
      sink += (i + j) & 7;
    }
  }
}

int
main(int argc, char *argv[])
{
  struct schedstats stats;
  char gate;
  int start;
  int end;

  if(argc > 1){
    int gatefd = atoi(argv[1]);
    if(read(gatefd, &gate, 1) != 1){
      printf("mediumjob: start gate failed\n");
      exit(1);
    }
    close(gatefd);
  }

  start = uptime();
  printf("mediumjob start pid=%d tick=%d\n", getpid(), start);
  burn(7200);
  end = uptime();

  if(getschedstats(&stats) < 0){
    printf("mediumjob: getschedstats failed\n");
    exit(1);
  }

  printf("mediumjob done pid=%d start=%d end=%d wall=%d run_ticks=%d picks=%d last_burst=%d\n",
         stats.pid, start, end, end - start, stats.run_ticks,
         stats.times_scheduled, stats.recent_burst_ticks);
  exit(0);
}
