#include <iostream>
#include <coroutine>
#include <exception>
#include <utility>

struct Task {
    struct promise_type;

    using handle_type = std::coroutine_handle<promise_type>;

    handle_type coro;

    explicit Task(handle_type h)
        : coro(h) {}

    Task(const Task&) = delete;
    Task& operator=(const Task&) = delete;

    Task(Task&& other) noexcept
        : coro(std::exchange(other.coro, nullptr)) {}

    Task& operator=(Task&& other) noexcept {
        if (this != &other) {
            if (coro) {
                coro.destroy();
            }
            coro = std::exchange(other.coro, nullptr);
        }
        return *this;
    }

    ~Task() {
        if (coro) {
            coro.destroy();
        }
    }

    bool resume() {
        if (coro && !coro.done()) {
            coro.resume();
            return true;
        }
        return false;
    }

    bool done() const {
        return !coro || coro.done();
    }

    struct promise_type {
        Task get_return_object() {
            return Task{
                handle_type::from_promise(*this)
            };
        }

        std::suspend_always initial_suspend() noexcept {
            return {};
        }

        std::suspend_always final_suspend() noexcept {
            return {};
        }

        void return_void() noexcept {}

        void unhandled_exception() {
            std::terminate();
        }
    };
};

Task simple_coroutine() {
    std::cout << "Coroutine started" << std::endl;

    co_await std::suspend_always{};

    std::cout << "Coroutine finished" << std::endl;

    co_return;
}

int main() {
    Task t = simple_coroutine();

    std::cout << "Main function" << std::endl;

    t.resume();

    std::cout << "Back to main" << std::endl;

    t.resume();

    std::cout << "Coroutine done" << std::endl;

    return 0;
}