// Copyright (c) 2011 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef TINYNET_AT_EXIT_H
#define TINYNET_AT_EXIT_H

#pragma once

#include <stack>
#include <mutex>
#include "base.h"

namespace tinynet {

// This class provides a facility similar to the CRT atexit(), except that
// we control when the callbacks are executed. Under Windows for a DLL they
// happen at a really bad time and under the loader lock. This facility is
// mostly used by butil::Singleton.
//
// The usage is simple. Early in the main() or WinMain() scope create an
// AtExitManager object on the stack:
// int main(...) {
//    butil::AtExitManager exit_manager;
//
// }
// When the exit_manager object goes out of scope, all the registered
// callbacks and singleton destructors will be called.

class AtExitManager {
  public:
    typedef void (*AtExitCallbackType)(void*);

    AtExitManager();

    // The dtor calls all the registered callbacks. Do not try to register more
    // callbacks after this point.
    ~AtExitManager();

    // Registers the specified function to be called at exit. The prototype of
    // the callback function is void func(void*).
    static void RegisterCallback(AtExitCallbackType func, void* param);

    // Calls the functions registered with RegisterCallback in LIFO order. It
    // is possible to register new callbacks after calling this function.
    static void ProcessCallbacksNow();

  protected:
    // This constructor will allow this instance of AtExitManager to be created
    // even if one already exists.  This should only be used for testing!
    // AtExitManagers are kept on a global stack, and it will be removed during
    // destruction.  This allows you to shadow another AtExitManager.
    explicit AtExitManager(bool shadow);

  private:
    struct Callback {
        AtExitCallbackType func;
        void* param;
    };
    std::mutex lock_;
    std::stack<Callback> stack_;
    AtExitManager* next_manager_;  // Stack of managers to allow shadowing.

    AtExitManager(const AtExitManager&) = delete;
    AtExitManager& operator=(const AtExitManager&) = delete;
};

#if defined(UNIT_TEST)
class ShadowingAtExitManager : public AtExitManager {
  public:
    ShadowingAtExitManager() : AtExitManager(true) {}
};
#endif  // defined(UNIT_TEST)

}  // namespace tinynet

#endif  // TINYNET_AT_EXIT_H
