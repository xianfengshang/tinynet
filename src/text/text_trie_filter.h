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

struct TrieNode;
typedef std::unique_ptr<TrieNode> TrieNodePtr;

struct TrieNode {
    TrieNode();
    TrieNode(int code);
    TrieNode* find(int code);
    TrieNode* insert(int code);
    void erase(int code);

    bool ending() { return value < 0; }
    void set_ending(bool ending) { value = ending ? -std::abs(value): std::abs(value); }
    int code() const { return std::abs(value); }

    int value{ 0 };
    std::vector<TrieNodePtr> children;
    TrieNode* parent{ nullptr };
};


class TextTrieFilter: public TextFilterImpl {
  public:
    TextTrieFilter();
    virtual ~TextTrieFilter();
  public:
    void Init(const std::vector<std::wstring>& keys) override;
    bool ContainsKey(const std::wstring& key) override;
    bool Contains(const std::wstring& text) override;
    size_t Replace(const std::wstring& text, const std::wstring& rep, std::wstring* output) override;

    bool Add(const std::wstring& key);
    bool Remove(const std::wstring& key);
  private:
    struct SearchResult {
        size_t pos{ 0 };
        size_t len{ 0 };
    };
    size_t FullTextSearch(const std::wstring& text, std::vector<SearchResult>* results);
  private:
    TrieNode root_;
};

inline bool operator < (const TrieNodePtr& a, const TrieNodePtr& b) {
    return a->code() < b->code();
}

inline bool operator < (const TrieNodePtr& a, int b) {
    return a->code() < b;
}

inline bool operator < (int a, const TrieNodePtr& b) {
    return a < b->code();
}

}
}
