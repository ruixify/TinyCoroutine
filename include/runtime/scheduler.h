#ifndef SCHEDULER_H
#define SCHEDULER_H

#include <queue>

#include "../corotine/task.h"

static Scheduler* g_scheduler = nullptr;

class Scheduler {
public:
    Scheduler() {
        g_scheduler = this;
    }

    ~Scheduler() {
        g_scheduler = nullptr;
    }

    // 将一个准备好的协程放入就绪队列
    void schedule(std::coroutine_handle<> handle) {
        ready_queue_.push(handle);
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

            if(!handle || handle.done()) {
                continue;
            }

            handle.resume();

            if(handle.done()) {
                handle.destroy();
            }
        }
    }

private:
    std::queue<std::coroutine_handle<> > ready_queue_; 
};

#endif