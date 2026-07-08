#include <iostream>
#include <cerrno>
#include <stdexcept>
#include <cstring>

#include <arpa/inet.h>
#include <netinet/in.h>
#include <string.h>
#include <stdlib.h>
#include <unistd.h>
#include <assert.h>
#include <fcntl.h>

#include "../include/corotine/task.h"
#include "../include/runtime/event_loop.h"


void set_nonblocking(int fd) {
    int old_status = fcntl(fd, F_GETFL, 0);
    int new_status = old_status | O_NONBLOCK;

    if(fcntl(fd, F_SETFL, new_status) == -1) {
        throw std::runtime_error(std::strerror(errno));
    }
}

int create_listen_socket(int port) {
    int listen_fd = socket(PF_INET, SOCK_STREAM, 0);
    if(listen_fd == -1) {
        throw std::runtime_error(std::strerror(errno));
    }

    int op = 1;
    if(setsockopt(listen_fd, SOL_SOCKET, SO_REUSEADDR, &op, sizeof(op)) == -1) {
        close(listen_fd);
        throw std::runtime_error(std::strerror(errno));
    }

    struct sockaddr_in skaddr;
    skaddr.sin_family = AF_INET;
    skaddr.sin_addr.s_addr = htonl(INADDR_ANY);
    skaddr.sin_port = htons(port);
    if(bind(listen_fd, (struct sockaddr*)(&skaddr), sizeof(skaddr)) == -1) {
        close(listen_fd);
        throw std::runtime_error(std::strerror(errno));
    }

    if(listen(listen_fd, 8) == -1) {
        close(listen_fd);
        throw std::runtime_error(std::strerror(errno));
    }

    set_nonblocking(listen_fd);
    return listen_fd;
}


Task handle_client(Event_loop& loop, int client_fd) {
    char buf[1024];

    while(true) {
        co_await loop.read(client_fd);

        ssize_t n = read(client_fd, buf, sizeof(buf));

        if(n == -1) {
            if(errno == EAGAIN || errno == EWOULDBLOCK) {
                continue;
            }

            std::cout << "read error" << std::endl;
            break;
        } else if(n > 0) {
            printf("%s\n", buf);
        } else {
            printf("client closed\n");
        }

        co_await loop.write(client_fd);
        char* data = "i am server.";
        ssize_t written = write(client_fd, data, strlen(data));

        if(written == -1) {
            if(errno == EAGAIN || errno == EWOULDBLOCK) {
                continue;
            }

            std::cout << "write erro" << std::endl;
            break;
        }
    }

    close(client_fd);
    co_return;
}

Task accept_loop(Event_loop& loop, Scheduler& sc, int listen_fd) {
    while(true) {
        co_await loop.read(listen_fd);

        while(true) {
            struct sockaddr_in clientaddr;
            socklen_t client_len = sizeof(clientaddr);
            int client_fd = accept(listen_fd, (struct sockaddr*)(&clientaddr), &client_len);

            if(client_fd == -1) {
                if(errno == EAGAIN || errno == EWOULDBLOCK) {
                    break;
                }

                std::cout << "accept erro" << std::endl;
                break;
            }

            int port = ntohs(clientaddr.sin_port);
            set_nonblocking(client_fd);

            std::cout << "client port: " << port << std::endl;

            sc.spawn(handle_client(loop, client_fd));
        }
    }

    co_return;
}

int main() {
    Scheduler sc;
    Event_loop loop(sc);

    int listen_fd = create_listen_socket(1234);

    sc.spawn(accept_loop(loop, sc, listen_fd));

    std::cout << "server listening on 127.0.0.1:1234" << std::endl;

    loop.run();

    close(listen_fd);
}