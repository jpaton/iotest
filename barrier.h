/* 
 * File:   barrier.h
 * Author: jpaton
 * 
 * A file to implement barriers on MacOS X.
 * 
 * The code is all due to:
 *      sugree (http://www.howforge.com/implementing-barrier-in-pthreads)
 *
 * Created on May 17, 2012, 5:17 PM
 */

#ifndef BARRIER_H
#define	BARRIER_H

#ifdef	__cplusplus
extern "C" {
#endif

#if defined(__APPLE__)
#define pthread_barrier_t barrier_t
#define pthread_barrier_attr_t barrier_attr_t
#define pthread_barrier_init(b,a,n) barrier_init(b,n)
#define pthread_barrier_destroy(b) barrier_destroy(b)
#define pthread_barrier_wait(b) barrier_wait(b)
  
typedef struct {
    int needed;
    int called;
    pthread_mutex_t mutex;
    pthread_cond_t cond;
} barrier_t;  

int barrier_init(barrier_t *barrier, int needed);
int barrier_destroy(barrier_t *barrier);
int barrier_wait(barrier_t *barrier);
#endif


#ifdef	__cplusplus
}
#endif

#endif	/* BARRIER_H */

