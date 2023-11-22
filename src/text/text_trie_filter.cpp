// Copyright (C), Xianfeng Shang.  All rights reserved.
// Author: Xianfeng Shang (shangxianfeng@outlook.com)
#include "text_trie_filter.h"
#include <vector>
#include <stack>
#include <algorithm>

namespace tinynet {
namespace text {
TrieNode::TrieNode() = default;

TrieNode::TrieNode(int code):
    value(code) {

}

TrieNode* TrieNode::find(int code) {
    auto it = std::lower_bound(children.begin(), children.end(), code);
    if ((it == children.end()) || (code < (*it)->code())) {
        return nullptr;
    }
    return it->get();
}

TrieNode* TrieNode::insert(int code) {
    auto it = std::upper_bound(children.begin(), children.end(), code);
    auto node = new TrieNode(code);
    node->parent = this;
    children.emplace(it, node);
    return node;
}

void TrieNode::erase(int code) {
    auto it = std::lower_bound(children.begin(), children.end(), code);
    if ((it == children.end()) || (code < (*it)->code())) {
        return;
    }
    children.erase(it);
}

TextTrieFilter::TextTrieFilter() = default;

TextTrieFilter::~TextTrieFilter() = default;

void TextTrieFilter::Init(const std::vector<std::wstring>& keys) {
    for (auto& word : keys) {
        Add(word);
    }
}

bool TextTrieFilter::Add(const std::wstring& key) {
    int len = (int)key.length();
    TrieNode* tail = &root_;
    for (int i = 0; i < len; ++i) {
        auto p = tail->find(key[i]);
        if (p == nullptr) {
            p = tail->insert(key[i]);
            p->set_ending(false);
        }
        tail = p;
    }
    if (!tail->ending()) {
        tail->set_ending(true);
    }
    return true;
}

bool TextTrieFilter::Remove(const std::wstring& key) {
    if (key.empty()) return false;
    int len = (int)key.length();
    TrieNode* tail = &root_;
    for (int i = 0; i < len; ++i) {
        auto p = tail->find(key[i]);
        if (p == nullptr) {
            return false;
        }
        tail = p;
    }
    if (!tail->ending()) {
        return false;
    }
    tail->set_ending(false);

    TrieNode* parent = tail->parent;
    while (parent && tail->children.empty()) {
        parent->erase(tail->code());
        tail = parent;
        parent = tail->parent;
    }
    return true;
}

bool TextTrieFilter::ContainsKey(const std::wstring& key) {
    if (key.empty()) return false;
    int len = (int)key.length();
    TrieNode* tail = &root_;
    for (int i = 0; i < len; ++i) {
        auto p = tail->find(key[i]);
        if (p == nullptr) {
            return false;
        }
        tail = p;
    }
    return tail->ending();
}

bool TextTrieFilter::Contains(const std::wstring& text) {
    std::vector<SearchResult> results;
    return FullTextSearch(text, &results) > 0;
}

size_t TextTrieFilter::Replace(const std::wstring& text, const std::wstring& rep, std::wstring* output) {
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

size_t TextTrieFilter::FullTextSearch(const std::wstring& text, std::vector<SearchResult>* results) {
    int i = 0;
    int len = (int)text.length();
    while (i < len) {
        int step = 1;
        std::stack<TrieNode*> stk;
        TrieNode* base = &root_;
        for (int j = i; j < len; ++j) {
            base = base->find(text[j]);
            if (base)
                stk.push(base);
            else
                break;
        }
        while (stk.size() > 0) {
            TrieNode* top = stk.top();
            if (top->ending())
                break;
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
    return results->size();
}
}
}