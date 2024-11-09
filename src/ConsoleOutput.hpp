#ifndef CONSOLE_OUTPUT_HPP
#define CONSOLE_OUTPUT_HPP

#include <iostream>
#include <mutex>
#include <sstream>
#include <utility>

class ConsoleOutput {
public:
    ConsoleOutput()
    {
        // std::cin.tie(nullptr);
        // std::cout.tie(nullptr);
        // std::cin.sync_with_stdio(false);
        // std::cout.sync_with_stdio(false);
    }

    template <typename... Args>
    void print(Args&&... args)
    {
        std::lock_guard<std::mutex> lock(m_);
        (std::cout << ... << std::forward<Args>(args));
    }

private:
    std::mutex m_;
};

#endif  // CONSOLE_OUTPUT_HPP