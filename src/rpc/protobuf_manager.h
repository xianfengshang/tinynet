// Copyright (C), Xianfeng Shang.  All rights reserved.
// Author: Xianfeng Shang (shangxianfeng@outlook.com)
#pragma once
#include <google/protobuf/descriptor.h>
#include <google/protobuf/descriptor_database.h>
#include <google/protobuf/message.h>
#include <google/protobuf/repeated_field.h>
#include <google/protobuf/dynamic_message.h>
#include <google/protobuf/stubs/common.h>
#include <google/protobuf/compiler/importer.h>
#include <memory>
#include <unordered_map>
#include "base/singleton.h"
#include "zero_copy_stream.h"
#include "base/io_buffer.h"

namespace tinynet {
namespace app {
class AppContainer;
}
namespace rpc {
class ProtobufManager;
class TMultiFileErrorCollector :
    public google::protobuf::compiler::MultiFileErrorCollector {
  public:
    TMultiFileErrorCollector(ProtobufManager *protobuf);
  public:
    void AddError(const std::string& filename,
                  int line,
                  int column,
                  const std::string& message) override;
  private:
    ProtobufManager * protobuf_;
};

class TfsSourceTree : public google::protobuf::compiler::SourceTree {
  public:
    TfsSourceTree(app::AppContainer* app);
  public:
    virtual google::protobuf::io::ZeroCopyInputStream* Open(const std::string& filename) override;
    virtual std::string GetLastErrorMessage() override;
  public:
    void MapPath(const std::string& virtual_path, const std::string& disk_path);
  private:
    bool FindFile(const std::string& file_path, std::string* output);
  public:
    app::AppContainer* app_;
    IOBuffer io_buf_;
    IBufferStream stream_;
    std::string last_error_message_;
    std::unordered_map<std::string, std::string> mappings_;
};

class ProtobufManager {
  public:
  public:
    ProtobufManager(app::AppContainer* app);
    ~ProtobufManager();
    void Init();
    void MapPath(const std::string &virtual_path, const std::string &disk_path);
    bool Import(const std::string &filename);
    google::protobuf::Message* CreateMessage(const std::string &message_name);
    google::protobuf::Message* CreateRpcRequest(const std::string &method_name);
    google::protobuf::Message* CreateRpcResponse(const std::string &method_name);
    google::protobuf::DynamicMessageFactory* GetFactory();
    void Clear();
  public:
    void AddCompileError(const std::string &message);
  public:
    const std::string& get_compile_errors() const { return compile_errors_; }
  private:
    void Reset();
  private:
    static std::once_flag once_flag_;
    static void Initialize();
    static void Finalize(void*);
  private:
    using DiskSourceTreePtr = std::shared_ptr<google::protobuf::compiler::DiskSourceTree>;
    using TfsSourceTreePtr = std::shared_ptr<TfsSourceTree>;
    using SourceTreePtr = std::shared_ptr<google::protobuf::compiler::SourceTree>;
    using MultiFileErrorCollectorPtr = std::shared_ptr<TMultiFileErrorCollector>;
    using ImporterPtr = std::shared_ptr<google::protobuf::compiler::Importer>;
    using DynamicMessageFactoryPtr = std::shared_ptr<google::protobuf::DynamicMessageFactory>;

    app::AppContainer*			app_;
    TfsSourceTreePtr			sourceTree_;
    MultiFileErrorCollectorPtr	errorHandler_;
    ImporterPtr					importer_;
    DynamicMessageFactoryPtr	factory_;
    std::string					compile_errors_;
};
}
}