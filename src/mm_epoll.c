/**
 *===========================================================================
 *  DarkBlue Engine Source File.
 *  Copyright (C), DarkBlue Studios.
 * -------------------------------------------------------------------------
 *    File name: mm_epoll.c
 *      Version: v0.0.0
 *   Created on: Apr 2, 2019 by konyka
 *       Editor: Sublime Text3
 *  Description: 
 * -------------------------------------------------------------------------
 *      History:
 *
 *===========================================================================
 */


#include <sys/eventfd.h>

#include "mm_coroutine.h"


int mm_epoller_create(void) {
    return epoll_create(1024);
} 

int mm_epoller_wait(struct timespec t) {
    mm_schedule *sched = mm_coroutine_get_sched();
    return epoll_wait(sched->poller_fd, sched->eventlist, MM_CO_MAX_EVENT_SIZE, t.tv_sec*1000.0 + t.tv_nsec/1000000.0);
}

int mm_epoller_ev_register_trigger(void) {
    mm_schedule *sched = mm_coroutine_get_sched();

    if (!sched->eventfd) {
        sched->eventfd = eventfd(0, EFD_NONBLOCK);
        assert(sched->eventfd != -1);
    }

    struct epoll_event ev;
    ev.events = EPOLLIN;
    ev.data.fd = sched->eventfd;
    int ret = epoll_ctl(sched->poller_fd, EPOLL_CTL_ADD, sched->eventfd, &ev);

    assert(ret != -1);
}


