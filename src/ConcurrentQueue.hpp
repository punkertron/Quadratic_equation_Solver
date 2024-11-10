#pragma once

#include <condition_variable>
#include <mutex>
#include <optional>
#include <queue>

template <class T>
class ConcurrentQueue final {
public:

    ConcurrentQueue() = default;
    ~ConcurrentQueue() = default;

    ConcurrentQueue(const ConcurrentQueue&) = delete;
    ConcurrentQueue& operator=(const ConcurrentQueue&) = delete;
    ConcurrentQueue(ConcurrentQueue&&) = delete;
    ConcurrentQueue& operator=(ConcurrentQueue&&) = delete;

    void enqueue(const T& value)
    {
        std::lock_guard<std::mutex> lock(m_);
        queue_.push(value);
        cv_.notify_one();
    }

    std::optional<T> dequeue()
    {
        std::unique_lock<std::mutex> lock(m_);
        cv_.wait(lock, [&]() {
            return !queue_.empty() || isDone_;
        });

        if (queue_.empty()) {
            return std::nullopt;
        }
        std::optional<T> res = queue_.front();
        queue_.pop();
        return res;
    }

    void setDone()
    {
        std::lock_guard<std::mutex> lock(m_);
        isDone_ = true;
        cv_.notify_all();
    }

private:
    std::queue<T> queue_;
    std::mutex m_;
    std::condition_variable cv_;
    bool isDone_{false};
};
