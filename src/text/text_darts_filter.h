// Copyright (C), Xianfeng Shang.  All rights reserved.
// Author: Xianfeng Shang (shangxianfeng@outlook.com)
#pragma once
#include <string>
#include <vector>
#include <list>
#include <algorithm>
#include <set>
#include "text_filter_impl.h"

namespace tinynet {
namespace text {
class TextDartsFilter: public TextFilterImpl {
  public:
    static const int INITAL_SIZE = 65535;
    struct DictNode {
        int code{ 0 };
        int depth{ 0 };
        int left{ 0 };
        int right{ 0 };
    };
    struct TreeNode {
        int base{ 0 };
        int check{ 0 };
    };
    struct SearchResult {
        size_t pos{ 0 };
        size_t len{ 0 };
    };
  public:
    TextDartsFilter();
    virtual ~TextDartsFilter();
  public:
    void resize(int newSize);
    void Init(const std::vector<std::wstring>& dict) override;
    bool Contains(const std::wstring& key) override;
    bool ContainsKey(const std::wstring& key) override;
    virtual size_t Replace(const std::wstring& text, const std::wstring& rep, std::wstring* output);
  private:
    int insert(const std::vector<std::wstring>& keys, const std::vector<DictNode>& nodes);
    int find_children(const std::vector<std::wstring>& dict, const DictNode& parent, std::vector<DictNode>* children);
    int FullTextSearch(const std::wstring& text, std::vector<SearchResult>* results);
  private:
    std::vector<TreeNode> tree_;
    std::vector<char> used_;
    int next_pos_;
};
}
}
