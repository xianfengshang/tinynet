// Copyright (c) 2011 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "at_exit.h"
#include <stddef.h>
#include <ostream>
#include "runtime_logger.h"

namespace tinynet {

// Keep a stack of registered AtExitManagers.  We always operate on the most
// recent, and we should never have more than one outside of testing (for a
// statically linked version of this library).  Testing may use the shadow
// version of the constructor, and if we are building a dynamic library we may
// end up with multiple AtExitManagers on the same process.  We don't protect
// this for thread-safe access, since it will only be modified in testing.
static AtExitManager* g_top_manager = NULL;

AtExitManager::AtExitManager() : next_manager_(g_top_manager) {
    // If multiple modules instantiate AtExitManagers they'll end up living in this
    // module... they have to coexist.
#if !defined(COMPONENT_BUILD)
    TINYNET_ASSERT(!g_top_manager);
#endif
    g_top_manager = this;
}

AtExitManager::~AtExitManager() {
    if (!g_top_manager) {
        log_runtime_error("Tried to ~AtExitManager without an AtExitManager");
        return;
    }
    TINYNET_ASSERT(this == g_top_manager);

    ProcessCallbacksNow();
    g_top_manager = next_manager_;
}

// static
void AtExitManager::RegisterCallback(AtExitCallbackType func, void* param) {
    TINYNET_ASSERT(func);
    if (!g_top_manager) {
        log_runtime_error("Tried to RegisterCallback without an AtExitManager");
        return;
    }

    std::lock_guard<std::mutex> lock(g_top_manager->lock_);
    g_top_manager->stack_.push({ func, param });
}

// static
void AtExitManager::ProcessCallbacksNow() {
    if (!g_top_manager) {
        log_runtime_error("Tried to ProcessCallbacksNow without an AtExitManager");
        return;
    }

    std::lock_guard<std::mutex> lock(g_top_manager->lock_);

    while (!g_top_manager->stack_.empty()) {
        Callback task = g_top_manager->stack_.top();
        task.func(task.param);
        g_top_manager->stack_.pop();
    }
}

AtExitManager::AtExitManager(bool shadow) : next_manager_(g_top_manager) {
    TINYNET_ASSERT(shadow || !g_top_manager);
    g_top_manager = this;
}

}  // namespace tinynet
