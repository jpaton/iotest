/*
 * =====================================================================================
 *
 *       Filename:  iotest.c
 *
 *    Description:  Times raw disk reads and prints out timing data for 
 *                  different 512-byte sectors to help me understand block I/O.
 *
 *        Version:  1.1
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
#include <pthread.h>
#include "barrier.h"

#define PAGE_SIZE 512

#ifdef __MACH__ // MAC OS X
#include <mach/clock.h>
#include <mach/mach.h>

#define OPEN_FLAGS O_RDONLY
#define setup(x) fcntl(x, F_NOCACHE, 1)

#else // LINUX

#define OPEN_FLAGS O_RDONLY | O_DIRECT
#define setup(x) 0

#endif

/**
 * Test a condition. If the condition is true, call perror and exit with status
 * EXIT_FAILURE.
 * @param value The condition to be tested.
 * @param msg A parameter to pass to perror.
 */
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
 * Opens a file for raw I/O.
 * @param filename The name of the file to be opened.
 * @return The file descriptor.
 */
int open_file(char * filename) {
  // open file for raw I/O
  int f = open(filename, OPEN_FLAGS, 0);
  die_on_true(setup(f) < 0, "fcntl");
  return f;
}

struct thread_args {
  char * filename;
  pthread_barrier_t * barrier;
  int num_trials;
  int min_sector;
  int max_sector;
  int center;
};

void * read_center(
struct thread_args * args
) {
  // open file and seek to target sector
  int f = open_file(args->filename);
  
  // allocate a page-aligned buffer
  void * buf;
  die_on_true(posix_memalign(&buf, PAGE_SIZE, 2 * PAGE_SIZE), "posix_memalign");
  
  // loop for a number of trials, reading target sector each time
  int num_times = (args->max_sector - args->min_sector) * args->num_trials;
  for (int index = 0; index < num_times; index++) {
    die_on_true(lseek(f, PAGE_SIZE * args->center, SEEK_SET) < 0, "lseek");
    pthread_barrier_wait(args->barrier);
    int ret = read(f, buf, PAGE_SIZE);
    pthread_barrier_wait(args->barrier);
    die_on_true(ret < 0, "read");
  }
  
  // cleanup
  close(f);
  free(buf);
  
  pthread_exit(NULL);
}

void * read_sweep(
struct thread_args * args
) {
  struct timespec starttime;
  struct timespec endtime;
  
  // open file and seek to target sector
  int f = open_file(args->filename);
  
  // allocate a page-aligned buffer
  void * buf;
  die_on_true(posix_memalign(&buf, PAGE_SIZE, 2 * PAGE_SIZE), "posix_memalign");
  
  for (int sector = args->min_sector; sector < args->max_sector; sector++) {
    die_on_true(lseek(f, sector * PAGE_SIZE, SEEK_SET) < 0, "lseek");
    for (int trial = 0; trial < args->num_trials; trial++) {
      die_on_true(lseek(f, PAGE_SIZE * args->min_sector, SEEK_SET) < 0, "lseek");
      pthread_barrier_wait(args->barrier);
      current_time(&starttime);
      int ret = read(f, buf, PAGE_SIZE);
      current_time(&endtime);
      pthread_barrier_wait(args->barrier);
      die_on_true(ret < 0, "read");
      
      unsigned long elapsed = (
        (unsigned long) endtime.tv_sec * pow(10, 9) + 
        (unsigned long) endtime.tv_nsec) - 
        ((unsigned long) starttime.tv_sec * pow(10, 9) + 
        starttime.tv_nsec);
      printf("%d,%lu\n", sector, elapsed);
      
    }
  }
  
  // cleanup
  close(f);
  free(buf);
  
  pthread_exit(NULL);
}

/**
 * Runs a sweep test. Spawns two threads. One thread always submits a request
 * for center_sector. The other thread submits requests for other sectors
 * starting with min_sector and ending with max_sector - 1. Times are printed
 * for each pair of requests. The threads use barriers for synchronization. The
 * idea is to get the OS to schedule the requests close together (or better yet
 * merge them).
 * @param filename Name of the file to use.
 * @param num_trials Number of trials for each sector.
 * @param min_sector Sector to start with (inclusive).
 * @param max_sector Sector to end with (exclusive).
 * @param center_sector The sector that should always be included.
 */
void run_sweep_test(
        char * filename, 
        int num_trials, 
        int min_sector,
        int max_sector,
        int center_sector
) {
  fprintf(stderr, "Running sweep test...\n");
  
  // create barrier
  pthread_barrier_t barrier;
  pthread_barrier_init(&barrier, NULL, 2);
  
  // create threads
  pthread_t center_thread;
  pthread_t sweep_thread;
  struct thread_args args = {
    .filename = filename,
    .num_trials = num_trials,
    .center = center_sector,
    .barrier = &barrier,
    .max_sector = max_sector,
    .min_sector = min_sector
  };
  pthread_create(&center_thread, NULL, (void *)(*read_center), &args);
  pthread_create(&sweep_thread, NULL, (void *)(*read_sweep), &args);
  
  // wait for threads to finish
  pthread_join(center_thread, NULL);
  pthread_join(sweep_thread, NULL);
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
  fprintf(stderr, "Running sequential test...\n");
  
  // allocate a page-aligned buffer
  void * buf;
  die_on_true(posix_memalign(&buf, PAGE_SIZE, 2 * PAGE_SIZE), "posix_memalign");
  
  int f = open_file(filename);
  die_on_true(lseek(f, PAGE_SIZE * min_sector, SEEK_SET) < 0, "lseek");
  
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
  free(buf);
}

void print_usage(char * exec_name) {
    printf("Usage: %s <dev filename> [-s starting sector] [-e ending sector] " 
           "[-n number of trials] [-c center]\n", exec_name);
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
  int center = -1;
  
  char * optstring = "s:e:n:c:";
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
      case 'c':
        center = atoi(optarg);
        break;
      case '?':
      case ':':
      default:
        print_usage(argv[0]);
        exit(EXIT_FAILURE);
    }
  }
  
  if (optind != argc) {
    print_usage(argv[0]);
    exit(EXIT_FAILURE);
  }
  
  if (center != -1)
    run_sweep_test(filename, num_trials, min_sector, max_sector, (max_sector + min_sector) / 2);
  else 
    run_sequential_test(filename, num_trials, min_sector, max_sector);
  return 0;
}
