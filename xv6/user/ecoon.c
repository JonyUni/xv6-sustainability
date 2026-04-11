#include "kernel/types.h"
#include "user/user.h"

int
main(void)
{
  if(eco_on() < 0){
    fprintf(2, "ecoon: failed\n");
    exit(1);
  }

  printf("eco mode on\n");
  exit(0);
}
