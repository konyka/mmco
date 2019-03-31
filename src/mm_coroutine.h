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




#include "mm_queue.h"
#include "mm_tree.h"


#define MM_CO_MAX_EVENT_SIZE    (1024*1024)
#define MM_CO_MAX_STACK_SIZE    (16*1024)



typedef void (*p_co_routine)(void *);

typedef enum {

    MM_COROUTINE_STATUS_BIRTH, 

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
    struct _mm_coroutine *curr_coroutine;
    int page_size;

    int poller_fd;
    int eventfd;
    struct epoll_event eventlist[MM_CO_MAX_EVENT_SIZE];
    int nevents;

    mm_coroutine_queue ready_queue;

    mm_coroutine_list busy_list;
    
    mm_coroutine_rbtree_sleep sleeping_tree;
    mm_coroutine_rbtree_wait waiting_tree;

    //private 

} mm_schedule;






#endif /* __DARKBLUE_MM_COROUTINE_H__ */
















