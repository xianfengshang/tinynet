// Copyright (C), Xianfeng Shang.  All rights reserved.
// Author: Xianfeng Shang (shangxianfeng@outlook.com)
#pragma once
#include <memory>
#include <string>
#include <vector>
#include <algorithm>
#include "text_filter_impl.h"

namespace tinynet {
namespace text {

class TextFilterImpl {
  public:
    TextFilterImpl() {};
    virtual ~TextFilterImpl() {}
  public:
    virtual void Init(const std::vector<std::wstring>& dict) = 0;
    virtual bool ContainsKey(const std::wstring& key) = 0;
    virtual bool Contains(const std::wstring& text) = 0;
    virtual size_t Replace(const std::wstring& text, const std::wstring& rep, std::wstring* output) = 0;
};

}
}
