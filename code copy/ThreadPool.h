#ifndef THREAD_POOL_H
#define THREAD_POOL_H

#include <vector>
#include <queue>
#include <memory>
#include <thread>
#include <mutex>
#include <condition_variable>
#include <future>
#include <functional>
#include <stdexcept>
#include <iostream>

class ThreadPool {
public:
    ThreadPool(size_t threads = 10);
    
    template<class F, class... Args>
    auto enqueue(F&& f, Args&&... args) 
        -> std::future<typename std::result_of<F(Args...)>::type>;
    
    size_t getWorkerCount() const { return workers.size(); }
    size_t getQueueSize() const { 
        std::unique_lock<std::mutex> lock(queue_mutex);
        return tasks.size(); 
    }
    
    ~ThreadPool();

private:
    // Worker threads
    std::vector< std::thread > workers;
    // Task queue
    std::queue< std::function<void()> > tasks;
    
    // Synchronization
    mutable std::mutex queue_mutex;
    std::condition_variable condition;
    bool stop;
};

// Constructor: 建立指定數量的worker threads
inline ThreadPool::ThreadPool(size_t threads) : stop(false) {
    std::cout << "Creating ThreadPool with " << threads << " workers" << std::endl;
    
    for(size_t i = 0; i < threads; ++i) {
        workers.emplace_back([this, i] {
            std::cout << "Worker " << i << " started (thread ID: " 
                      << std::this_thread::get_id() << ")" << std::endl;
            
            for(;;) {
                std::function<void()> task;

                {
                    std::unique_lock<std::mutex> lock(this->queue_mutex);
                    this->condition.wait(lock,
                        [this]{ return this->stop || !this->tasks.empty(); });
                    
                    if(this->stop && this->tasks.empty())
                        return;
                    
                    task = std::move(this->tasks.front());
                    this->tasks.pop();
                }

                try {
                    task();
                } catch(const std::exception& e) {
                    std::cerr << "Worker exception: " << e.what() << std::endl;
                } catch(...) {
                    std::cerr << "Worker unknown exception" << std::endl;
                }
            }
        });
    }
}

// 將任務加入佇列
template<class F, class... Args>
auto ThreadPool::enqueue(F&& f, Args&&... args) 
    -> std::future<typename std::result_of<F(Args...)>::type> {
    
    using return_type = typename std::result_of<F(Args...)>::type;

    auto task = std::make_shared< std::packaged_task<return_type()> >(
            std::bind(std::forward<F>(f), std::forward<Args>(args)...)
        );

    std::future<return_type> res = task->get_future();
    {
        std::unique_lock<std::mutex> lock(queue_mutex);

        // 檢查ThreadPool是否已停止
        if(stop)
            throw std::runtime_error("enqueue on stopped ThreadPool");

        tasks.emplace([task](){ (*task)(); });
    }
    condition.notify_one();
    return res;
}

// Destructor: 停止所有worker threads
inline ThreadPool::~ThreadPool() {
    std::cout << "Shutting down ThreadPool..." << std::endl;
    {
        std::unique_lock<std::mutex> lock(queue_mutex);
        stop = true;
    }
    condition.notify_all();
    for(std::thread &worker: workers) {
        if(worker.joinable()) {
            worker.join();
        }
    }
    std::cout << "ThreadPool shutdown complete" << std::endl;
}

#endif // THREAD_POOL_H