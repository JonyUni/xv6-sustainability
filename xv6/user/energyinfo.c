#include "kernel/types.h"
#include "kernel/energy.h"
#include "user/user.h"

int
main(void)
{
  struct energy_record records[ENERGY_LOG_SIZE];

  if(getenergy(records) < 0){
    fprintf(2, "energyinfo: getenergy failed\n");
    exit(1);
  }

  printf("PID\tNAME\tCPU_TICKS\tENERGY\n");
  for(int i = 0; i < ENERGY_LOG_SIZE; i++){
    if(records[i].pid == 0)
      continue;
    printf("%d %s %d %d\n",
       records[i].pid,
       records[i].name,
       (int)records[i].cpu_ticks,
       (int)records[i].energy_used);
  }

  exit(0);
}
