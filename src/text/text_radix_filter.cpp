// Copyright (C), Xianfeng Shang.  All rights reserved.
// Author: Xianfeng Shang (shangxianfeng@outlook.com)
#include "text_radix_filter.h"
#include <vector>
#include <stack>
#include <algorithm>

namespace tinynet {
namespace text {
RadixNode::RadixNode():
    ending(false) {
}

RadixNode::RadixNode(const std::wstring& str):
    value(str),
    ending(false) {
}

RadixNode* RadixNode::find(int code) {
    auto it = std::lower_bound(children.begin(), children.end(), code);
    if ((it == children.end()) || (code != (*it)->code())) {
        return nullptr;
    }
    return it->get();
}

RadixNode* RadixNode::insert(const std::wstring& str) {
    int code = str[0];
    auto it = std::upper_bound(children.begin(), children.end(), code);
    auto node = new RadixNode(str);
    children.emplace(it, node);
    return node;
}

void RadixNode::erase(int code) {
    auto it = std::lower_bound(children.begin(), children.end(), code);
    if ((it == children.end()) || (code != (*it)->code())) {
        return;
    }
    children.erase(it);
}

TextRadixFilter::TextRadixFilter() = default;

TextRadixFilter::~TextRadixFilter() = default;

void TextRadixFilter::Init(const std::vector<std::wstring>& keys) {
    for (auto& word : keys) {
        Add(word);
    }
}

static size_t CommomPrefixLength(const std::wstring& left, const std::wstring& right) {
    size_t n = (std::min)(left.length(), right.length());
    if (n == 0 || left[0] != right[0]) return 0;
    size_t i = 1;
    for (; i < n; ++i) {
        if (left[i] != right[i])
            break;
    }
    return i;
}

void TextRadixFilter::AddToNode(RadixNode* node, const std::wstring& str) {
    size_t len = CommomPrefixLength(node->value, str);
    if (len == 0)
        return;

    if (len == node->value.size() && len == str.size()) {
        node->ending = true;
        return;
    }
    if (len < node->value.size()) {
        auto sub_str = node->value.substr(len);
        node->value.erase(len);

        std::vector<RadixNodePtr> children;
        node->children.swap(children);
        auto child = node->insert(sub_str);
        child->children.swap(children);
        child->ending = node->ending;

        node->ending = false;
    }
    if (len < str.size()) {
        auto sub_str = str.substr(len);
        AddToChild(node, sub_str);
    }
}

void TextRadixFilter::AddToChild(RadixNode* node, const std::wstring& str) {
    auto child = node->find(str[0]);
    if (child) {
        AddToNode(child, str);
    } else {
        child = node->insert(str);
        child->ending = true;
    }
}

bool TextRadixFilter::Add(const std::wstring& key) {
    if (key.empty())
        return false;

    AddToChild(&root_, key);
    return true;
}

bool TextRadixFilter::Remove(const std::wstring& key) {
    if (key.empty()) return false;
    RadixNode* base = &root_;
    size_t len = key.size();
    size_t i, n;
    std::stack<RadixNode*> stk;
    for (i = 0; i < len;) {
        base = base->find(key[i]);
        if (base) {
            n = (int)base->value.size();
            if (n == 1 || (n > 1 && (len - i) >= n && key.compare(i, n, base->value) == 0)) {
                i += n;
                stk.push(base);
            } else
                return false;
        } else
            return false;
    }
    if (stk.empty() || !stk.top()->ending)
        return false;
    RadixNode* child = stk.top();
    child->ending = false;
    stk.pop();

    while (stk.size() > 0) {
        RadixNode* parent = stk.top();
        if (child->children.empty()) {
            parent->erase(child->code());
        }
        child = parent;
        stk.pop();
    }
    return true;
}

bool TextRadixFilter::ContainsKey(const std::wstring& key) {
    if (key.empty()) return false;
    RadixNode* base = &root_;
    size_t len = key.size();
    size_t i, n;
    RadixNode* leaf = nullptr;
    for (i = 0; i < len;) {
        base = base->find(key[i]);
        if (base) {
            n = (int)base->value.size();
            if (n == 1 || (n > 1 && (len - i) >= n && key.compare(i, n, base->value) == 0)) {
                i += n;
                leaf = base;
            } else
                return false;
        } else
            return false;
    }
    return  leaf && leaf->ending;
}

bool TextRadixFilter::Contains(const std::wstring& text) {
    std::vector<SearchResult> results;
    return FullTextSearch(text, &results) > 0;
}

size_t TextRadixFilter::Replace(const std::wstring& text, const std::wstring& rep, std::wstring* output) {
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

size_t TextRadixFilter::FullTextSearch(const std::wstring& text, std::vector<SearchResult>* results) {
    int i = 0;
    int len = (int)text.size();
    size_t nmatch;
    RadixNode* base;
    int step, n;
    while (i < len) {
        step = 1;
        std::stack<RadixNode*> stk;
        base = &root_;
        nmatch = 0;
        for (int j = i; j < len;) {
            base = base->find(text[j]);
            if (base) {
                n = (int)base->value.size();
                if (n == 1 || (n > 1 && (len - j) >= n && text.compare(j, n, base->value) == 0)) {
                    j += n;
                    stk.push(base);
                    nmatch += n;
                } else
                    break;
            } else
                break;
        }
        while (stk.size() > 0) {
            RadixNode* top = stk.top();
            if (top->ending)
                break;
            stk.pop();
            nmatch -= top->value.size();
        }
        if (stk.size() > 0) {
            SearchResult result;
            result.pos = i;
            result.len = nmatch;
            step = (int)result.len;
            results->emplace_back(std::move(result));
        }
        i += step;
    }
    return results->size();
}
}
}