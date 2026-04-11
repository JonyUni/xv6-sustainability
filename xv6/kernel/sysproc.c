#include "types.h"
#include "riscv.h"
#include "defs.h"
#include "param.h"
#include "memlayout.h"
#include "spinlock.h"
#include "proc.h"
#include "schedstats.h"
#include "vm.h"

extern struct proc proc[];
extern struct energy_record energy_log[];
extern int energy_log_idx;
extern struct spinlock energy_log_lock;
extern int eco_mode;

uint64
sys_exit(void)
{
  int n;
  argint(0, &n);
  kexit(n);
  return 0;  // not reached
}

uint64
sys_getpid(void)
{
  return myproc()->pid;
}

uint64
sys_fork(void)
{
  return kfork();
}

uint64
sys_wait(void)
{
  uint64 p;
  argaddr(0, &p);
  return kwait(p);
}

uint64
sys_sbrk(void)
{
  uint64 addr;
  int t;
  int n;

  argint(0, &n);
  argint(1, &t);
  addr = myproc()->sz;

  if(t == SBRK_EAGER || n < 0) {
    if(growproc(n) < 0) {
      return -1;
    }
  } else {
    // Lazily allocate memory for this process: increase its memory
    // size but don't allocate memory. If the processes uses the
    // memory, vmfault() will allocate it.
    if(addr + n < addr)
      return -1;
    if(addr + n > TRAPFRAME)
      return -1;
    myproc()->sz += n;
  }
  return addr;
}

uint64
sys_pause(void)
{
  int n;
  uint ticks0;

  argint(0, &n);
  if(n < 0)
    n = 0;
  acquire(&tickslock);
  ticks0 = ticks;
  while(ticks - ticks0 < n){
    if(killed(myproc())){
      release(&tickslock);
      return -1;
    }
    sleep(&ticks, &tickslock);
  }
  release(&tickslock);
  return 0;
}

uint64
sys_kill(void)
{
  int pid;

  argint(0, &pid);
  return kkill(pid);
}

// return how many clock tick interrupts have occurred
// since start.
uint64
sys_uptime(void)
{
  uint xticks;

  acquire(&tickslock);
  xticks = ticks;
  release(&tickslock);
  return xticks;
}

uint64
sys_getschedstats(void)
{
  uint64 addr;
  struct proc *p;
  struct schedstats stats;

  argaddr(0, &addr);
  p = myproc();

  acquire(&p->lock);
  stats.pid = p->pid;
  stats.run_ticks = p->run_ticks;
  stats.recent_burst_ticks = p->recent_burst_ticks;
  stats.times_scheduled = p->times_scheduled;
  release(&p->lock);

  if(copyout(p->pagetable, addr, (char *)&stats, sizeof(stats)) < 0)
    return -1;

  return 0;
}
uint64
sys_getenergy(void)
{
  uint64 addr;
  struct proc *self;
  struct proc *p;
  struct energy_record records[ENERGY_LOG_SIZE];
  int count;

  argaddr(0, &addr);
  self = myproc();

  count = 0;
  for(p = proc; p < &proc[NPROC] && count < ENERGY_LOG_SIZE; p++){
    acquire(&p->lock);
    if(p->state != UNUSED){
      records[count].pid = p->pid;
      safestrcpy(records[count].name, p->name, sizeof(records[count].name));
      records[count].cpu_ticks = p->cpu_ticks;
      records[count].energy_used = p->energy_used;
      count++;
    }
    release(&p->lock);
  }

  for(int i = count; i < ENERGY_LOG_SIZE; i++){
    records[i].pid = 0;
    records[i].name[0] = '\0';
    records[i].cpu_ticks = 0;
    records[i].energy_used = 0;
  }

  if(copyout(self->pagetable, addr, (char *)records, sizeof(records)) < 0)
    return -1;

  return count;
}

uint64
sys_eco_on(void)
{
  eco_mode = 1;
  return 0;
}

uint64
sys_eco_off(void)
{
  eco_mode = 0;
  return 0;
}
