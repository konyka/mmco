/**
 *===========================================================================
 *  DarkBlue Engine Source File.
 *  Copyright (C), DarkBlue Studios.
 * -------------------------------------------------------------------------
 *    File name: mm_socket.c
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



#define handle_error(msg) \
           do { perror(msg); return(EXIT_FAILURE); } while (0)

static uint32_t mm_pollevent_2epoll( short events )
{
    uint32_t e = 0; 
    if( events & POLLIN )   e |= EPOLLIN;
    if( events & POLLOUT )  e |= EPOLLOUT;
    if( events & POLLHUP )  e |= EPOLLHUP;
    if( events & POLLERR )  e |= EPOLLERR;
    if( events & POLLRDNORM ) e |= EPOLLRDNORM;
    if( events & POLLWRNORM ) e |= EPOLLWRNORM;
    return e;
}

static short mm_epollevent_2poll( uint32_t events )
{
    short e = 0;    
    if( events & EPOLLIN )  e |= POLLIN;
    if( events & EPOLLOUT ) e |= POLLOUT;
    if( events & EPOLLHUP ) e |= POLLHUP;
    if( events & EPOLLERR ) e |= POLLERR;
    if( events & EPOLLRDNORM ) e |= POLLRDNORM;
    if( events & EPOLLWRNORM ) e |= POLLWRNORM;
    return e;
}


/*
 * mm_poll_inner --> 1. sockfd--> epoll, 2 yield, 3. epoll x sockfd
 * pfds : 
 */

static int mm_poll_inner(struct pollfd *pfds, nfds_t nfds, int timeout) {

    if (timeout == 0)
    {
        return poll(pfds, nfds, timeout);
    }
    if (timeout < 0)
    {
        timeout = INT_MAX;
    }

    mm_schedule *sched = mm_coroutine_get_sched();
    mm_coroutine *co = sched->curr_thread;
    
    int i = 0;
    for (i = 0;i < nfds;i ++) {
    
        struct epoll_event ev;
        ev.events = mm_pollevent_2epoll(pfds[i].events);
        ev.data.fd = pfds[i].fd;
        epoll_ctl(sched->poller_fd, EPOLL_CTL_ADD, pfds[i].fd, &ev);

        co->events = pfds[i].events;
        mm_schedule_sched_wait(co, pfds[i].fd, pfds[i].events, timeout);
    }
    mm_coroutine_yield(co); 

    for (i = 0;i < nfds;i ++) {
    
        struct epoll_event ev;
        ev.events = mm_pollevent_2epoll(pfds[i].events);
        ev.data.fd = pfds[i].fd;
        epoll_ctl(sched->poller_fd, EPOLL_CTL_DEL, pfds[i].fd, &ev);

        mm_schedule_desched_wait(pfds[i].fd);
    }

    return nfds;
}

int mm_socket(int domain, int type, int protocol) {

    int fd = socket(domain, type, protocol);
    if (fd == -1) {
        handle_error("Failed to create a new socket\n");
    }
    int ret = fcntl(fd, F_SETFL, O_NONBLOCK);
    if (ret == -1) {
        close(ret);
        return -1;
    }
    int reuse = 1;
    setsockopt(fd, SOL_SOCKET, SO_REUSEADDR, (char *)&reuse, sizeof(reuse));
    
    return fd;
}


//mm_accept 
//return failed == -1, success > 0

int mm_accept(int fd, struct sockaddr *addr, socklen_t *len) {
    int sockfd = -1;
    int timeout = 1;
    mm_coroutine *co = mm_coroutine_get_sched()->curr_thread;
    
    while (1) {
        struct pollfd pfds;
        pfds.fd = fd;
        pfds.events = POLLIN | POLLERR | POLLHUP;
        mm_poll_inner(&pfds, 1, timeout);

        sockfd = accept(fd, addr, len);
        if (sockfd < 0) {
            if (errno == EAGAIN) {
                continue;
            } else if (errno == ECONNABORTED) {
                printf("accept : ECONNABORTED\n");
                
            } else if (errno == EMFILE || errno == ENFILE) {
                printf("accept : EMFILE || ENFILE\n");
            }
            return -1;
        } else {
            break;
        }
    }

    int ret = fcntl(sockfd, F_SETFL, O_NONBLOCK);
    if (ret == -1) {
        close(sockfd);
        return -1;
    }
    int reuse = 1;
    setsockopt(fd, SOL_SOCKET, SO_REUSEADDR, (char *)&reuse, sizeof(reuse));
    
    return sockfd;
}

