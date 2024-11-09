#ifndef CONSOLE_OUTPUT_HPP
#define CONSOLE_OUTPUT_HPP

#include <iostream>
#include <mutex>
#include <utility>

class ConsoleOutput {
public:
    ConsoleOutput() = default;
    ~ConsoleOutput() = default;

    ConsoleOutput(const ConsoleOutput&) = delete;
    ConsoleOutput& operator=(const ConsoleOutput&) = delete;
    ConsoleOutput(ConsoleOutput&&) = delete;
    ConsoleOutput& operator=(ConsoleOutput&&) = delete;

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