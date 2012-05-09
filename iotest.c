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
#include <fcntl.h>
#include <sys/mman.h>
#include <errno.h>
#include <stdbool.h>
#include <string.h>
#include <unistd.h>
#include <math.h>
#include <time.h>

#define PAGE_SIZE 512

int main(int argc, char **argv) {
    if (argc != 3) {
        printf("Usage: %s <dev filename> <max block>\n",  argv[0]);
        exit(EXIT_FAILURE);
    }
    int max_block = atoi(argv[2]);
    // allocate a page-aligned buffer
    void * buf;
    if (posix_memalign(&buf, PAGE_SIZE, 2 * PAGE_SIZE)) {
        perror("Error occurred");
        exit(EXIT_FAILURE);
    }
    int f = open(argv[1], O_RDONLY | O_DIRECT, 0);
    if (f < 0) {
        perror("");
        exit(f);
    }
    // read bytes
    int block = 0;
    int ret = 1;
    while (ret) {
        struct timespec starttime;
        struct timespec endtime;
        clock_getres(CLOCK_REALTIME, &starttime);
        if (clock_gettime(CLOCK_REALTIME, &starttime)) {
            perror("");
            exit(EXIT_FAILURE);
        }
        for (int i = 0; i < 10; i++) {
            ret = read(f, buf, 2 * PAGE_SIZE);
            if (ret < 0) {
                perror("");
                exit(ret);
            }
		if (lseek(f, -ret, SEEK_CUR) < 0) {
		    perror("");
		    exit(EXIT_FAILURE);
		}
        }
        if (clock_gettime(CLOCK_REALTIME, &endtime)) {
            perror("");
            exit(EXIT_FAILURE);
        }
        // compute elapsed time
        long elapsed = (endtime.tv_sec * pow(10, 9) + endtime.tv_nsec) -
            (starttime.tv_sec * pow(10, 9) + starttime.tv_nsec);
        printf("%d,%d\n", block, elapsed);
        block++;
	if (max_block != 0 && block >= max_block) break;
	if (lseek(f, PAGE_SIZE, SEEK_CUR) < 0) {
	    perror("lseek");
	    exit(EXIT_FAILURE);
	}
    }
    close(f);
    return 0;
}
