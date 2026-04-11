#include "kernel/types.h"
#include "user/user.h"

int
main(void)
{
  if(eco_off() < 0){
    fprintf(2, "ecooff: failed\n");
    exit(1);
  }

  printf("eco mode off\n");
  exit(0);
}
