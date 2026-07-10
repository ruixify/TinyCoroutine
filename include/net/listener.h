#ifndef LISTENER_H
#define LISTENER_H

#include <cerrno>
#include <cstring>
#include <stdexcept>
#include <utility>

#include <fcntl.h>
#include <unistd.h>
#include <arpa/inet.h>
#include <netinet/in.h>

#include "socket.h"

class Listener {
public:
    static Listener bind(uint32_t port, int backlog = SOMAXCONN) {
        int fd = socket(PF_INET, SOCK_STREAM, 0);
        if(fd == -1) {
            throw std::runtime_error(std::strerror(errno));
        }

        int op = 1;
        if(setsockopt(fd, SOL_SOCKET, SO_REUSEADDR, &op, sizeof(op)) == -1) {
            throw std::runtime_error(std::strerror(errno));
        }

        struct sockaddr_in skaddr;
        skaddr.sin_family = AF_INET;
        skaddr.sin_addr.s_addr = htonl(INADDR_ANY);
        skaddr.sin_port = htons(port);

        if(::bind(fd, (struct sockaddr*)(&skaddr), sizeof(skaddr)) == -1) {
            ::close(fd);
            throw std::runtime_error(std::strerror(errno));
        }

        if(::listen(fd, backlog) == -1) {
            ::close(fd);
            throw std::runtime_error(std::strerror(errno));
        }

        Listener listener(fd);
        listener.set_nonblocking();

        return listener;
        
    }

    Listener(int fd) : fd_(fd) {}
    ~Listener() {
        close();
    }

    Listener(const Listener&) = delete;
    Listener& operator=(const Listener&) = delete;

    Listener(Listener&& other) : fd_(std::exchange(other.fd_, -1)) {}
    Listener& operator=(Listener&& other) {
        if(&other != this) {
            close();
            fd_ = std::exchange(other.fd_, -1);
        }

        return *this;
    }

    int fd() {
        return fd_;
    }

    bool valid() {
        return fd_ != -1;
    }

    Socket accept(struct sockaddr* skaddr, socklen_t* len) {
        int client_fd = ::accept(fd_, skaddr, len);

        if(client_fd == -1) {
            if(errno == EAGAIN || errno == EWOULDBLOCK) {
                return Socket{-1};
            }
            
            throw std::runtime_error(std::strerror(errno));
        }

        Socket client_sock(client_fd);
        client_sock.set_nonblocking();

        return client_sock;
    }

    void close() {
        if(fd_ != -1) {
            ::close(fd_);
            fd_ = -1;
        }
    }

private:
    void set_nonblocking() {
        int old_option = fcntl(fd_, F_GETFL, 0);
        int new_option = old_option | O_NONBLOCK;
        if(fcntl(fd_, F_SETFL, new_option) == -1) {
            throw std::runtime_error(std::strerror(errno));
        }
    }

private:
    int fd_;
};

#endif