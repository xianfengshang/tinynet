// Copyright (C), Xianfeng Shang.  All rights reserved.
// Author: Xianfeng Shang (shangxianfeng@outlook.com)
#include "text_bloom_filter.h"
#include <vector>
#include <stack>
#include <algorithm>
#include "util/string_utils.h"

namespace tinynet {
namespace text {

TextBloomFilter::TextBloomFilter() {
}

TextBloomFilter::~TextBloomFilter() = default;

void TextBloomFilter::Init(const std::vector<std::wstring>& dict) {
    if (!bloom_filter_)
        bloom_filter_ = std::make_shared<BloomFilter>(dict.size());
    for (auto& key : dict) {
        bloom_filter_->Add(key);
    }
}

bool TextBloomFilter::ContainsKey(const std::wstring& key) {
    return bloom_filter_->Contains(key);
}


bool TextBloomFilter::Contains(const std::wstring& text) {
    std::vector<SearchResult> results;
    return FullTextSearch(text, &results) > 0;
}

size_t TextBloomFilter::Replace(const std::wstring& text, const std::wstring& rep, std::wstring* output) {
    std::vector<SearchResult> results;
    if (FullTextSearch(text, &results) == 0) {
        output->assign(text);
        return 0;
    }
    std::wstring w_output;
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
    return results.size();
}

size_t TextBloomFilter::FullTextSearch(const std::wstring& text, std::vector<SearchResult>* results) {
    if (text.empty()) return 0;

    int i = 0;
    int len = (int)text.size();
    size_t num = 0;
    while (i < len) {
        int step = 1;
        int first = i + (int)bloom_filter_->min_key_size();
        int last = (std::min)(len, i + (int)bloom_filter_->max_key_size());
        for (int j = last; j >= first; j--) {
            if (bloom_filter_->Contains(&text[i], j - i)) {
                SearchResult result;
                result.pos = i;
                result.len = j - i;
                step = (int)result.len;
                results->emplace_back(std::move(result));
                ++num;
                break;
            }
        }
        i += step;
    }
    return num;
}
}
}