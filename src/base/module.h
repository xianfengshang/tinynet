// Copyright (C), Xianfeng Shang.  All rights reserved.
// Author: Xianfeng Shang (shangxianfeng@outlook.com)
#pragma once
#include <string>
#include <typeinfo>
#include <cstdint>
#include "application.h"
#include "singleton.h"
namespace tinynet {
class ModuleBase {
  public:
    const char* get_name() const { return name_; }

    void set_name(const char* name) {
        strncpy(name_, name, sizeof(name_) - 1);
        name_[sizeof(name_) -1] = 0;
    }
  private:
    char name_[128] { 0 };
};

template<class T>
class Module :
    public ModuleBase,
    public Singleton<T> {
  protected:
    char Init() { return 0; }

    char Start() { return 0; }

    char Stop() { return 0; }

    char Run() { return 0; }

    char Signal(int signum) { return 0; }

    int Priority() const { return 0; }
  public:
    static bool Register(ModuleBase* module);
};
}
namespace tinynet {
template <class T>
bool Module<T>::Register(ModuleBase* module) {
    T* self = static_cast<T*>(module);
    self->set_name(typeid(T).name());
    ModuleItem item = {
        self,
        self->Priority()
    };
    if (sizeof(self->Init()) == sizeof(int)) {
        item.Init = std::bind(&T::Init, self);
    }
    if (sizeof(self->Start()) == sizeof(int)) {
        item.Start = std::bind(&T::Start, self);
    }
    if (sizeof(self->Stop()) == sizeof(int)) {
        item.Stop = std::bind(&T::Stop, self);
    }
    if (sizeof(self->Run()) == sizeof(int)) {
        item.Run = std::bind(&T::Run, self);
    }
    if (sizeof(self->Signal(0)) == sizeof(int)) {
        item.Signal = std::bind(&T::Signal, self, std::placeholders::_1);
    }
    return g_App->RegisterModule(item);
}

}

#define MODULE_IMPL(m) \
struct m##Register\
{\
	m##Register()\
	{\
		tinynet::GetModuleEntries().emplace_back([](){\
			m::Register(m::Instance()); \
		});\
	}\
} g_##m##Register;
