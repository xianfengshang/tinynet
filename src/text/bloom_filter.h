#include <vector>
namespace tinynet {
namespace text {
class BloomFilter{
  public:
    BloomFilter(size_t size);
    ~BloomFilter();
  public:
    void SetBit(size_t bitpos) {
        bits_[bitpos / 8] |= 1 << (bitpos % 8);
    }
    bool TestBit(size_t bitpos) {
        return (bits_[bitpos / 8] & (1 << (bitpos % 8)));
    }
  public:
    void Add(const std::wstring& key);
    bool Contains(const std::wstring& key);
    bool Contains(const wchar_t* key, size_t len);
  public:
    size_t max_key_size() { return max_key_size_; }
    size_t min_key_size() { return min_key_size_; }
  private:
    std::vector<uint8_t> bits_;
    size_t min_key_size_;
    size_t max_key_size_;
};
}
}
