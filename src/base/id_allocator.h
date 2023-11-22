/** Copyright 2010-2012 Twitter, Inc.*/
/**
* An object that generates IDs.
*/
#pragma once
#include "clock.h"
#include "runtime_logger.h"
#include <cstdint>
#include <thread>
#include <mutex>
namespace tinynet {
class IdAllocator {
  public:
    static const int64_t Twepoch = 1288834974657L;

    static const int workerId_Bits = 5;
    static const int datacenterId_Bits = 5;
    static const int SequenceBits = 12;
    static const int64_t MaxworkerId_ = 31;//-1L ^ (-1L << workerId_Bits);
    static const int64_t MaxdatacenterId_ = 32;//-1L ^ (-1L << datacenterId_Bits);

  private:
    static const int workerId_Shift = SequenceBits;
  private:
    static const int datacenterId_Shift = SequenceBits + workerId_Bits;
  public:
    static const int TimestampLeftShift = SequenceBits + workerId_Bits + datacenterId_Bits;
  private:
    static const int64_t SequenceMask = 4095;//-1L ^ (-1L << SequenceBits);


  public:
    IdAllocator(int64_t workerId, int64_t datacenterId, int64_t sequence = 0L) {
        workerId_ = workerId;
        datacenterId_ = datacenterId;
        sequence_ = sequence;
        lastTimestamp_ = 0;
    }
    static IdAllocator* Instance() {
        static IdAllocator worker(0, 0);
        return &worker;
    }
  public:
    uint64_t NextId() {
        std::lock_guard<std::mutex> lock(lock_);
        {
            auto timestamp = TimeGen();
            if (timestamp < lastTimestamp_) {
                auto diffTime = lastTimestamp_ - timestamp;
                log_runtime_error("Clock moved backwards.  Refusing to generate id for %lld milliseconds", diffTime);
                std::this_thread::sleep_for(std::chrono::milliseconds(++diffTime));
            }

            if (lastTimestamp_ == timestamp) {
                sequence_ = (sequence_ + 1) & SequenceMask;
                if (sequence_ == 0) {
                    timestamp = TilNextMillis(lastTimestamp_);
                }
            } else {
                sequence_ = 0;
            }

            lastTimestamp_ = timestamp;
            auto id = ((timestamp - Twepoch) << TimestampLeftShift) |
                      (datacenterId_ << datacenterId_Shift) |
                      (workerId_ << workerId_Shift) | sequence_;

            return id;
        }
    }

  protected:
    int64_t TilNextMillis(int64_t lastTimestamp) {
        auto timestamp = TimeGen();
        while (timestamp <= lastTimestamp) {
            std::this_thread::sleep_for(std::chrono::milliseconds(1));
            timestamp = TimeGen();
        }
        return timestamp;
    }

  protected:
    int64_t TimeGen() {
        return Time_ms();
    }
  private:
    int64_t		workerId_;
    int64_t		datacenterId_;
    int64_t		sequence_;
    int64_t     lastTimestamp_;
    std::mutex	lock_;
};
}
