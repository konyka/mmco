/**
 *===========================================================================
 *  DarkBlue Engine Source File.
 *  Copyright (C), DarkBlue Studios.
 * -------------------------------------------------------------------------
 *    File name: mm_schedule.c
 *      Version: v0.0.0
 *   Created on: Apr 2, 2019 by konyka
 *       Editor: Sublime Text3
 *  Description: 
 * -------------------------------------------------------------------------
 *      History:
 *
 *===========================================================================
 */
 

#include "mm_coroutine.h"



#define FD_KEY(f,e) (((int64_t)(f) << (sizeof(int32_t) * 8)) | e)
#define FD_EVENT(f) ((int32_t)(f))
#define FD_ONLY(f) ((f) >> ((sizeof(int32_t) * 8)))


static inline int mm_coroutine_sleep_cmp(mm_coroutine *co1, mm_coroutine *co2) {
    if (co1->sleep_usecs < co2->sleep_usecs) {
        return -1;
    }
    if (co1->sleep_usecs == co2->sleep_usecs) {
        return 0;
    }
    return 1;
}

static inline int mm_coroutine_wait_cmp(mm_coroutine *co1, mm_coroutine *co2) {
#if CANCEL_FD_WAIT_UINT64
    if (co1->fd < co2->fd) return -1;
    else if (co1->fd == co2->fd) return 0;
    else return 1;
#else
    if (co1->fd_wait < co2->fd_wait) {
        return -1;
    }
    if (co1->fd_wait == co2->fd_wait) {
        return 0;
    }
#endif
    return 1;
}


RB_GENERATE(_mm_coroutine_rbtree_sleep, _mm_coroutine, sleep_node, mm_coroutine_sleep_cmp);
RB_GENERATE(_mm_coroutine_rbtree_wait, _mm_coroutine, wait_node, mm_coroutine_wait_cmp);



void mm_schedule_sched_sleepdown(mm_coroutine *co, uint64_t msecs) {
    uint64_t usecs = msecs * 1000u;

    mm_coroutine *co_tmp = RB_FIND(_mm_coroutine_rbtree_sleep, &co->sched->sleeping_tree, co);
    if (co_tmp != NULL) {
        RB_REMOVE(_mm_coroutine_rbtree_sleep, &co->sched->sleeping_tree, co_tmp);
    }

    co->sleep_usecs = mm_coroutine_diff_usecs(co->sched->birth, mm_coroutine_usec_now()) + usecs;

    while (msecs) {
        co_tmp = RB_INSERT(_mm_coroutine_rbtree_sleep, &co->sched->sleeping_tree, co);
        if (co_tmp) {
            printf("1111 sleep_usecs %"PRIu64"\n", co->sleep_usecs);
            co->sleep_usecs ++;
            continue;
        }
        co->status |= BIT(MM_COROUTINE_STATUS_SLEEPING);
        break;
    }

    //yield
}

void mm_schedule_desched_sleepdown(mm_coroutine *co) {
    if (co->status & BIT(MM_COROUTINE_STATUS_SLEEPING)) {
        RB_REMOVE(_mm_coroutine_rbtree_sleep, &co->sched->sleeping_tree, co);

        co->status &= CLEARBIT(MM_COROUTINE_STATUS_SLEEPING);
        co->status |= BIT(MM_COROUTINE_STATUS_READY);
        co->status &= CLEARBIT(MM_COROUTINE_STATUS_EXPIRED);
    }
}

mm_coroutine *mm_schedule_search_wait(int fd) {
    mm_coroutine find_it = {0};
    find_it.fd = fd;

    mm_schedule *sched = mm_coroutine_get_sched();
    
    mm_coroutine *co = RB_FIND(_mm_coroutine_rbtree_wait, &sched->waiting_tree, &find_it);
    co->status = 0;

    return co;
}

mm_coroutine* mm_schedule_desched_wait(int fd) {
    
    mm_coroutine find_it = {0};
    find_it.fd = fd;

    mm_schedule *sched = mm_coroutine_get_sched();
    
    mm_coroutine *co = RB_FIND(_mm_coroutine_rbtree_wait, &sched->waiting_tree, &find_it);
    if (co != NULL) {
        RB_REMOVE(_mm_coroutine_rbtree_wait, &co->sched->waiting_tree, co);
    }
    co->status = 0;
    mm_schedule_desched_sleepdown(co);
    
    return co;
}

void mm_schedule_sched_wait(mm_coroutine *co, int fd, unsigned short events, uint64_t timeout) {
    
    if (co->status & BIT(MM_COROUTINE_STATUS_WAIT_READ) ||
        co->status & BIT(MM_COROUTINE_STATUS_WAIT_WRITE)) {
        printf("Unexpected event. lt id %"PRIu64" fd %"PRId32" already in %"PRId32" state\n",
            co->id, co->fd, co->status);
        assert(0);
    }

    if (events & POLLIN) {
        co->status |= MM_COROUTINE_STATUS_WAIT_READ;
    } else if (events & POLLOUT) {
        co->status |= MM_COROUTINE_STATUS_WAIT_WRITE;
    } else {
        printf("events : %d\n", events);
        assert(0);
    }

    co->fd = fd;
    co->events = events;
    mm_coroutine *co_tmp = RB_INSERT(_mm_coroutine_rbtree_wait, &co->sched->waiting_tree, co);

    assert(co_tmp == NULL);

    //printf("timeout --> %"PRIu64"\n", timeout);
    if (timeout == 1) return ; //Error

    mm_schedule_sched_sleepdown(co, timeout);
    
}

