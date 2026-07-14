#ifndef TASK_H
#define TASK_H

#include <coroutine>
#include <exception>
#include <utility>

struct Task {
    struct promise_type;        // 承诺对象，用于定义协程的行为
    using handle_type = std::coroutine_handle<promise_type>;
    handle_type coro;           // 协程句柄，用于控制协程

    // 构造函数
    explicit Task(handle_type h) : coro(h) {}

    // 删除拷贝构造和拷贝赋值操作符
    Task(const Task&) = delete;
    Task& operator=(const Task&) = delete;

    // 定义移动构造和移动操作符
    Task(Task&& other) : coro(std::exchange(other.coro, nullptr)) {}
    Task& operator=(Task&& other) {
        if(this != &other) {
            if(coro) {
                coro.destroy();
            }

            coro = std::exchange(other.coro, nullptr);
        }

        return *this;
    }

    // 析构函数
    ~Task() {
        if(coro) {
            coro.destroy();
        }
    }

    // 由于调度器接管了协程，所以如果协程自己析构之后
    // 可能会导致调度器使用空指针
    handle_type release() {
        return std::exchange(coro, nullptr);
    }

    // 定义协程的resume行为
    bool resume() {
        if(coro && !coro.done()) {
            coro.resume();
            return true;
        }

        return false;
    }

    // 定义协程的done行为
    bool done() const {
        return !coro || coro.done();
    }

    bool await_ready() {
        return !coro || coro.done();
    }

    std::coroutine_handle<> await_suspend(std::coroutine_handle<> parent) {
        coro.promise().continuation = parent;
        return coro;
    }

    void await_resume() const noexcept {}


    // 完善定义承诺对象
    struct promise_type {
        std::coroutine_handle<> continuation{};

        // 创建并返回协程对象
        Task get_return_object() {
            return Task {
                handle_type::from_promise(*this)
            };
        }

        // 定义协程创建时的行为
        std::suspend_always initial_suspend() { return {}; }
        
        struct FinalAwaiter {
            bool await_ready() noexcept {
                return false;
            }

            std::coroutine_handle<> await_suspend(handle_type h) noexcept {
                auto continuation = h.promise().continuation;

                if(continuation) {
                    return continuation;
                }

                return std::noop_coroutine();
            }

            void await_resume() noexcept {}
        };

        // 定义协程结束时的行为
        FinalAwaiter final_suspend() noexcept { return {}; }

        // 处理 co_return 
        // 由于当前Task不返回任何值，所以为 return_void
        void return_void() noexcept {}

        // 处理过程未处理的异常
        void unhandled_exception() {
            std::terminate();
        }
    };
};

#endif