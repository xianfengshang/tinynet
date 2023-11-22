#include <algorithm>
#include "bloom_filter.h"
#include "base/base.h"

namespace tinynet {
namespace text {

static uint32_t HashSeeds[] = { 13, 131, 1313, 13131,131313 };
static const int SeedCount = sizeof(HashSeeds)/sizeof(HashSeeds[0]);

static uint64_t BloomHash(const wchar_t* data, size_t len, uint32_t seed) {
    uint32_t h = 0;
    uint32_t l = seed;
    for (size_t i = 0; i < len; ++i) {
        int ch = data[i];
        h = h * seed + ch;
        l = ((l << 5) + l) + ch;
    }
    h = h & 0x7FFFFFFF;
    return (unsigned long long)h << 32 | l;
}

BloomFilter::BloomFilter(size_t size) {
    if (size < 64) size = 64;
    size_t bitCount =  ALIGN_UP(size * 30, 8);
    size_t byteCount = bitCount / 8;
    bits_.resize(byteCount);
    min_key_size_ = 65535;
    max_key_size_ = 0;
}

BloomFilter::~BloomFilter() {
}

void BloomFilter::Add(const std::wstring& key) {
    size_t bits = bits_.size() * 8;
    for (int i = 0; i < SeedCount; ++i) {
        auto hash = BloomHash(key.data(), key.size(), HashSeeds[i]);
        uint32_t h = (hash >> 32) & 0xffffffff;
        uint32_t l = hash & 0xffffffff;
        SetBit(h % bits);
        SetBit(l % bits);
    }
    size_t keySize = key.length();
    if (keySize < min_key_size_)
        min_key_size_ = keySize;
    if (keySize > max_key_size_)
        max_key_size_ = keySize;
}

bool BloomFilter::Contains(const std::wstring& key) {
    return Contains(key.data(), key.length());
}

bool BloomFilter::Contains(const wchar_t* key, size_t len) {
    if (len < min_key_size_ || len > max_key_size_)
        return false;
    size_t bits = bits_.size() * 8;
    for (int i = 0; i < SeedCount; ++i) {
        auto hash = BloomHash(key, len, HashSeeds[i]);
        uint32_t h = (hash >> 32) & 0xffffffff;
        uint32_t l = hash & 0xffffffff;
        if (!TestBit(h % bits)) return false;
        if (!TestBit(l % bits)) return false;
    }
    return true;
}

}
}