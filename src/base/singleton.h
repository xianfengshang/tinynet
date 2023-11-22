// Copyright (C), Xianfeng Shang.  All rights reserved.
// Author: Xianfeng Shang (shangxianfeng@outlook.com)
#pragma once
#include <mutex>
#include <cstdlib>
#include <cassert>
#include "at_exit.h"
namespace tinynet {
template <typename T>
class Singleton {
  public:
    Singleton() = default;
    Singleton(const Singleton &) = delete;
    Singleton(Singleton&&) = delete;
    Singleton& operator=(const Singleton &) = delete;
  public:
    static T* Instance() {
        std::call_once(flag_, &Singleton::Init);
        assert(instance_ != nullptr);
        return instance_;
    }
  private:
    static void Init() {
        instance_ = new(std::nothrow) T();
        assert(instance_ != nullptr);
        AtExitManager::RegisterCallback(Destroy, nullptr);
    }
    static void Destroy(void *) {
        delete instance_;
        instance_ = nullptr;
    }
  private:
    static std::once_flag	flag_;
    static T				*instance_;
};
template<typename T >
std::once_flag Singleton<T>::flag_;

template<typename T >
T* Singleton<T>::instance_ = nullptr;
}
