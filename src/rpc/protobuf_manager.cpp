// Copyright (C), Xianfeng Shang.  All rights reserved.
// Author: Xianfeng Shang (shangxianfeng@outlook.com)
#include "protobuf_manager.h"
#include "util/string_utils.h"
#include "app/app_container.h"
#include "tfs/tfs_service.h"
#include "base/at_exit.h"

namespace tinynet {
namespace rpc {

TMultiFileErrorCollector::TMultiFileErrorCollector(ProtobufManager *protobuf) :
    protobuf_(protobuf) {

}

void TMultiFileErrorCollector::AddError(const std::string& filename, int line, int column, const std::string& message) {
    if (protobuf_) {
        std::string error_msg;
        StringUtils::Format(error_msg, "%s:%d:%d:%s\n", filename.c_str(), line, column, message.c_str());
        protobuf_->AddCompileError(error_msg);
    }
}

TfsSourceTree::TfsSourceTree(app::AppContainer* app):
    app_(app),
    io_buf_(),
    stream_(&io_buf_) {

}
google::protobuf::io::ZeroCopyInputStream* TfsSourceTree::Open(const std::string& filename) {
    std::string disk_filename;
    if (!FindFile(filename, &disk_filename)) {
        last_error_message_ = "File not found";
        return nullptr;
    }
    stream_.seekg(0);
    io_buf_.clear();
    if (!app_->get<tfs::TfsService>()->LoadFile(disk_filename, &io_buf_)) {
        last_error_message_ = "Load file error";
        return nullptr;
    }
    ZeroCopyInputStream* stream = new ZeroCopyInputStream(&io_buf_);
    return stream;
}

std::string TfsSourceTree::GetLastErrorMessage() {
    return last_error_message_;
}

void TfsSourceTree::MapPath(const std::string& virtual_path, const std::string& disk_path) {
    if (StringUtils::EndWith(disk_path, "/\\")) {
        mappings_[virtual_path] = disk_path.substr(0, disk_path.length() - 1);
    } else {
        mappings_[virtual_path] = disk_path;
    }
}

static std::string MakePath(const std::string& file_path, const std::string& virtual_path, const std::string& disk_path) {
    const char* p = file_path.c_str();
    if (virtual_path.length() > 0) {
        size_t i = file_path.find(virtual_path);
        if (i != 0) {
            return file_path;
        }
        p += virtual_path.length();
    }
    std::string result;
    result.append(disk_path);
    if (p[0] != '/' && p[0] != '\\') {
        result.append(1, '/');
    }
    result.append(p);
    return result;
}

bool TfsSourceTree::FindFile(const std::string& file_path, std::string* output) {
    if (app_->get<tfs::TfsService>()->Exists(file_path)) {
        output->assign(file_path);
        return true;
    }
    for (auto& pair : mappings_) {
        std::string path = MakePath(file_path, pair.first, pair.second);
        if (app_->get<tfs::TfsService>()->Exists(path)) {
            *output = std::move(path);
            return true;
        }
    }
    return false;
}

std::once_flag ProtobufManager::once_flag_;

void ProtobufManager::Initialize() {
    GOOGLE_PROTOBUF_VERIFY_VERSION;
    tinynet::AtExitManager::RegisterCallback(Finalize, nullptr);
}

void ProtobufManager::Finalize(void*) {
    google::protobuf::ShutdownProtobufLibrary();
}


ProtobufManager::ProtobufManager(app::AppContainer* app):
    app_(app) {

}

ProtobufManager::~ProtobufManager() = default;

void ProtobufManager::Init() {
    std::call_once(once_flag_, Initialize);
    Reset();
}

void ProtobufManager::MapPath(const std::string &virtual_path, const std::string &disk_path) {
    sourceTree_->MapPath(virtual_path, disk_path);
}

bool ProtobufManager::Import(const std::string &filename) {
    auto descriptor = importer_->Import(filename);
    if (!descriptor) {
        return false;
    }
    return true;
}

google::protobuf::Message* ProtobufManager::CreateMessage(const std::string &message_name) {
    auto descriptor = importer_->pool()->FindMessageTypeByName(message_name);
    if (!descriptor) {
        return nullptr;
    }
    auto prototype = factory_->GetPrototype(descriptor);
    if (!prototype) {
        return nullptr;
    }
    return prototype->New();
}
google::protobuf::Message* ProtobufManager::CreateRpcRequest(const std::string &method_name) {
    auto method = importer_->pool()->FindMethodByName(method_name);
    if (!method) {
        return nullptr;
    }
    auto descriptor = method->input_type();
    if (!descriptor) {
        return nullptr;
    }
    auto prototype = factory_->GetPrototype(descriptor);
    if (!prototype) {
        return nullptr;
    }
    return prototype->New();
}

google::protobuf::Message* ProtobufManager::CreateRpcResponse(const std::string &method_name) {
    auto method = importer_->pool()->FindMethodByName(method_name);
    if (!method) {
        return nullptr;
    }
    auto descriptor = method->output_type();
    if (!descriptor) {
        return nullptr;
    }
    auto prototype = factory_->GetPrototype(descriptor);
    if (!prototype) {
        return nullptr;
    }
    return prototype->New();
}

google::protobuf::DynamicMessageFactory* ProtobufManager::GetFactory() {
    return factory_.get();
}

void ProtobufManager::Clear() {
    Reset();
    compile_errors_.clear();
}

void ProtobufManager::Reset() {
    sourceTree_ = std::make_shared<TfsSourceTree>(app_);
    //sourceTree_ = std::make_shared<google::protobuf::compiler::DiskSourceTree>();
    errorHandler_ = std::make_shared<TMultiFileErrorCollector>(this);
    importer_ = std::make_shared<google::protobuf::compiler::Importer>(sourceTree_.get(), errorHandler_.get());
    factory_ = std::make_shared<google::protobuf::DynamicMessageFactory>();
}

void ProtobufManager::AddCompileError(const std::string &message) {
    compile_errors_.append(message);
}
}
}