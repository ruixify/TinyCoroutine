#ifndef SCHEDULER_H
#define SCHEDULER_H

#include <queue>

#include "../corotine/task.h"

class Scheduler;

struct Yield {
    Scheduler& scheduler;
    // 一定会挂起
    bool await_ready() const noexcept {
        return false;
    }

    // 当前协程挂起时调用
    void await_suspend(std::coroutine_handle<> handle) noexcept;

    // 协程恢复后，co_await 的结果
    void await_resume() {}
};


class Scheduler {
public:
    // 调度器进行析构前，需要对就绪队列中未完成的协程进行销毁
    ~Scheduler() {
        while(!ready_queue_.empty()) {
            auto handle = ready_queue_.front(); ready_queue_.pop();

            if(handle) {
                handle.destroy();
            }
        }
    }

    // 禁止拷贝构造和移动
    Scheduler(const Scheduler&) = delete;
    Scheduler& operator=(const Scheduler&) = delete;

    Scheduler(Scheduler&&) = delete;
    Scheduler& operator=(Scheduler&&) = delete;


    // 将一个准备好的协程放入就绪队列
    void schedule(std::coroutine_handle<> handle) {
        if(handle) {
            ready_queue_.push(handle);
        }
    }

    // 启动一个 Task
    void spawn(Task task) {
        auto handle = task.release();
        schedule(handle);
    }

    // 调度循环
    void run() {
        while(!ready_queue_.empty()) {
            auto handle = ready_queue_.front(); ready_queue_.pop();

            if(!handle) {
                continue;
            }

            if(handle.done()) {
                handle.destroy();
                continue;
            }

            handle.resume();

            if(handle.done()) {
                handle.destroy();
            }
        }
    }

    Yield yield() noexcept {
        return Yield{*this};
    }

private:
    std::queue<std::coroutine_handle<> > ready_queue_; 
};

inline void Yield::await_suspend(std::coroutine_handle<> handle) noexcept {
    scheduler.schedule(handle);
}

#endif