/*
    * Timer.h
    *
    *  Created on: Jan 15, 2024
    *  定时器，基于STL和timerfd实现。
*/
#ifndef TIMER_H_
#define TIMER_H_

#include <cstdint>
#include <sys/epoll.h>
#include <functional>
#include <chrono>
#include <map>
#include <memory>
#include <iostream>
#include <sys/types.h>

/*
    实现要点：
    1. 包装任务，到期时间和对应的回调函数；
    2. 定时器类实现要点：
        提供的接口：
            添加定时器，删除定时器，处理定时器，获取下一个定时器的等待时间；
        内部维护一个有序的定时器容器，使用multimap实现,key为到期时间，value为任务指针；
        
*/



using namespace std;
class Timer;
class TimerTask{
    friend class Timer;
public:
    using cb = function<void(TimerTask*)>;
    TimerTask(uint add_time, uint expire_time, cb callback):
        m_add_time(add_time),
        m_expire_time(expire_time),
        m_callback(callback) 
    {
        
    }
    
    uint AddTime() const {
        return m_add_time;
    }

    uint ExpireTime() const {
        return m_expire_time;
    }
private:
    void run() {
        m_callback(this);
    }
private:
    uint m_add_time;
    uint m_expire_time;
    cb m_callback;
};


class Timer {
using Milliseconds = std::chrono::milliseconds;
public:
    static uint64_t GetTick() {
        auto sc = chrono::time_point_cast<Milliseconds>(chrono::steady_clock::now());
        auto temp = chrono::duration_cast<Milliseconds>(sc.time_since_epoch());
        return temp.count();
    }

    //添加一个定时器，after_sec秒后到期，执行callback回调函数
    TimerTask* AddTimer(uint after_sec, TimerTask::cb callback) {
        time_t now = GetTick();
        TimerTask* task = new TimerTask(now, now + after_sec, callback);
        //如果当前定时器队列为空，或者新添加的定时器到期时间大于最后一个定时器的到期时间，直接插入到末尾
        if(m_timer_map.empty() || m_timer_map.rbegin()->first < now + after_sec) {
            m_timer_map.emplace_hint(m_timer_map.end(), now + after_sec, task);
            return task;
        }
        auto ele = m_timer_map.emplace(now + after_sec, task);
        return ele->second;
    }

    //删除一个定时器
    void DelTimer(TimerTask* task) {
        for(auto it = m_timer_map.begin(); it != m_timer_map.end(); ++it) {
            if(it->second == task) {
                m_timer_map.erase(it);
                delete task;
                break;
            }
        }
    }

    void HandleTimer(time_t now) {
        auto it = m_timer_map.begin();
        while(it != m_timer_map.end() && it->first <= now) {
            TimerTask* task = it->second;
            task->run();
            delete task;
            it = m_timer_map.erase(it);
        }   
    }


    uint WaitTime(){
        auto it = m_timer_map.begin();
        if(it == m_timer_map.end()){
            return -1;
        }
        uint diss = it->first - GetTick();
        return diss > 0 ? diss : 0;
    }


private:
    multimap<time_t, TimerTask*> m_timer_map;
};

#endif /* TIMER_H_ */
