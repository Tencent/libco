#include <iostream>
#include <map>
#include <memory>
#include <sys/poll.h>
#include "epoll.h"

using namespace std;

static uint32_t PollEvent2Epoll( short events )
{
    uint32_t e = 0;
    if( events & POLLIN ) 	e |= EPOLLIN;
    if( events & POLLOUT )  e |= EPOLLOUT;
    if( events & POLLHUP ) 	e |= EPOLLHUP;
    if( events & POLLERR )	e |= EPOLLERR;
    if( events & POLLRDNORM ) e |= EPOLLRDNORM;
    if( events & POLLWRNORM ) e |= EPOLLWRNORM;
    return e;
}
static short EpollEvent2Poll( uint32_t events )
{
    short e = 0;
    if( events & EPOLLIN ) 	e |= POLLIN;
    if( events & EPOLLOUT ) e |= POLLOUT;
    if( events & EPOLLHUP ) e |= POLLHUP;
    if( events & EPOLLERR ) e |= POLLERR;
    if( events & EPOLLRDNORM ) e |= POLLRDNORM;
    if( events & EPOLLWRNORM ) e |= POLLWRNORM;
    return e;
}

class Event {
public:
    Event(int fd, short event, epoll_data_t ep_data) : fd(fd), event(event), ep_data(ep_data) { }

    int fd;
    short event;
    epoll_data_t ep_data;
};

typedef map<int, shared_ptr<Event>> fdEvent_t;

static fdEvent_t *gEpollArr[1024];
static int gEpollArrIndex = 0;

static int createEpollStruct(int size)
{
    gEpollArr[gEpollArrIndex] = new fdEvent_t;
    return gEpollArrIndex++;
}

static fdEvent_t *getEpollEventMap(int epfd)
{
    if (epfd < gEpollArrIndex && epfd >= 0) {
        return gEpollArr[epfd];
    }
    return NULL;
}


int epoll_create(int size)
{
    return createEpollStruct(size);
}

int epoll_ctl(int epfd, int op, int fd, struct epoll_event *event)
{
    fdEvent_t *fdevents = getEpollEventMap(epfd);
    if (fdevents == NULL)
    {
        return -1;
    }
    switch (op) {
        case EPOLL_CTL_ADD:
        {
            short pollev = EpollEvent2Poll(event->events);
            auto it = fdevents->find(fd);
            if (it == fdevents->end()) {
                fdevents->insert(make_pair(fd, make_shared<Event>(fd, pollev, event->data)));
            } else {
                it->second->event = pollev;
                it->second->ep_data = event->data;
            }
        }
            break;
        case EPOLL_CTL_DEL:
        {
            auto it = fdevents->find(fd);
            if (it != fdevents->end()) {
                fdevents->erase(fd);
            }
        }
            break;
        case EPOLL_CTL_MOD:
        {
            auto it = fdevents->find(fd);
            if (it != fdevents->end()) {
                it->second->event = EpollEvent2Poll(event->events);
                it->second->ep_data = event->data;
            } else {
                return -3;
            }
        }
            break;
        default:
            return -2;
    }
    return 0;
}

int epoll_wait(int epfd, struct epoll_event *events, int maxevents, int timeout)
{
    static pollfd pfdsArr[1024*10];
    fdEvent_t *fdevents = getEpollEventMap(epfd);
    if (fdevents == NULL) {
        return -1;
    }
    int pollArrIdx = 0;
    for (auto it = fdevents->begin();
         it != fdevents->end() && pollArrIdx < maxevents;
         ++it, ++pollArrIdx) {
        pfdsArr[pollArrIdx].fd = it->first;
        pfdsArr[pollArrIdx].events = it->second->event;
        pfdsArr[pollArrIdx].revents = 0;
    }

    int retval = poll(pfdsArr, (nfds_t) pollArrIdx, timeout);
    if (retval <= 0) {
        return retval;
    }

    int epollIdx = 0;
    for (int i = 0; i < pollArrIdx && epollIdx < maxevents; ++i) {
        if (pfdsArr[i].revents != 0) {
            events[epollIdx].events = PollEvent2Epoll(pfdsArr[i].revents);
            events[epollIdx].data = fdevents->find(pfdsArr[i].fd)->second->ep_data;
            epollIdx++;
        }
    }
    return epollIdx;
}
