#pragma once

// #include <sys/epoll.h>
// #include <unistd.h>
#include <map>
#include <functional>
#include <chrono>

class TimerNode {
public:
    friend class Timer;
    TimerNode(uint64_t timeout, std::function<void()> callback)
        : timeout_(timeout), callback_(std::move(callback)) {}
private:
    int id;
    uint64_t timeout_;
    std::function<void()> callback_;
};

class Timer {
public:
    static Timer* GetInstance() {
        static Timer instance;
        return &instance;
    }

    static uint64_t GetCurrentTime() {
        using namespace std::chrono;
        return duration_cast<milliseconds>(steady_clock::now().time_since_epoch()).count();
    }

    TimerNode* AddTimeout(uint64_t diff, std::function<void()> cb) {
        auto node  =  new TimerNode(GetCurrentTime() + diff, std::move(cb));
        if (timer_map_.empty() || node->timeout_ < timer_map_.rbegin()->first) {
            auto it = timer_map_.insert(std::make_pair(node->timeout_, std::move(node)));
            return it->second;
        } else {
            auto it = timer_map_.emplace_hint(timer_map_.crbegin().base(), std::make_pair(node->timeout_, std::move(node)));
            return it->second;
        }
    };

    void DelTimeout(TimerNode* node) {
        auto it = timer_map_.equal_range(node->timeout_);
        for (auto iter = it.first; iter != it.second; ++iter) {
            if (iter->second == node) {
                timer_map_.erase(iter);
                break;
            }
        }
    }

    // 
    int WaitTime() {
        auto iter = timer_map_.begin();
        if (iter == timer_map_.end()) {
            return -1;
        }
        uint64_t diff = iter->first - GetCurrentTime();
        return diff > 0 ? diff : 0;
    }

    void HandleTimeout() {
        auto iter = timer_map_.begin();
        while (iter != timer_map_.end() && iter->first <= GetCurrentTime()) {
            iter->second->callback_();
            iter = timer_map_.erase(iter); // 删除已处理的定时器
        }
    }
private:
    std::multimap<uint64_t, TimerNode*> timer_map_;

    Timer() = default;
    Timer(const Timer&) = delete;
    Timer& operator=(const Timer&) = delete;
    Timer(Timer&&) = delete;
    Timer& operator=(Timer&&) = delete;
    ~Timer() {
        for (auto& pair : timer_map_) {
            delete pair.second;
        }
    }
};

#define TimerInstance Timer::GetInstance

/*

int main() {
    int epfd = epoll_create1(0);
    if (epfd == -1) {
        std::cerr << "epoll_create error: " << errno << std::endl;
        return -1;
    }

    Timer timer;

    int i = 0;
    timer.AddTimeout(1000, [&]() {
        std::cout << "Timeout 1 second:" << i++ << std::endl;
    });

    timer.AddTimeout(2000, [&]() {
        std::cout << "Timeout 2 seconds:" << i++ << std::endl;
    });

    auto node = timer.AddTimeout(3000, [&]() {
        std::cout << "Timeout 3 seconds:" << i++ << std::endl;
    });

    timer.DelTimeout(node);

    epoll_event evs[512];

    while (true) {
        int n = epoll_wait(epfd, evs, 512, timer.WaitTime());
        if (n == -1) {
            if (errno == EINTR) {
                continue; // Interrupted by a signal, retry
            }
            std::cerr << "epoll_wait error: " << errno << std::endl;
            break;
        }
        // 处理延时任务
        timer.HandleTimeout();
    }

    return 0;
}
*/
