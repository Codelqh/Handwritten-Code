#include "ThreadPool.h"
#include <mutex>
#include <thread>

ThreadPool::ThreadPool(size_t numThreads):
    m_threadsNum(numThreads),
    m_bTerminate(false){
    Init();
}

ThreadPool::~ThreadPool(){
    {
        std::unique_lock<std::mutex> lock(m_mtx);
        m_bTerminate = true;
    }
    m_cv.notify_all();
    for(auto& thread : m_threads){
        if(thread.joinable()){
            thread.join();
        }
    }
}

void ThreadPool::Init(){
    for(int i = 0;i< m_threadsNum;i++){
        m_threads.emplace_back(
            //用this指针，明确是值捕获
            [this]{
                while(!m_bTerminate){
                    Task task = Get();
                    if(task)
                        task();
                    else {
                        return;
                    }
                }
            }
        );
    }
}

Task ThreadPool::Get(){
    std::unique_lock<std::mutex> lock(m_mtx);
    //当队列为空的时候阻塞等待
    //为什么不用while循环判断？
    m_cv.wait(lock,[this]{return !Empty() || m_bTerminate;});
    if(m_bTerminate && Empty()){
        return nullptr;
    }
    auto task = std::move(m_queue.front());
    m_queue.pop();
    return task;
}

bool ThreadPool::Empty(){
    return m_queue.empty();
}