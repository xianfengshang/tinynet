// Copyright (C), Xianfeng Shang.  All rights reserved.
// Author: Xianfeng Shang (shangxianfeng@outlook.com)
#pragma once
#include <memory>
#include <string>
#include <vector>
#include <algorithm>
#include <map>
#include "text_filter_impl.h"

namespace tinynet {
namespace text {
enum FilterType {
    TRIE_FILTER,
    BLOOM_FILTER,
    DARTS_FILTER,
    RADIX_FILTER
};

class TextFilter {
  public:

  public:
    TextFilter(int type);
    ~TextFilter();
  public:
    void Init(const std::vector<std::string>& dict);
    bool ContainsKey(const std::string& key);
    bool Contains(const std::string& text);
    size_t Replace(const std::string& text, const std::string& rep, std::string* output);
  private:
    std::shared_ptr<TextFilterImpl> filter_impl_;
};

}
}
