// Copyright (C), Xianfeng Shang.  All rights reserved.
// Author: Xianfeng Shang (shangxianfeng@outlook.com)
#include "util/random_utils.h"
#include <random>
#include <limits>
#include <mutex>
#include "base/singleton.h"
namespace RandomUtils {

class RandomGenerator:
    public tinynet::Singleton<RandomGenerator> {
  public:
    RandomGenerator():
        lock_(),
        dev_(),
        engine_(dev_()) {
    }
    ~RandomGenerator() = default;
  public:
    template<typename T>
    T Random(T start, T end) {
        std::lock_guard<std::mutex> lock(lock_);
        std::uniform_int_distribution<T> dis(start, end);
        return dis(engine_);
    }
    template<typename T>
    T Random() {
        return Random<T>((T)0, (std::numeric_limits<T>::max)());
    }

    template<typename T>
    T RandomReal(T start, T end) {
        std::lock_guard<std::mutex> lock(lock_);
        std::uniform_real_distribution<T> dis(start, end);
        return dis(engine_);
    }
    template<typename T>
    T RandomReal() {
        return RandomReal<T>((T)0, (std::numeric_limits<T>::max)());
    }
  private:
    std::mutex lock_;
    std::random_device dev_;
    std::default_random_engine engine_;

};

uint8_t Random8() {
    return (uint8_t)RandomGenerator::Instance()->Random<uint16_t>(0, (std::numeric_limits<uint8_t>::max)());
}

uint8_t Random8(uint8_t start, uint8_t end) {
    return (uint8_t)RandomGenerator::Instance()->Random<uint16_t>(start, end);
}

uint16_t Random16() {
    return RandomGenerator::Instance()->Random<uint16_t>();
}

uint16_t Random16(uint16_t start, uint16_t end) {
    return RandomGenerator::Instance()->Random<uint16_t>(start, end);
}

uint32_t Random32() {
    return RandomGenerator::Instance()->Random<uint32_t>();
}

uint32_t Random32(uint32_t start, uint32_t end) {
    return RandomGenerator::Instance()->Random<uint32_t>(start, end);
}

uint64_t Random64() {
    return RandomGenerator::Instance()->Random<uint64_t>();
}

uint64_t Random64(uint64_t start, uint64_t end) {
    return RandomGenerator::Instance()->Random<uint64_t>(start, end);
}

float RandomFloat() {
    return RandomGenerator::Instance()->RandomReal<float>();
}

float RandomFloat(float start, float end) {
    return RandomGenerator::Instance()->RandomReal<float>(start, end);
}

double RandomDouble() {
    return RandomGenerator::Instance()->RandomReal<double>();
}

double RandomDouble(double start, double end) {
    return RandomGenerator::Instance()->RandomReal<double>(start, end);
}
}
