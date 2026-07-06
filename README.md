# TinyCoroutine

## 缘由
传统的“一连接一线程”模型在线程数量较多时会产生较大的线程栈、上下文切换和同步开销。纯回调式异步 I/O虽然性能较好，但控制流容易割裂，代码维护难度较高。

C++20 协程可以使用接近同步代码的写法表达异步操作。同时，单进程单线程无法充分利用多核 CPU ，因此醒目引入多进程。如此也可以利用多喝，且避免复杂的跨线程协程调度和共享状态同步。

## 实现目标
目前处于第一阶段，该阶段的目标是实现一个能够稳定处理简单HTTP请求的服务器。

## 项目目录
```text
TinyCoroutine/
├── CMakeLists.txt
├── README.md
├── include/
│   ├── common/
│   │   ├── error.h
│   │   ├── log.h
│   │   └── config.h
│   ├── coroutine/
│   │   └── task.h
│   ├── runtime/
│   │   ├── scheduler.h
│   │   ├── event_loop.h
│   │   └── io_awaitable.h
│   ├── process/
│   │   ├── master.h
│   │   ├── worker.h
│   │   └── signal_handler.h
│   ├── net/
│   │   ├── socket.h
│   │   ├── acceptor.h
│   │   ├── connection.h
│   │   └── address.h
│   └── http/
│       ├── request.h
│       ├── response.h
│       ├── parser.h
│       └── server.h
├── src/
│   ├── common/
│   ├── runtime/
│   ├── process/
│   ├── net/
│   └── http/
├── examples/
│   └── hello_server.cpp
├── tests/
└── benchmark/
```