#include <iostream>
#include "../include/task.h"


Task simple_corotine() {
    std::cout << "Coroutine started" << std::endl;

    co_await std::suspend_always{};

    std::cout << "Coroutine finished" << std::endl;

    co_return;
}


int main() {

    Task tk = simple_corotine();

    std::cout << "Main started" << std::endl;

    tk.resume();

    std::cout << "Back to main" << std::endl;

    tk.resume();

    std::cout << "Coroutine done" << std::endl;
}