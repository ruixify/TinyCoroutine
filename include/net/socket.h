#ifndef SOCKET_H
#define SOCKET_H

#include <cerrno>
#include <cstring>
#include <stdexcept>
#include <utility>

#include <fcntl.h>
#include <unistd.h>


class Socket {
public:
    explicit Socket(int fd) : fd_(fd) {}
    ~Socket() {
        close();
    }

    Socket(const Socket&) = delete;
    Socket& operator=(const Socket&) = delete;

    Socket(Socket&& other) : fd_(std::exchange(other.fd_, -1)) {}
    Socket& operator=(Socket&& other) {
        if(&other != this) {
            close();
            fd_ = std::exchange(other.fd_, -1);
        }

        return *this;
    }

    int fd() const noexcept {
        return fd_;
    }

    bool valid() const noexcept {
        return fd_ != -1;
    }

    void set_nonblocking() {
        int old_option = fcntl(fd_, F_GETFL, 0);
        int new_option = old_option | O_NONBLOCK;
        if(fcntl(fd_, F_SETFL, new_option) == -1) {
            throw std::runtime_error(std::strerror(errno));
        }
    }

    ssize_t read(char* buffer, size_t size) {
        return ::read(fd_, buffer, size);
    }

    ssize_t write(const char* buffer, size_t size) {
        return ::write(fd_, buffer, size);
    }

    void close() noexcept {
        if(fd_ != -1) {
            ::close(fd_);
            fd_ = -1;
        }
    }


private:
    int fd_;
};



#endif