// Copyright (C), Xianfeng Shang.  All rights reserved.
// Author: Xianfeng Shang (shangxianfeng@outlook.com)
#pragma once
#include <string>
#include <vector>
#include <algorithm>
#include <cstdlib>
#include "text_filter_impl.h"

namespace tinynet {
namespace text {

struct RadixNode;
typedef std::unique_ptr<RadixNode> RadixNodePtr;

struct RadixNode {
    RadixNode();
    RadixNode(const std::wstring& str);
    RadixNode* find(int code);
    RadixNode* insert(const std::wstring& str);
    void erase(int code);

    int code() const { return value[0]; }

    std::wstring value;
    std::vector<RadixNodePtr> children;
    bool ending;
};


class TextRadixFilter: public TextFilterImpl {
  public:
    TextRadixFilter();
    virtual ~TextRadixFilter();
  public:
    void Init(const std::vector<std::wstring>& keys) override;
    bool ContainsKey(const std::wstring& key) override;
    bool Contains(const std::wstring& text) override;
    size_t Replace(const std::wstring& text, const std::wstring& rep, std::wstring* output) override;

    bool Add(const std::wstring& key);
    bool Remove(const std::wstring& key);
  private:
    void AddToNode(RadixNode* node, const std::wstring& str);
    void AddToChild(RadixNode* node, const std::wstring& str);
  private:
    struct SearchResult {
        size_t pos{ 0 };
        size_t len{ 0 };
    };
    size_t FullTextSearch(const std::wstring& text, std::vector<SearchResult>* results);
  private:
    RadixNode root_;
};

inline bool operator < (const RadixNodePtr& a, const RadixNodePtr& b) {
    return a->code() < b->code();
}

inline bool operator < (const RadixNodePtr& a, int b) {
    return a->code() < b;
}

inline bool operator < (int a, const RadixNodePtr& b) {
    return a < b->code();
}

}
}
