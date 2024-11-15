#pragma once

#include <iostream>
#include <mutex>
#include <utility>

// we can create base class Output and inherit from it and override print function,
// but keep without inheritance for simplicity

class ConsoleOutput final {
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
