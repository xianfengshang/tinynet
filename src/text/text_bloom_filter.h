// Copyright (C), Xianfeng Shang.  All rights reserved.
// Author: Xianfeng Shang (shangxianfeng@outlook.com)
#pragma once
#include <memory>
#include <string>
#include <vector>
#include <algorithm>
#include <map>
#include "bloom_filter.h"
#include "text_filter_impl.h"

namespace tinynet {
namespace text {
class TextBloomFilter : public TextFilterImpl {
  public:
    TextBloomFilter();
    virtual ~TextBloomFilter();
  public:
    void Init(const std::vector<std::wstring>& dict) override;
    bool ContainsKey(const std::wstring& key) override;
    bool Contains(const std::wstring& text) override;
    size_t Replace(const std::wstring& text, const std::wstring& rep, std::wstring* output) override;
  private:
    struct SearchResult {
        size_t pos{ 0 };
        size_t len{ 0 };
    };
    size_t FullTextSearch(const std::wstring& text, std::vector<SearchResult>* results);
  private:
    std::shared_ptr<BloomFilter> bloom_filter_;
};

}
}
