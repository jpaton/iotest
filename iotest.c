/*
 * =====================================================================================
 *
 *       Filename:  iotest.c
 *
 *    Description:
 *
 *        Version:  1.0
 *        Created:  05/07/2012 11:56:41 PM
 *       Revision:  none
 *       Compiler:  gcc
 *
 *         Author:  James Paton (paton@cs.wisc.edu).
 *
 * =====================================================================================
 */
#define _GNU_SOURCE

#include <stdio.h>
#include <stdlib.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <sys/mman.h>
#include <fcntl.h>
#include <errno.h>
#include <stdbool.h>
#include <string.h>
#include <unistd.h>
#include <math.h>
#include <time.h>

#define PAGE_SIZE 512

#ifdef __MACH__ // MAC OS X
#include <mach/clock.h>
#include <mach/mach.h>

#define OPEN_FLAGS O_RDONLY
#define setup(x) fcntl(x, F_NOCACHE, 1);

#else // LINUX

#define OPEN_FLAGS O_RDONLY | O_DIRECT
#define setup(x)

#endif

static inline void die_on_true(bool value, char * msg) {
  if (value) {
    perror(msg);
    exit(EXIT_FAILURE);
  }
}

/* Gets current time. Taken from https://gist.github.com/1087739 */
void current_time(struct timespec *ts) {
#ifdef __MACH__ // OS X does not have clock_gettime, use clock_get_time
  clock_serv_t cclock;
  mach_timespec_t mts;
  host_get_clock_service(mach_host_self(), CALENDAR_CLOCK, &cclock);
  clock_get_time(cclock, &mts);
  mach_port_deallocate(mach_task_self(), cclock);
  ts->tv_sec = mts.tv_sec;
  ts->tv_nsec = mts.tv_nsec;
#else
  clock_gettime(CLOCK_REALTIME, ts);
#endif
}

int main(int argc, char **argv) {
  if (argc != 3) {
    printf("Usage: %s <dev filename> <max block>\n", argv[0]);
    exit(EXIT_FAILURE);
  }
  int max_block = atoi(argv[2]);
  // allocate a page-aligned buffer
  void * buf;
  die_on_true(posix_memalign(&buf, PAGE_SIZE, 2 * PAGE_SIZE), "posix_memalign");
  int f = open(argv[1], OPEN_FLAGS, 0);
  setup(f);
  die_on_true(f < 0, "fcntl");
  // read bytes
  int block = 0;
  int ret = 1;
  while (ret) {
    struct timespec starttime;
    struct timespec endtime;
    for (int i = 0; i < 10; i++) {
      current_time(&starttime);
      ret = read(f, buf, 2 * PAGE_SIZE);
      current_time(&endtime);
      die_on_true(ret < 0, "read");
      die_on_true(lseek(f, -ret, SEEK_CUR) < 0, "lseek");
      // compute elapsed time
      long elapsed = (endtime.tv_sec * pow(10, 9) + endtime.tv_nsec) -
              (starttime.tv_sec * pow(10, 9) + starttime.tv_nsec);
      printf("%d,%d\n", block, elapsed);
    }
    block++;
    if (max_block != 0 && block >= max_block) break;
    die_on_true(lseek(f, PAGE_SIZE, SEEK_CUR) < 0, "lseek");
  }
  close(f);
  return 0;
}
