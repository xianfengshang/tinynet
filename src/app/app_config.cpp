// Copyright (C), Xianfeng Shang.  All rights reserved.
// Author: Xianfeng Shang (shangxianfeng@outlook.com)
#include "app_config.h"
#include "util/string_utils.h"
#include "base/runtime_logger.h"

namespace tinynet {
namespace app {
AppConfig::AppConfig() {
}

AppConfig::~AppConfig() {
}

int AppConfig::Init(const std::string& app_names, const std::string& app_labels) {
    std::vector<std::string> names;
    StringUtils::Split(app_names, ";|", names);
    for (auto& name : names) {
        AppMeta meta;
        meta.name = StringUtils::Trim(name);
        if (!meta.name.empty()) {
            meta_infos_.push_back(meta);
        }
    }
    if (meta_infos_.empty()) {
        return 1;
    }

    std::vector<std::string> labels;
    StringUtils::Split(app_labels, ";|", labels);
    for (size_t i = 0; i < labels.size(); ++i) {
        if (i == meta_infos_.size()) {
            meta_infos_.push_back(meta_infos_[i - 1]);
        }
        if (!ParseLabels(meta_infos_[i], labels[i])) {
            return 1;
        }
    }
    return 0;
}

bool AppConfig::ParseLabels(AppMeta& meta, const std::string& app_labels) {
    meta.labels.clear();
    std::vector<std::string> labels;
    StringUtils::Split(app_labels, ",&", labels);
    for (auto &label : labels) {
        std::vector<std::string> kv;
        StringUtils::Split(label, ":=", kv);
        std::string key = StringUtils::Trim(kv[0]);
        std::string value;
        if (kv.size() > 1) {
            value = StringUtils::Trim(kv[1]);
        }
        meta.labels[key] = value;
    }
    return true;
}
}
}
