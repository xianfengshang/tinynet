#pragma once
#include <mutex>
#include <atomic>
namespace tinynet {
class RWLock {
  public:
    RWLock() :
        read_count_(0),
        write_count_(0),
        write_flag_(false) {

    }
    ~RWLock() = default;
  private:
    RWLock(const RWLock&) = delete;
    RWLock(RWLock&&) = default;
    RWLock& operator =(const RWLock&) = delete;
  public:
    void rlock() {
        std::unique_lock<std::mutex> lock(lock_);
        read_cond_.wait(lock, [this]() { return write_count_ == 0; });
        ++read_count_;
    }
    void runlock() {
        std::unique_lock<std::mutex> lock(lock_);
        if (--read_count_ && write_count_ > 0) {
            write_cond_.notify_one();
        }
    }

    void wlock() {
        std::unique_lock<std::mutex> lock(lock_);
        ++write_count_;
        write_cond_.wait(lock, [this]() { return read_count_ == 0 && !write_flag_; });
    }

    void wunlock() {
        std::unique_lock<std::mutex> lock(lock_);
        if (--write_count_ == 0) {
            read_cond_.notify_all();
        } else {
            write_cond_.notify_one();
        }
        write_flag_ = true;
    }
  private:
    std::mutex lock_;
    std::atomic_int read_count_;
    std::atomic_int write_count_;
    std::atomic_bool write_flag_;
    std::condition_variable read_cond_;
    std::condition_variable write_cond_;
};

template<typename RWLOCK>
class rlock_guard {
  public:
    rlock_guard(RWLock& lock):
        lock_(lock) {
        lock_.rlock();
    }
    ~rlock_guard() {
        lock_.wlock();
    }
  private:
    RWLock& lock_;
};

template<typename RWLOCK>
class wlock_guard {
  public:
    wlock_guard(RWLock& lock) :
        lock_(lock) {
        lock_.wlock();
    }
    ~wlock_guard() {
        lock_.wlock();
    }
  private:
    RWLock& lock_;
};
}
