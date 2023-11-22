// Copyright (C), Xianfeng Shang.  All rights reserved.
// Author: Xianfeng Shang (shangxianfeng@outlook.com)
#pragma once
#include <string.h>
#include <string>
#include <map>
#include <memory>
#include <vector>

namespace tinynet {
namespace cmdline {
class Parser {
  public:
    enum class OptionRule { required, optional };

    enum  class OptionType { kv, flag };

    struct Option {
        std::string name;
        char short_name{ 0 };
        std::string value;
        std::string desc;
        OptionType type{ OptionType::kv };
        OptionRule rule{ OptionRule::required };
        bool has_value{ false };
    };

    typedef std::shared_ptr<Option> OptionPtr;

    void add(const std::string& name,
             char short_name,
             const std::string& desc,
             OptionType type = OptionType::kv,
             OptionRule rule = OptionRule::required,
             const std::string& default_value = "") {
        auto option = std::make_shared<Option>();
        option->name = name;
        option->short_name = short_name;
        option->desc = desc;
        option->type = type;
        option->rule = rule;
        option->value = default_value;
        option->has_value = false;
        option_map_[name] = option;
        option_vec_.emplace_back(std::move(option));
    }

    bool parse(const std::vector<std::string>& argv) {
        errors_.clear();
        warnings_.clear();
        if (argv.empty()) {
            errors_.push_back("Arguments number must be at least 1");
            return false;
        }
        size_t i = argv[0].find_last_of("/\\");
        if (i == std::string::npos) {
            program_name_ = argv[0];
        } else {
            program_name_ = argv[0].substr(i + 1);
        }
        std::map<char, OptionPtr > option_map;
        for (auto& option : option_vec_) {
            if (option->short_name) {
                if (option_map.find(option->short_name) == option_map.end()) {
                    option_map[option->short_name] = option;
                } else {
                    errors_.push_back("Short option -" + std::string(1, option->short_name) + "is ambiguous");
                    return false;
                }

            }
        }
        for (size_t i = 1; i < argv.size(); ++i) {
            if (argv[i].length() < 2) {
                continue;
            }
            if (argv[i][0] == '-') {
                if (argv[i][1] == '-') {
                    size_t pos = argv[i].find_first_of("=:", 2);
                    if (pos == std::string::npos) {
                        std::string name = argv[i].substr(pos + 1);
                        if (option_map_.find(name) == option_map_.end()) {
                            warnings_.push_back("Unknown option: --" + name);
                            continue;
                        }
                        std::string value;
                        if (option_map_[name]->type == OptionType::kv) {
                            if (i + 1 < argv.size()) {
                                ++i;
                                value = argv[i];
                            }
                        }
                        set_option(name, value);
                    } else {
                        set_option(argv[i].substr(2, pos - 2), argv[i].substr(pos + 1));
                    }
                } else {
                    //Short name option
                    char name = argv[i][1];
                    if (option_map.find(name) == option_map.end()) {
                        warnings_.push_back("Unknown short option: -" + std::string(1,name));
                        continue;
                    }
                    std::string value;
                    if (option_map[name]->type == OptionType::kv) {
                        if (i + 1 < argv.size()) {
                            ++i;
                            value = argv[i];
                        }
                    }
                    set_option(option_map[name]->name, value);
                }
            } else {
                //Other unknown options
                warnings_.push_back("Unknown option:" + argv[i]);
            }
        }
        for (auto& option : option_vec_) {
            if (option->rule == OptionRule::required && !option->has_value) {
                errors_.push_back("Option needs value: --" + option->name);
            }
        }
        return errors_.size() == 0;
    }
    void set_option(const std::string& name, const std::string& value) {
        auto it = option_map_.find(name);
        if (it == option_map_.end()) {
            warnings_.push_back("Unknown option: --" + name);
            return;
        }
        it->second->value = value;
        it->second->has_value = true;
    }

    std::string usage() const {
        std::string msg;
        std::string options;
        msg += program_name_;
        msg += " ";
        for (auto& option : option_vec_) {
            if (option->rule == OptionRule::required) {
                msg += "--";
                msg += option->name;
                if (option->type == OptionType::kv) {
                    msg += "=";
                    msg += "\"";
                    msg += option->value;
                    msg += "\"";
                }
            } else {
                options += "-";
                options += std::string(1, option->short_name);
                options += ",";
                options += "--";
                options += option->name;
                options += ",";
                options += option->desc;
                options += "\n";
            }
        }
        msg += " [options] ...";
        msg += "\n";
        msg += "options\n";
        msg += options;
        return msg;
    }
    void parse_check(const std::vector<std::string>& argv) {
        if (option_map_.find("help") == option_map_.end()) {
            add("help", '?', "print this message", OptionType::flag, OptionRule::optional);
        }
        if (parse(argv)) {
            if (has("help")) {
                auto msg = usage();
                fprintf(stderr, "%.*s", (int)msg.length(), msg.data());
                fflush(stderr);
                exit(0);
            }
            return;
        }
        for (auto& err : warnings_) {
            fprintf(stderr, "%.*s\n", (int)err.length(), err.data());
        }
        for (auto& err : errors_) {
            fprintf(stderr, "%.*s\n", (int)err.length(), err.data());
        }
        auto msg = usage();
        fprintf(stderr, "%.*s", (int)msg.length(), msg.data());
        fflush(stderr);
        exit(1);
    }

    bool has(const std::string& name) {
        auto it = option_map_.find(name);
        if (it == option_map_.end()) {
            return false;
        }
        return it->second->has_value;
    }

    std::string get(const std::string& name) {
        auto it = option_map_.find(name);
        if (it == option_map_.end()) {
            return "";
        }
        return it->second->value;
    }
  private:
    std::map<std::string, OptionPtr > option_map_;
    std::vector<OptionPtr> option_vec_;
    std::vector<std::string> errors_;
    std::vector<std::string> warnings_;
    std::string program_name_;
};
}
}
