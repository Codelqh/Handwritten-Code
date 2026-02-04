
#pragma once

#include <iostream>
#include <memory>
#include <functional>
#include <sys/epoll.h>
#include <unistd.h>
#include "Timer.h"

#define MAX_EVENTS 1024

class EventLoop
{
public:
    EventLoop() : epfd_(::epoll_create1(0))
    {
        if (epfd_ == -1)
        {
            std::cerr << "epoll_create error: " << errno << std::endl;
            exit(EXIT_FAILURE);
        }
    }

    ~EventLoop()
    {
        close(epfd_);
    }

    void AddEvent(int fd, uint32_t events, void *ptr)
    {
        epoll_event ev;
        ev.events = events;
        ev.data.ptr = ptr;
        if (::epoll_ctl(epfd_, EPOLL_CTL_ADD, fd, &ev) == -1)
        {
            std::cerr << "epoll_ctl add error: " << errno << std::endl;
        }
    }

    void ModEvent(int fd, uint32_t events, void *ptr)
    {
        epoll_event ev;
        ev.events = events;
        ev.data.ptr = ptr;
        if (::epoll_ctl(epfd_, EPOLL_CTL_MOD, fd, &ev) == -1)
        {
            std::cerr << "epoll_ctl mod error: " << errno << std::endl;
        }
    }

    void DelEvent(int fd)
    {
        if (::epoll_ctl(epfd_, EPOLL_CTL_DEL, fd, nullptr) == -1)
        {
            std::cerr << "epoll_ctl del error: " << errno << std::endl;
        }
    }

    void Run()
    {
        epoll_event events[MAX_EVENTS];
        while (true)
        {
            int nfds = ::epoll_wait(epfd_, events, MAX_EVENTS, TimerInstance()->WaitTime());
            if (nfds == -1)
            {
                if (errno == EINTR)
                    continue;
                std::cerr << "epoll_wait error: " << errno << std::endl;
                return;
            }

            for (int i = 0; i < nfds; ++i)
            {
                auto handler = static_cast<std::function<void()> *>(events[i].data.ptr);
                (*handler)();
            }
            // 处理定时器
            TimerInstance()->HandleTimeout();
        }
    }

private:
    int epfd_;
};