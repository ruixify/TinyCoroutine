#ifndef IO_OPS_H
#define IO_OPS_H

#include <cerrno>
#include <cstddef>

#include "../runtime/event_loop.h"
#include "../corotine/task.h"
#include "socket.h"

inline Task async_write_all(Event_loop& loop, Socket& socket, const char* data, size_t size) {
    size_t offset = 0;

    while(offset < size) {
        co_await loop.write(socket.fd());

        ssize_t n = socket.write(data, strlen(data));

        if(n > 0) {
            offset += static_cast<ssize_t>(n);
            continue;
        }

        if(n == -1) {
            if(errno == EAGAIN || errno == EWOULDBLOCK) {
                continue;
            }

            break;
        }

        break;
    }

    co_return;
}

#endif