int mm_connect(int fd, struct sockaddr *name, socklen_t namelen) {

    int ret = 0;

    while (1) {

        struct pollfd fds;
        fds.fd = fd;
        fds.events = POLLOUT | POLLERR | POLLHUP;
        mm_poll_inner(&fds, 1, 1);

        ret = connect(fd, name, namelen);
        if (ret == 0) break;

        if (ret == -1 && (errno == EAGAIN ||
            errno == EWOULDBLOCK || 
            errno == EINPROGRESS)) {
            continue;
        } else {
            break;
        }
    }

    return ret;
}


ssize_t mm_send(int fd, const void *buf, size_t len, int flags) {
    
    int sent = 0;

    int ret = send(fd, ((char*)buf)+sent, len-sent, flags);
    if (ret == 0) return ret;
    if (ret > 0) sent += ret;

    while (sent < len) {
        struct pollfd pfds;
        pfds.fd = fd;
        pfds.events = POLLOUT | POLLERR | POLLHUP;

        mm_poll_inner(&pfds, 1, 1);
        ret = send(fd, ((char*)buf)+sent, len-sent, flags);
        //printf("send --> len : %d\n", ret);
        if (ret <= 0) {         
            break;
        }
        sent += ret;
    }

    if (ret <= 0 && sent == 0) return ret;
    
    return sent;
}


//recv 
// add epoll first
//
ssize_t mm_recv(int fd, void *buf, size_t len, int flags) {
    
    struct pollfd pfds;
    pfds.fd = fd;
    pfds.events = POLLIN | POLLERR | POLLHUP;

    mm_poll_inner(&pfds, 1, 1);

    int ret = recv(fd, buf, len, flags);
    if (ret < 0) {
        //if (errno == EAGAIN) return ret;
        if (errno == ECONNRESET) return -1;
        //printf("recv error : %d, ret : %d\n", errno, ret);
        
    }
    return ret;
}

int mm_close(int fd) {
#if 0
    mm_schedule *sched = mm_coroutine_get_sched();

    mm_coroutine *co = sched->curr_thread;
    if (co) {
        TAILQ_INSERT_TAIL(&mm_coroutine_get_sched()->ready, co, ready_next);
        co->status |= BIT(mm_COROUTINE_STATUS_FDEOF);
    }
#endif  
    return close(fd);
}

ssize_t mm_sendto(int fd, const void *buf, size_t len, int flags,
               const struct sockaddr *dest_addr, socklen_t addrlen) {


    int sent = 0;

    while (sent < len) {
        struct pollfd fds;
        fds.fd = fd;
        fds.events = POLLOUT | POLLERR | POLLHUP;

        mm_poll_inner(&fds, 1, 1);
        int ret = sendto(fd, ((char*)buf)+sent, len-sent, flags, dest_addr, addrlen);
        if (ret <= 0) {
            if (errno == EAGAIN) continue;
            else if (errno == ECONNRESET) {
                return ret;
            }
            printf("send errno : %d, ret : %d\n", errno, ret);
            assert(0);
        }
        sent += ret;
    }
    return sent;
    
}

ssize_t mm_recvfrom(int fd, void *buf, size_t len, int flags,
                 struct sockaddr *src_addr, socklen_t *addrlen) {

    struct pollfd fds;
    fds.fd = fd;
    fds.events = POLLIN | POLLERR | POLLHUP;

    mm_poll_inner(&fds, 1, 1);

    int ret = recvfrom(fd, buf, len, flags, src_addr, addrlen);
    if (ret < 0) {
        if (errno == EAGAIN) return ret;
        if (errno == ECONNRESET) return 0;
        
        printf("recv error : %d, ret : %d\n", errno, ret);
        assert(0);
    }
    return ret;

}







