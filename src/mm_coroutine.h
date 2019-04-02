/**
 *===========================================================================
 *  DarkBlue Engine Source File.
 *  Copyright (C), DarkBlue Studios.
 * -------------------------------------------------------------------------
 *    File name: mm_coroutine.h
 *      Version: v0.0.0
 *   Created on: Mar 31, 2019 by konyka
 *       Editor: Sublime Text3
 *  Description: 
 * -------------------------------------------------------------------------
 *      History:
 *
 *===========================================================================
 */
 

#ifndef __DARKBLUE_MM_COROUTINE_H__
#define __DARKBLUE_MM_COROUTINE_H__

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdint.h>
#include <limits.h>
#include <assert.h>
#include <inttypes.h>
#include <unistd.h>
#include <pthread.h>
#include <fcntl.h>
#include <sys/time.h>
#include <sys/mman.h>
#include <netinet/tcp.h>

#include <sys/poll.h>
#include <sys/epoll.h>

#include <errno.h>

#include "mm_queue.h"
#include "mm_tree.h"


#define MM_CO_MAX_EVENT_SIZE    ( 1024 * 1024 )
#define MM_CO_MAX_STACK_SIZE    ( 16 * 1024 )

#define BIT(x)                          (1 << (x))
#define CLEARBIT(x)              ~(1 << (x))

#define CANCEL_FD_WAIT_UINT64   1

typedef void (*p_coroutine)(void *);

typedef enum {

    MM_COROUTINE_STATUS_NEW, 

    MM_COROUTINE_STATUS_READY,
    MM_COROUTINE_STATUS_SLEEPING,

    MM_COROUTINE_STATUS_WAIT_READ,
    MM_COROUTINE_STATUS_WAIT_WRITE,

    MM_COROUTINE_STATUS_WAIT_IO_READ,
    MM_COROUTINE_STATUS_WAIT_IO_WRITE,

    MM_COROUTINE_STATUS_BUSY,
    
    MM_COROUTINE_STATUS_EXPIRED,
    
    MM_COROUTINE_STATUS_EXITED,

    MM_COROUTINE_STATUS_PENDING_RUNCOMPUTE,
    MM_COROUTINE_STATUS_RUNCOMPUTE,
      
    
    MM_COROUTINE_STATUS_FDEOF,
    MM_COROUTINE_STATUS_DETACH,
    MM_COROUTINE_STATUS_CANCELLED,

    MM_COROUTINE_STATUS_WAIT_MULTI,

} mm_coroutine_status;

LIST_HEAD(_mm_coroutine_list, _mm_coroutine);
TAILQ_HEAD(_mm_coroutine_queue, _mm_coroutine);

RB_HEAD(_mm_coroutine_rbtree_sleep, _mm_coroutine);
RB_HEAD(_mm_coroutine_rbtree_wait, _mm_coroutine);



typedef struct _mm_coroutine_list mm_coroutine_list;
typedef struct _mm_coroutine_queue mm_coroutine_queue;

typedef struct _mm_coroutine_rbtree_sleep mm_coroutine_rbtree_sleep;
typedef struct _mm_coroutine_rbtree_wait mm_coroutine_rbtree_wait;

typedef struct _mm_cpu_ctx {

    void *rsp; //
    void *rbp;
    void *rip;
    void *rdi;
    void *rsi;
    void *rbx;
    void *r1;
    void *r2;
    void *r3;
    void *r4;
    void *r5;

} mm_cpu_ctx;


typedef struct _mm_schedule {
    uint64_t birth;
    mm_cpu_ctx ctx;
    void *stack;
    size_t stack_size;
    int spawned_coroutines;
    uint64_t default_timeout;
    struct _mm_coroutine *curr_thread;
    int page_size;

    int poller_fd;
    int eventfd;
    struct epoll_event eventlist[MM_CO_MAX_EVENT_SIZE];
    int nevents;

    int num_new_events;

    mm_coroutine_queue ready_queue;

    mm_coroutine_list busy_list;
    
    mm_coroutine_rbtree_sleep sleeping_tree;
    mm_coroutine_rbtree_wait waiting_tree;

    //private 

} mm_schedule;


typedef struct _mm_coroutine {

    //private

     uint64_t id;   
     uint64_t birth;

    mm_cpu_ctx ctx;
    p_coroutine func;
    void *arg;
    void *data;
    void *stack;
    size_t stack_size;
    size_t last_stack_size;
    
    mm_coroutine_status status;
    mm_schedule *sched;

#if CANCEL_FD_WAIT_UINT64
    int fd;
    unsigned short events;  //POLL_EVENT
#else
    int64_t fd_wait;
#endif

    char func_name[64];
    struct _mm_coroutine *co_join;

    uint32_t ops;
    uint64_t sleep_usecs;

    RB_ENTRY(_mm_coroutine) sleep_node;
    RB_ENTRY(_mm_coroutine) wait_node;

    LIST_ENTRY(_mm_coroutine) busy_next;
    TAILQ_ENTRY(_mm_coroutine) ready_next;

    int ready_fds;
    struct pollfd *pfds;
    nfds_t nfds;
    
} mm_coroutine;

extern pthread_key_t global_sched_key;
static inline mm_schedule *mm_coroutine_get_sched(void) {
    return pthread_getspecific(global_sched_key);
}

static inline uint64_t mm_coroutine_diff_usecs(uint64_t t1, uint64_t t2) {
    return t2-t1;
}

static inline uint64_t mm_coroutine_usec_now(void) {
    struct timeval t1 = {0, 0};
    gettimeofday(&t1, NULL);

    return t1.tv_sec * 1000000 + t1.tv_usec;
}

int mm_epoller_ev_register_trigger(void);
int mm_epoller_wait(struct timespec t);
int mm_coroutine_resume(mm_coroutine *co);
void mm_coroutine_free(mm_coroutine *co);
int mm_coroutine_create(mm_coroutine **new_co, p_coroutine func, void *arg);
void mm_coroutine_yield(mm_coroutine *co);

void mm_coroutine_sleep(uint64_t msecs);

int mm_socket(int domain, int type, int protocol);
int mm_accept(int fd, struct sockaddr *addr, socklen_t *len);
int mm_connect(int fd, struct sockaddr *name, socklen_t namelen) ;
ssize_t mm_send(int fd, const void *buf, size_t len, int flags);
ssize_t mm_recv(int fd, void *buf, size_t len, int flags);
int mm_close(int fd);
int mm_poll(struct pollfd *fds, nfds_t nfds, int timeout);



#endif /* __DARKBLUE_MM_COROUTINE_H__ */
















