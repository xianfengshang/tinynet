#pragma once
#include <atomic>
#include <thread>

namespace tinynet {
class SpinLock {
  public:
    SpinLock() = default;
    ~SpinLock() = default;
  private:
    SpinLock(const SpinLock&) = delete;
    SpinLock(SpinLock&&) = default;
    SpinLock& operator =(const SpinLock&) = delete;
  public:
    void lock() {
        while (lock_.test_and_set(std::memory_order_acquire))
            std::this_thread::yield();
    }
    void unlock() {
        lock_.clear(std::memory_order_release);
    }

    bool try_lock() {
        return lock_.test_and_set(std::memory_order_acquire);
    }
  private:
    std::atomic_flag lock_{ {0} };
};
}