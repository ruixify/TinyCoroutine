#ifndef EVENT_LOOP_H
#define EVENT_LOOP_H

#include "scheduler.h"

#include <sys/epoll.h>
#include <unistd.h>

#include <system_error>
#include <unordered_map>

class Event_loop {
public:
    // 读事件的等待体
    struct ReadAwait{
        Event_loop& loop;
        int fd;

        bool await_ready() const noexcept {
            return false;
        }

        void await_suspend(std::coroutine_handle<> handle) {
            loop.add_read_event(fd, handle);
        } 

        void await_resume() const noexcept {}
    };

    // 写事件的等待体
    struct WriteAwait{
        Event_loop& loop;
        int fd;

        bool await_ready() const noexcept {
            return false;
        }

        void await_suspend(std::coroutine_handle<> handle) {
            loop.add_write_event(fd, handle);
        }

        void await_resume() const noexcept {}
    };
public:
    explicit Event_loop(Scheduler& sc) : scheduler_(sc), running_(false) {
        epoll_fd_ = ::epoll_create1(EPOLL_CLOEXEC);

        if(epoll_fd_ == -1) {
            throw std::system_error(errno, std::generic_category(), "epoll_create1");
        }
    }

    ~Event_loop() {
        if(epoll_fd_ != -1) {
            ::close(epoll_fd_);
        }
    }

    // 删除拷贝构造和赋值操作符
    Event_loop(const Event_loop&) = delete;
    Event_loop& operator=(const Event_loop&) = delete;

    Event_loop(Event_loop&&) = delete;
    Event_loop& operator=(Event_loop&&) = delete;

    //  协程读写的挂起点
    ReadAwait read(int fd) noexcept {
        return ReadAwait{*this, fd};
    }

    WriteAwait write(int fd) noexcept {
        return WriteAwait{*this, fd};
    }

    // 将对应的读写事件与对应的协程绑定
    void add_read_event(int fd, std::coroutine_handle<> handle) {
        read_waiters_[fd] = handle;
        update_event(fd, interests_[fd] | EPOLLIN);
    }

    void add_write_event(int fd, std::coroutine_handle<> handle) {
        write_waiters_[fd] = handle;
        update_event(fd, interests_[fd] | EPOLLOUT);
    }

    void stop() noexcept {
        running_ = false;
    }

    void run();

private:
    void update_event(int fd, uint32_t new_events);
    void resume_read_waiter(int fd);
    void resume_write_waiter(int fd);

private:
    int epoll_fd_;
    bool running_;

    Scheduler& scheduler_;

    std::unordered_map<int, uint32_t> interests_;
    std::unordered_map<int, std::coroutine_handle<> > read_waiters_;
    std::unordered_map<int, std::coroutine_handle<> > write_waiters_;
};

// 主循环
inline void Event_loop::run() {
    constexpr int max_events = 64;
    epoll_event events[max_events];

    running_ = true;

    while(running_) {
        scheduler_.run();

        int n = ::epoll_wait(epoll_fd_, events, max_events, -1);

        if(n == -1) {
            if(errno == EINTR) {
                continue;
            }

            throw std::system_error(errno, std::generic_category(), "epoll_wait");
        }

        for(int i = 0; i < n; ++i) {
            int fd = events[i].data.fd;
            auto event = events[i].events;

            if(event & (EPOLLIN | EPOLLERR | EPOLLHUP)) {
                resume_read_waiter(fd);
            }

            if(event & (EPOLLOUT | EPOLLERR | EPOLLHUP)) {
                resume_write_waiter(fd);
            }
        }
    }
}

// 更新对应操作符的事件
inline void Event_loop::update_event(int fd, uint32_t new_events) {
    uint32_t old_events = interests_[fd];

    if(new_events == 0) {
        if(old_events != 0) {
            ::epoll_ctl(epoll_fd_, EPOLL_CTL_DEL, fd, nullptr);
        }

        interests_.erase(fd);
        return;
    }

    epoll_event events{};
    events.data.fd = fd;
    events.events = new_events;

    int op = old_events == 0 ? EPOLL_CTL_ADD : EPOLL_CTL_MOD;

    if(::epoll_ctl(epoll_fd_, op, fd, &events) == -1) {
        throw std::system_error(errno, std::generic_category(), "epoll_ctl");
    }

    interests_[fd] = new_events;

}

inline void Event_loop::resume_read_waiter(int fd) {
    auto it = read_waiters_.find(fd);

    if(it == read_waiters_.end()) {
        return;
    }

    auto handle = it->second;
    read_waiters_.erase(fd);

    update_event(fd, interests_[fd] & ~EPOLLIN);

    scheduler_.schedule(handle);
}

inline void Event_loop::resume_write_waiter(int fd) {
    auto it = write_waiters_.find(fd);

    if(it == write_waiters_.end()) {
        return;
    }

    auto handle = it->second;
    write_waiters_.erase(fd);

    update_event(fd, interests_[fd] & ~EPOLLOUT);

    scheduler_.schedule(handle);
}



#endif