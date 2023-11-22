// Copyright (C), Xianfeng Shang.  All rights reserved.
// Author: Xianfeng Shang (shangxianfeng@outlook.com)
#include "text_darts_filter.h"
#include "util/string_utils.h"
#include <vector>
#include <stack>
#include <algorithm>
#include <iostream>

namespace tinynet {
namespace text {
TextDartsFilter::TextDartsFilter():
    next_pos_(0) {
}

TextDartsFilter::~TextDartsFilter() = default;

void TextDartsFilter::resize(int newSize) {
    tree_.resize(newSize);
    used_.resize(newSize);
}

void TextDartsFilter::Init(const std::vector<std::wstring>& keys) {
    resize(INITAL_SIZE);
    tree_[0].base = 1;
    next_pos_ = 0;

    DictNode root;
    root.code = 0;
    root.depth = 0;
    root.left = 0;
    root.right = (int)keys.size();
    std::vector<DictNode> nodes;
    find_children(keys, root, &nodes);
    insert(keys, nodes);
}

int TextDartsFilter::insert(const std::vector<std::wstring>& keys, const std::vector<DictNode>& nodes) {
    int begin;
    int pos = (std::max)(nodes[0].code + 1, next_pos_) -1;
    bool flag = false;
    int max_pos = 0;
    while (1) {
repeat:
        ++pos;
        if ((size_t)pos >= tree_.size())
            resize(pos + 1);
        if (tree_[pos].check)
            continue;
        if (!flag) {
            next_pos_ = pos;
            flag = true;
        }
        begin = pos - nodes[0].code;

        max_pos = begin + nodes[nodes.size() - 1].code;
        if ((size_t)max_pos >= tree_.size()) {
            resize(max_pos + 1);
        }
        if (used_[begin]) continue;

        for (size_t i = 0; i < nodes.size(); ++i) {
            if (tree_[begin + nodes[i].code].check)
                goto repeat;
        }
        break;
    }
    used_[begin] = 1;
    for (size_t i = 0; i < nodes.size(); ++i)
        tree_[begin + nodes[i].code].check = begin;

    for (size_t i = 0; i < nodes.size(); ++i) {
        std::vector<DictNode> children;
        if (find_children(keys, nodes[i], &children) == 0) {
            tree_[begin + nodes[i].code].base = - nodes[i].left - 1;
        } else {
            int h = insert(keys, children);
            tree_[begin + nodes[i].code].base = h;
        }
    }
    return begin;
}

int TextDartsFilter::find_children(const std::vector<std::wstring>& dict, const DictNode& parent, std::vector<DictNode>* children) {
    int code, last;
    last = 0;
    for (int i = parent.left; i < parent.right; ++i) {
        if (dict[i].length() < (size_t)parent.depth)
            continue;
        code = 0;
        if (dict[i].length() != (size_t)parent.depth)
            code = dict[i][parent.depth] + 1;
        if (last > code) //The words in the dict must be in dictionary order.
            return 0;
        if (code != last || children->empty()) {
            DictNode node;
            node.code = code;
            node.depth = parent.depth + 1;
            node.left = i;
            if (!children->empty())
                (*children)[children->size() - 1].right = i;
            children->emplace_back(std::move(node));
        }
        last = code;
    }
    if (children->size() > 0)
        (*children)[children->size() - 1].right = parent.right;
    return (int)children->size();
}

bool TextDartsFilter::ContainsKey(const std::wstring& key) {
    int base = tree_[0].base;
    int pos = 0;
    int len = (int)key.length();
    std::stack<int> stk;
    for (int j = 0; j < len; ++j) {
        pos = base + (int)key[j] + 1;
        if (base != tree_[pos].check) {
            break;
        }
        base = tree_[pos].base;
        stk.push(base);
    }
    while (stk.size() > 0) {
        base = stk.top();
        pos = base;
        if (base == tree_[pos].check && tree_[pos].base < 0) {
            break;
        }
        stk.pop();
    }
    return !stk.empty();
}

bool TextDartsFilter::Contains(const std::wstring& text) {
    std::vector<SearchResult> results;
    return FullTextSearch(text, &results) > 0;
}

int TextDartsFilter::FullTextSearch(const std::wstring& text, std::vector<SearchResult>* results) {
    int i, len;
    i = 0;
    len = (int)text.length();
    while (i < len) {
        int step = 1;
        int base = tree_[0].base;
        int pos = 0;
        std::stack<int> stk;
        for (int j = i; j < len; ++j) {
            pos = base + (int)text[j] + 1;
            if (base != tree_[pos].check) {
                break;
            }
            base = tree_[pos].base;
            stk.push(base);
        }
        while (stk.size() > 0) {
            base = stk.top();
            pos = base;
            if (base == tree_[pos].check && tree_[pos].base < 0) {
                break;
            }
            stk.pop();
        }
        if (stk.size() > 0) {
            SearchResult result;
            result.pos = i;
            result.len = (int)stk.size();
            step = (int)result.len;
            results->emplace_back(std::move(result));
        }

        i += step;
    }
    return (int)results->size();
}

size_t TextDartsFilter::Replace(const std::wstring& text, const std::wstring& rep, std::wstring* output) {
    std::vector<SearchResult> results;
    if (FullTextSearch(text, &results) == 0) {
        output->assign(text);
        return 0;
    }
    size_t start = 0;
    for (size_t i = 0; i < results.size(); ++i) {
        if (start < results[i].pos) {
            output->append(text.substr(start, results[i].pos - start));
        }
        output->append(rep);
        start = results[i].pos + results[i].len;
    }
    if (start < text.size()) {
        output->append(text.substr(start));
    }
    return (int)results.size();
}

}
}
