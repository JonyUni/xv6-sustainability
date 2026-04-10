#include "kernel/types.h"
#include "kernel/energy.h"
#include "user/user.h"

static void
sort_energy(struct energy_record *records, int n)
{
  struct energy_record tmp;

  for(int i = 0; i < n; i++){
    for(int j = i + 1; j < n; j++){
      if(records[j].energy_used > records[i].energy_used){
        tmp = records[i];
        records[i] = records[j];
        records[j] = tmp;
      }
    }
  }
}

int
main(void)
{
  struct energy_record records[ENERGY_LOG_SIZE];
  uint64 total_energy;
  int count;

  if(getenergy(records) < 0){
    fprintf(2, "energytop: getenergy failed\n");
    exit(1);
  }

  count = 0;
  total_energy = 0;
  for(int i = 0; i < ENERGY_LOG_SIZE; i++){
    if(records[i].pid == 0)
      continue;
    records[count++] = records[i];
    total_energy += records[i].energy_used;
  }

  sort_energy(records, count);

  printf("PID\tENERGY\tNAME\n");
  for(int i = 0; i < count; i++)
    printf("%d\t%lu\t%s\n", records[i].pid, records[i].energy_used, records[i].name);
  printf("Total\t%lu\n", total_energy);

  exit(0);
}
