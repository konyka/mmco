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





#endif /* __DARKBLUE_MM_COROUTINE_H__ */
















