#include "kernel/types.h"
#include "user/user.h"

// CHANGES START
static void
cpu_burst(int ticks)
{
  volatile uint64 value = 0;
  uint start = uptime();
#ifdef SCHEDULER_MLFQ
  uint last_event_tick = start;
#endif

  while (uptime() - start < ticks) {
    value = value * 1664525 + 1013904223;
#ifdef SCHEDULER_MLFQ
    struct schedstats stats;
    uint now = uptime();
    if (now != last_event_tick && schedstats(&stats) == 0) {
      printf("event tick=%d pid=%d queue=%d\n", now, getpid(), stats.queue);
      last_event_tick = now;
    }
#endif
  }
  printf("pid %d cpu burst %d ticks complete\n", getpid(), ticks);
}

static void
io_bound(int rounds)
{
  for (int round = 0; round < rounds; round++) {
    cpu_burst(2);
    pause(4);
  }
  printf("pid %d io workload complete\n", getpid());
}

static void
print_metrics(void)
{
  struct schedstats stats;
  int finish = uptime();

  if (schedstats(&stats) < 0) {
    printf("pid %d metrics unavailable\n", getpid());
    return;
  }
  printf("metrics pid=%d arrival=%d first=%d finish=%d turnaround=%d "
         "waiting=%d response=%d cpu=%d\n",
         getpid(), (int)stats.arrival, (int)stats.first_run, finish,
         (int)(finish - stats.arrival), (int)stats.waiting,
         (int)(stats.first_run - stats.arrival), (int)stats.ticks);
}

int
main(void)
{
  int pid;

  printf("scheduler test start at tick %d\n", uptime());
  for (int workload = 0; workload < 4; workload++) {
    pid = fork();
    if (pid < 0) {
      printf("schedulertest: fork failed\n");
      exit(1);
    }
    if (pid == 0) {
      if (workload == 0)
        cpu_burst(12);
      else if (workload == 1)
        cpu_burst(28);
      else if (workload == 2)
        cpu_burst(56);
      else
        io_bound(8);
      print_metrics();
      exit(0);
    }
  }

  while (wait(0) >= 0)
    ;
  printf("scheduler test complete at tick %d\n", uptime());
  exit(0);
}
// CHANGES END: scheduler workload and metrics test
