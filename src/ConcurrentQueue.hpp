#ifndef CONCURRENT_QUEUE_HPP
#define CONCURRENT_QUEUE_HPP

#include <atomic>
#include <condition_variable>
#include <mutex>
#include <optional>
#include <queue>

template <class T>
class ConcurrentQueue {
public:
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
        isDone_ = true;
        cv_.notify_all();
    }

private:
    std::queue<T> queue_;
    std::mutex m_;
    std::condition_variable cv_;
    std::atomic<bool> isDone_ = false;
};

#endif  // CONCURRENT_QUEUE_HPP