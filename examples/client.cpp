#include <iostream>
#include <cerrno>
#include <cstring>
#include <fcntl.h>

#include <arpa/inet.h>
#include <netinet/in.h>
#include <unistd.h>
#include <string.h>
#include <stdlib.h>
#include <stdexcept>

#include "../include/corotine/task.h"
#include "../include/runtime/event_loop.h"


void set_nonblocking(int fd) {
    int old_status =  fcntl(fd, F_GETFL, 0);
    int new_status = old_status | O_NONBLOCK;

    if(fcntl(fd, F_SETFL, new_status) == -1) {
        throw std::runtime_error(std::strerror(errno));
    }
}


int create_socket(const char* ip, int port) {
    int fd = socket(PF_INET, SOCK_STREAM, 0);
    if(fd == -1) {
        throw std::runtime_error(std::strerror(errno));
    }

    struct sockaddr_in clienmtaddr;
    clienmtaddr.sin_family = AF_INET;
    inet_pton(AF_INET, ip, &clienmtaddr.sin_addr.s_addr);
    clienmtaddr.sin_port = htons(port);

    if(connect(fd, (struct sockaddr*)(&clienmtaddr), sizeof(clienmtaddr)) == -1) {
        close(fd);
        throw std::runtime_error(std::strerror(errno));
    }

    set_nonblocking(fd);

    return fd;
}

Task handle_server(Event_loop& loop, int fd) {
    char buf[1024];

    while(true) {
        co_await loop.write(fd);

        char* data = "i am client.";
        ssize_t written = write(fd, data, strlen(data));

        if(written == -1) {
            if(errno == EAGAIN || errno == EWOULDBLOCK) {
                continue;
            }
            printf("write error\n");
            break;
        }

        sleep(1);
        co_await loop.read(fd);

        ssize_t n = read(fd, buf, sizeof(buf));

        if(n == -1) {
            if(errno == EAGAIN || errno == EWOULDBLOCK) {
                continue;
            }
            printf("read error\n");
            break;
        } else if(n > 0){
            printf("%s\n", buf);
        } else {
            printf("server closed\n");
        }
    }
}

int main() {
    Scheduler sc;
    Event_loop loop(sc);

    int fd = create_socket("127.0.0.1", 1234);

    sc.spawn(handle_server(loop, fd));

    loop.run();

    close(fd);
}