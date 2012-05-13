/*
 * =====================================================================================
 *
 *       Filename:  iotest.c
 *
 *    Description:  Times raw disk reads and prints out timing data for 
 *                  different 512-byte sectors to help me understand block I/O.
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
#include <stdint.h>

#define PAGE_SIZE 512

#ifdef __MACH__ // MAC OS X
#include <mach/clock.h>
#include <mach/mach.h>

#define OPEN_FLAGS O_RDONLY
#define setup(x) fcntl(x, F_NOCACHE, 1)

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

/**
 * Runs the sequential test. For each sector in range, for a number of trials,
 * fetch that sector and the next sector, printing out times for each one.
 */
void run_sequential_test(
        char * filename, 
        int num_trials, 
        int min_sector,
        int max_sector
) {
  // allocate a page-aligned buffer
  void * buf;
  die_on_true(posix_memalign(&buf, PAGE_SIZE, 2 * PAGE_SIZE), "posix_memalign");
  
  // open file for raw I/O
  int f = open(filename, OPEN_FLAGS, 0);
  die_on_true(setup(f) < 0, "fcntl");
  die_on_true(lseek(f, PAGE_SIZE * min_sector, SEEK_SET), "lseek");
  
  // read bytes
  int ret = 1;
  for (int block = min_sector; block < max_sector; block++) {
    struct timespec starttime;
    struct timespec endtime;
    for (int i = 0; i < num_trials; i++) {
      current_time(&starttime);
      ret = read(f, buf, 2 * PAGE_SIZE);
      current_time(&endtime);
      die_on_true(ret < 0, "read");
      die_on_true(lseek(f, -ret, SEEK_CUR) < 0, "lseek");
      // compute elapsed time
      unsigned long elapsed = (
        (unsigned long) endtime.tv_sec * pow(10, 9) + 
        (unsigned long) endtime.tv_nsec) - 
        ((unsigned long) starttime.tv_sec * pow(10, 9) + 
        starttime.tv_nsec);
      printf("%d,%lu\n", block, elapsed);
    }
    die_on_true(lseek(f, PAGE_SIZE, SEEK_CUR) < 0, "lseek");
  }
  close(f);
}

void print_usage(char * exec_name) {
    printf("Usage: %s <dev filename> [-s starting sector] [-e ending sector] [-n number of trials]\n", exec_name);
}

int main(int argc, char **argv) {
  if (argc < 2) {
    print_usage(argv[0]);
    exit(EXIT_FAILURE);
  }
  
  // default parameters
  char * filename = argv[1]; // no default, obviously
  int min_sector = 0;
  int max_sector = INT32_MAX;
  int num_trials = 10;
  
  char * optstring = "s:e:n:";
  optind = 2;
  int ch;
  while ((ch = getopt(argc, argv, optstring)) != -1) {
    switch (ch) {
      case 's':
        min_sector = atoi(optarg);
        break;
      case 'e':
        max_sector = atoi(optarg);
        break;
      case 'n':
        num_trials = atoi(optarg);
        break;
      case '?':
      case ':':
      default:
        print_usage(argv[0]);
        exit(EXIT_FAILURE);
    }
  }
  
  run_sequential_test(filename, num_trials, min_sector, max_sector);
  return 0;
}
