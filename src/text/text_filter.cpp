// Copyright (C), Xianfeng Shang.  All rights reserved.
// Author: Xianfeng Shang (shangxianfeng@outlook.com)
#include "text_filter.h"
#include <vector>
#include <stack>
#include <algorithm>
#include "util/string_utils.h"
#include "text_trie_filter.h"
#include "text_bloom_filter.h"
#include "text_darts_filter.h"
#include "text_radix_filter.h"

namespace tinynet {
namespace text {

TextFilter::TextFilter(int type) {
    switch (type) {
    case TRIE_FILTER: {
        filter_impl_ = std::make_shared<TextTrieFilter>();
        break;
    }
    case BLOOM_FILTER: {
        filter_impl_ = std::make_shared<TextBloomFilter>();
        break;
    }
    case DARTS_FILTER: {
        filter_impl_ = std::make_shared<TextDartsFilter>();
        break;
    }
    case RADIX_FILTER: {
        filter_impl_ = std::make_shared<TextRadixFilter>();
        break;
    }
    default:
        filter_impl_ = std::make_shared<TextRadixFilter>();
        break;
    }
}

TextFilter::~TextFilter() = default;

void TextFilter::Init(const std::vector<std::string>& dict) {
    std::vector<std::wstring> wdict;
    for (auto& key : dict) {
        wdict.emplace_back(StringUtils::to_utf16_string(key));
    }
    filter_impl_->Init(wdict);
}

bool TextFilter::ContainsKey(const std::string& key) {
    std::wstring wkey = StringUtils::to_utf16_string(key);
    return filter_impl_->ContainsKey(wkey);
}


bool TextFilter::Contains(const std::string& text) {
    std::wstring wtext = StringUtils::to_utf16_string(text);
    return filter_impl_->Contains(wtext);
}

size_t TextFilter::Replace(const std::string& text, const std::string& rep, std::string* output) {
    std::wstring wtext = StringUtils::to_utf16_string(text);
    std::wstring wrep = StringUtils::to_utf16_string(rep);
    std::wstring woutput;
    size_t n = filter_impl_->Replace(wtext, wrep, &woutput);
    if (!woutput.empty())
        StringUtils::convert_utf16_to_utf8(woutput, output);
    return n;
}

}
}