void mm_schedule_cancel_wait(mm_coroutine *co) {
    RB_REMOVE(_mm_coroutine_rbtree_wait, &co->sched->waiting_tree, co);
}

void mm_schedule_free(mm_schedule *sched) {
    if (sched->poller_fd > 0) {
        close(sched->poller_fd);
    }
    if (sched->eventfd > 0) {
        close(sched->eventfd);
    }
    
    free(sched);

    assert(pthread_setspecific(global_sched_key, NULL) == 0);
}

int mm_schedule_create(int stack_size) {

    int sched_stack_size = stack_size ? stack_size : MM_CO_MAX_STACK_SIZE;

    mm_schedule *sched = (mm_schedule*)calloc(1, sizeof(mm_schedule));
    if (sched == NULL) {
        printf("Failed to initialize scheduler\n");
        return -1;
    }

    assert(pthread_setspecific(global_sched_key, sched) == 0);

    sched->poller_fd = mm_epoller_create();
    if (sched->poller_fd == -1) {
        printf("Failed to initialize epoller\n");
        mm_schedule_free(sched);
        return -2;
    }

    mm_epoller_ev_register_trigger();

    sched->stack_size = sched_stack_size;
    sched->page_size = getpagesize();

    sched->spawned_coroutines = 0;
    sched->default_timeout = 3000000u;

    RB_INIT(&sched->sleeping_tree);
    RB_INIT(&sched->waiting_tree);

    sched->birth = mm_coroutine_usec_now();

    TAILQ_INIT(&sched->ready_queue);
    //TAILQ_INIT(&sched->defer);
    LIST_INIT(&sched->busy_list);

    bzero(&sched->ctx, sizeof(mm_cpu_ctx));
}


static mm_coroutine *mm_schedule_expired(mm_schedule *sched) {
    
    uint64_t t_diff_usecs = mm_coroutine_diff_usecs(sched->birth, mm_coroutine_usec_now());
    mm_coroutine *co = RB_MIN(_mm_coroutine_rbtree_sleep, &sched->sleeping_tree);
    if (co == NULL) return NULL;
    
    if (co->sleep_usecs <= t_diff_usecs) {
        RB_REMOVE(_mm_coroutine_rbtree_sleep, &co->sched->sleeping_tree, co);
        return co;
    }
    return NULL;
}

static inline int mm_schedule_isdone(mm_schedule *sched) {
    return (RB_EMPTY(&sched->waiting_tree) && 
        LIST_EMPTY(&sched->busy_list) &&
        RB_EMPTY(&sched->sleeping_tree) &&
        TAILQ_EMPTY(&sched->ready_queue));
}

static uint64_t mm_schedule_min_timeout(mm_schedule *sched) {
    uint64_t t_diff_usecs = mm_coroutine_diff_usecs(sched->birth, mm_coroutine_usec_now());
    uint64_t min = sched->default_timeout;

    mm_coroutine *co = RB_MIN(_mm_coroutine_rbtree_sleep, &sched->sleeping_tree);
    if (!co) return min;

    min = co->sleep_usecs;
    if (min > t_diff_usecs) {
        return min - t_diff_usecs;
    }

    return 0;
} 

static int mm_schedule_epoll(mm_schedule *sched) {

    sched->num_new_events = 0;

    struct timespec t = {0, 0};
    uint64_t usecs = mm_schedule_min_timeout(sched);
    if (usecs && TAILQ_EMPTY(&sched->ready_queue)) {
        t.tv_sec = usecs / 1000000u;
        if (t.tv_sec != 0) {
            t.tv_nsec = (usecs % 1000u) * 1000u;
        } else {
            t.tv_nsec = usecs * 1000u;
        }
    } else {
        return 0;
    }

    int nready = 0;
    while (1) {
        nready = mm_epoller_wait(t);
        if (nready == -1) {
            if (errno == EINTR) continue;
            else assert(0);
        }
        break;
    }

    sched->nevents = 0;
    sched->num_new_events = nready;

    return 0;
}

void mm_schedule_run(void) {

    mm_schedule *sched = mm_coroutine_get_sched();
    if (sched == NULL) return ;

    while (!mm_schedule_isdone(sched)) {
        
        // 1. expired --> sleep rbtree
        mm_coroutine *expired = NULL;
        while ((expired = mm_schedule_expired(sched)) != NULL) {
            mm_coroutine_resume(expired);
        }
        // 2. ready queue
        mm_coroutine *last_co_ready = TAILQ_LAST(&sched->ready_queue, _mm_coroutine_queue);
        while (!TAILQ_EMPTY(&sched->ready_queue)) {
            mm_coroutine *co = TAILQ_FIRST(&sched->ready_queue);
            TAILQ_REMOVE(&co->sched->ready_queue, co, ready_next);

            if (co->status & BIT(MM_COROUTINE_STATUS_FDEOF)) {
                mm_coroutine_free(co);
                break;
            }

            mm_coroutine_resume(co);
            if (co == last_co_ready) break;
        }

        // 3. wait rbtree
        mm_schedule_epoll(sched);
        while (sched->num_new_events) {
            int idx = --sched->num_new_events;
            struct epoll_event *ev = sched->eventlist+idx;
            
            int fd = ev->data.fd;
            int is_eof = ev->events & EPOLLHUP;
            if (is_eof) errno = ECONNRESET;

            mm_coroutine *co = mm_schedule_search_wait(fd);
            if (co != NULL) {
                if (is_eof) {
                    co->status |= BIT(MM_COROUTINE_STATUS_FDEOF);
                }
                mm_coroutine_resume(co);
            }

            is_eof = 0;
        }
    }

    mm_schedule_free(sched);
    
    return ;
}






