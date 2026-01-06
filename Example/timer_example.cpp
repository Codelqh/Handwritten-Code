#include <Timer.h>
#include <iostream>
#include <thread>
#include <sys/epoll.h>

using namespace std;

int main() {
    int epfd = epoll_create(1);

    unique_ptr<Timer> timer = make_unique<Timer>();

    int i = 0;

    timer->AddTimer(1000, [&](TimerTask *task) {
        cout << Timer::GetTick() << " addtime:" << task->AddTime() << " revoked times:" << ++i << endl;
    });


    timer->AddTimer(2000, [&](TimerTask *task) {
        cout << Timer::GetTick() << " addtime:" << task->AddTime() << " revoked times:" << ++i << endl;
    });

    timer->AddTimer(3000, [&](TimerTask *task) {
        cout << Timer::GetTick() << " addtime:" << task->AddTime() << " revoked times:" << ++i << endl;
    });

    auto task = timer->AddTimer(2100, [&](TimerTask *task) {
        cout << Timer::GetTick() << " addtime:" << task->AddTime() << " revoked times:" << ++i << endl;
    });

    timer->DelTimer(task);
    cout << "now time:" << Timer::GetTick() << endl;
    epoll_event ev[64] = {0};

    while (true) {
        cout << "waittime:" << timer->WaitTime() << endl;
        int n = epoll_wait(epfd, ev, 64, timer->WaitTime());
        time_t now = Timer::GetTick();
        for (int i = 0; i < n; i++) {
            /**/
        }
        /* 处理定时事件*/
        timer->HandleTimer(now);
    }
    
    return 0;
}