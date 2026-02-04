#include <mutex>
#include <thread>
#include <vector>
#include <functional>
#include <queue>
#include <condition_variable>
#include <atomic>
#include <future>
#include <memory>

//为什么要放这？
using Task = std::function<void()>;

class ThreadPool {
public:
    ThreadPool(size_t numThreads = std::thread::hardware_concurrency());
    ~ThreadPool();

    //任务入队列
    template<typename F,typename... Args>
    auto Enqueue(F&& f,Args&&... args) -> std::future<decltype(f(args...))>{
        using RetType = decltype(f(args...));
        auto task = std::make_shared<std::packaged_task<RetType()>>(
            std::bind(std::forward<F>(f),std::forward<Args>(args)...)
        );
        auto ret = task->get_future();
        {
            std::unique_lock<std::mutex> lock(m_mtx);
            m_queue.emplace([task](){ (*task)(); });
        }
        m_cv.notify_one();
        return ret;
    }

private:
    void Init();
    Task Get();
    bool Empty();
private:
    bool m_bTerminate;

    std::vector<std::thread> m_threads;
    std::queue<Task> m_queue;
    int m_threadsNum;
    std::mutex m_mtx;
    std::condition_variable m_cv;
};