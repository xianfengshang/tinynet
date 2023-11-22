// Copyright (C), Xianfeng Shang.  All rights reserved.
// Author: Xianfeng Shang (shangxianfeng@outlook.com)
#pragma once
#include <string>
#include <memory>
#include <vector>
#include <map>

namespace tinynet {
namespace naming {
struct kv_pair_t {
    std::string key;
    std::string value;
    int64_t expire_at{ -1 };
};
typedef std::shared_ptr<kv_pair_t> kv_pair_ptr_t;


class KVTreeNode;
typedef std::shared_ptr<KVTreeNode> KVTreeNodePtr;

class KVTreeNode:
    public std::enable_shared_from_this<KVTreeNode> {
    friend class KVTree;
  public:
    KVTreeNode();
    ~KVTreeNode();
  public:
    const KVTreeNodePtr find_child(const std::string& key) const;
    KVTreeNodePtr mutable_child(const std::string& key);
    void remove_child(const std::string& key);
    const KVTreeNodePtr find_node(const std::string& key) const;
    KVTreeNodePtr mutable_node(const std::string& key);
    void retrieve_data(std::vector<std::string>* results);
  private:
    std::string name_;
    std::string data_;
    KVTreeNode* parent_;
    std::map<std::string, KVTreeNodePtr> children_;
};

class KVTree {
  public:
    KVTree();
    ~KVTree();
  public:
    void insert(const std::string& data);
    void erase(const std::string& key);
    void find(const std::string& key, std::vector<std::string>* results);
    void clear();
  private:
    KVTreeNodePtr root_;
};

class KVDB {
  public:
    bool get(const std::string& key, std::string* value, int64_t* expire_at = nullptr);
    void put(const std::string& key, const std::string& value, int64_t expire_at = -1);
    void del(const std::string& key);
    void keys(const std::string& key_prefix, std::vector<std::string>* output_keys, int64_t expire_at);

    void snapshot(int64_t now, std::vector<kv_pair_ptr_t>* output);
    void clear();
  private:
    std::map<std::string, kv_pair_ptr_t> data_dict_;
    KVTree name_tree_;
};

}
}
