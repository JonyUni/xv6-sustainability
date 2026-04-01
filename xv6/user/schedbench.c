#include "kernel/types.h"
#include "kernel/stat.h"
#include "user/user.h"

struct workload {
  char *name;
  int pid;
};

#define NJOBS 4

static void
run_child(char *name, int gatefd)
{
  char *argv[3];
  char gatefdarg[2];

  argv[0] = name;
  gatefdarg[0] = '0' + gatefd;
  gatefdarg[1] = 0;
  argv[1] = gatefdarg;
  argv[2] = 0;
  exec(name, argv);
  printf("schedbench: exec %s failed\n", name);
  exit(1);
}

int
main(void)
{
  static struct workload jobs[] = {
    { "shortjob", -1 },
    { "mediumjob", -1 },
    { "longjob", -1 },
    { "burstjob", -1 },
  };
  int i;
  int order;
  int ready[2];
  int start_tick;
  int finish_tick;
  int status;
  int pid;
  char *name;

  start_tick = uptime();
  printf("schedbench start tick=%d\n", start_tick);

  if(pipe(ready) < 0){
    printf("schedbench: pipe failed\n");
    exit(1);
  }

  for(i = 0; i < NJOBS; i++){
    pid = fork();
    if(pid < 0){
      printf("schedbench: fork failed for %s\n", jobs[i].name);
      exit(1);
    }
    if(pid == 0){
      close(ready[1]);
      run_child(jobs[i].name, ready[0]);
    }
    jobs[i].pid = pid;
    printf("schedbench spawned job=%s pid=%d tick=%d\n",
           jobs[i].name, pid, uptime());
  }

  close(ready[0]);
  for(i = 0; i < NJOBS; i++){
    if(write(ready[1], "g", 1) != 1){
      printf("schedbench: release write failed\n");
      exit(1);
    }
  }
  close(ready[1]);

  for(order = 1; order <= NJOBS; order++){
    pid = wait(&status);
    finish_tick = uptime();
    name = "unknown";
    for(i = 0; i < NJOBS; i++){
      if(jobs[i].pid == pid){
        name = jobs[i].name;
        break;
      }
    }
    printf("schedbench reap order=%d job=%s pid=%d tick=%d status=%d\n",
           order, name, pid, finish_tick, status);
  }

  printf("schedbench done total_wall=%d\n", uptime() - start_tick);
  exit(0);
}
