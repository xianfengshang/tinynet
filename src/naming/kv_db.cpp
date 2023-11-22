// Copyright (C), Xianfeng Shang.  All rights reserved.
// Author: Xianfeng Shang (shangxianfeng@outlook.com)
#include "kv_db.h"

namespace tinynet {
namespace naming {
KVTreeNode::KVTreeNode():
    parent_(nullptr) {
}

KVTreeNode::~KVTreeNode() = default;

const KVTreeNodePtr KVTreeNode::find_child(const std::string& key) const {
    auto it = children_.find(key);
    if (it != children_.end()) {
        return it->second;
    }
    return KVTreeNodePtr();
}

KVTreeNodePtr KVTreeNode::mutable_child(const std::string& key) {
    auto it = children_.find(key);
    if (it != children_.end()) {
        return it->second;
    }
    auto node = std::make_shared<KVTreeNode>();
    node->name_ = key;
    node->parent_ = this;
    children_.emplace(key, node);
    return node;
}

void KVTreeNode::remove_child(const std::string& name) {
    children_.erase(name);
}

const KVTreeNodePtr KVTreeNode::find_node(const std::string& key) const {
    size_t pos = key.find_first_of(":/\\");
    if (pos == std::string::npos) {
        return find_child(key);
    }
    auto child_name = key.substr(0, pos);
    auto child_node = find_child(child_name);
    if (pos == (key.length() - 1) || !child_node) {
        return child_node;
    }
    return child_node->find_node(key.substr(pos + 1));

}

KVTreeNodePtr KVTreeNode::mutable_node(const std::string& key) {
    if (key.length() == 0) {
        return shared_from_this();
    }
    size_t begin_pos = key.find_first_not_of(":/\\");
    if (begin_pos == std::string::npos) {
        return shared_from_this();
    }
    size_t end_pos = key.find_first_of(":/\\", begin_pos);
    auto child_name = key.substr(begin_pos, end_pos - begin_pos);
    auto child_node = mutable_child(child_name);
    if (end_pos == std::string::npos || (end_pos == key.length() - 1) || !child_node) {
        return child_node;
    }
    return child_node->mutable_node(key.substr(end_pos + 1));
}

void KVTreeNode::retrieve_data(std::vector<std::string>* results) {
    if (!data_.empty()) {
        results->push_back(data_);
    }
    for (auto& child: children_) {
        child.second->retrieve_data(results);
    }
}

KVTree::KVTree() {
    root_ = std::make_shared<KVTreeNode>();
}

KVTree::~KVTree() = default;

void KVTree::insert(const std::string& data) {
    auto node = root_->mutable_node(data);
    if (node) {
        node->data_ = data;
    }
}

void KVTree::erase(const std::string& key) {
    auto node = root_->find_node(key);
    if (node) {
        node->data_ = "";
        if (node->parent_ && node->children_.empty()) {
            node->parent_->remove_child(node->name_);
        }
    }
}

void KVTree::find(const std::string& key, std::vector<std::string>* results) {
    auto node = root_->find_node(key);
    if (node) {
        node->retrieve_data(results);
    }
}

void KVTree::clear() {
    root_ = std::make_shared<KVTreeNode>();
}

bool KVDB::get(const std::string& key, std::string* value, int64_t* expire_at) {
    auto it = data_dict_.find(key);
    if (it == data_dict_.end()) {
        return false;
    }
    if (value)
        *value = it->second->value;
    if (expire_at)
        *expire_at = it->second->expire_at;
    return true;
}

void KVDB::put(const std::string& key, const std::string& value, int64_t expire_at) {
    if (data_dict_.find(key) != data_dict_.end()) {
        del(key);
    }
    kv_pair_ptr_t data_ptr = std::make_shared<kv_pair_t>();
    data_ptr->key = key;
    data_ptr->value = value;
    data_ptr->expire_at = expire_at;
    data_dict_[key] = data_ptr;

    name_tree_.insert(key);
}

void KVDB::del(const std::string& key) {
    auto it = data_dict_.find(key);
    if (it == data_dict_.end()) {
        return;
    }
    name_tree_.erase(key);
    data_dict_.erase(it);
}

void KVDB::keys(const std::string& key_prefix, std::vector<std::string>* output_keys, int64_t expire_at) {
    if (!output_keys) {
        return;
    }
    std::vector<std::string> keys;
    name_tree_.find(key_prefix, &keys);
    for (auto& key : keys) {
        auto it = data_dict_.find(key);
        if (it != data_dict_.end() && (it->second->expire_at <= 0 || it->second->expire_at > expire_at)) {
            output_keys->push_back(key);
        }
    }
}

void KVDB::snapshot(int64_t now, std::vector<kv_pair_ptr_t>* output) {
    auto it = data_dict_.begin();
    while (it != data_dict_.end()) {
        auto& data = it->second;
        if (data->expire_at > 0 && now >= data->expire_at) {
            it = data_dict_.erase(it);
        } else {
            output->push_back(data);
            ++it;
        }
    }
}

void KVDB::clear() {
    data_dict_.clear();
    name_tree_.clear();
}
}
}
