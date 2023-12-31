// Generated by the protocol buffer compiler.  DO NOT EDIT!
// source: cli.proto

#define INTERNAL_SUPPRESS_PROTOBUF_FIELD_DEPRECATION
#include "cli.pb.h"

#include <algorithm>

#include <google/protobuf/stubs/common.h>
#include <google/protobuf/stubs/once.h>
#include <google/protobuf/io/coded_stream.h>
#include <google/protobuf/wire_format_lite_inl.h>
#include <google/protobuf/descriptor.h>
#include <google/protobuf/generated_message_reflection.h>
#include <google/protobuf/reflection_ops.h>
#include <google/protobuf/wire_format.h>
// @@protoc_insertion_point(includes)

namespace tinynet {
namespace raft {

namespace {

const ::google::protobuf::Descriptor* GetLeaderReq_descriptor_ = NULL;
const ::google::protobuf::internal::GeneratedMessageReflection*
  GetLeaderReq_reflection_ = NULL;
const ::google::protobuf::Descriptor* GetLeaderResp_descriptor_ = NULL;
const ::google::protobuf::internal::GeneratedMessageReflection*
  GetLeaderResp_reflection_ = NULL;
const ::google::protobuf::ServiceDescriptor* RaftCliRpcService_descriptor_ = NULL;

}  // namespace


void protobuf_AssignDesc_cli_2eproto() {
  protobuf_AddDesc_cli_2eproto();
  const ::google::protobuf::FileDescriptor* file =
    ::google::protobuf::DescriptorPool::generated_pool()->FindFileByName(
      "cli.proto");
  GOOGLE_CHECK(file != NULL);
  GetLeaderReq_descriptor_ = file->message_type(0);
  static const int GetLeaderReq_offsets_[1] = {
  };
  GetLeaderReq_reflection_ =
    new ::google::protobuf::internal::GeneratedMessageReflection(
      GetLeaderReq_descriptor_,
      GetLeaderReq::default_instance_,
      GetLeaderReq_offsets_,
      GOOGLE_PROTOBUF_GENERATED_MESSAGE_FIELD_OFFSET(GetLeaderReq, _has_bits_[0]),
      GOOGLE_PROTOBUF_GENERATED_MESSAGE_FIELD_OFFSET(GetLeaderReq, _unknown_fields_),
      -1,
      ::google::protobuf::DescriptorPool::generated_pool(),
      ::google::protobuf::MessageFactory::generated_factory(),
      sizeof(GetLeaderReq));
  GetLeaderResp_descriptor_ = file->message_type(1);
  static const int GetLeaderResp_offsets_[2] = {
    GOOGLE_PROTOBUF_GENERATED_MESSAGE_FIELD_OFFSET(GetLeaderResp, leadername_),
    GOOGLE_PROTOBUF_GENERATED_MESSAGE_FIELD_OFFSET(GetLeaderResp, leaderaddress_),
  };
  GetLeaderResp_reflection_ =
    new ::google::protobuf::internal::GeneratedMessageReflection(
      GetLeaderResp_descriptor_,
      GetLeaderResp::default_instance_,
      GetLeaderResp_offsets_,
      GOOGLE_PROTOBUF_GENERATED_MESSAGE_FIELD_OFFSET(GetLeaderResp, _has_bits_[0]),
      GOOGLE_PROTOBUF_GENERATED_MESSAGE_FIELD_OFFSET(GetLeaderResp, _unknown_fields_),
      -1,
      ::google::protobuf::DescriptorPool::generated_pool(),
      ::google::protobuf::MessageFactory::generated_factory(),
      sizeof(GetLeaderResp));
  RaftCliRpcService_descriptor_ = file->service(0);
}

namespace {

GOOGLE_PROTOBUF_DECLARE_ONCE(protobuf_AssignDescriptors_once_);
inline void protobuf_AssignDescriptorsOnce() {
  ::google::protobuf::GoogleOnceInit(&protobuf_AssignDescriptors_once_,
                 &protobuf_AssignDesc_cli_2eproto);
}

void protobuf_RegisterTypes(const ::std::string&) {
  protobuf_AssignDescriptorsOnce();
  ::google::protobuf::MessageFactory::InternalRegisterGeneratedMessage(
    GetLeaderReq_descriptor_, &GetLeaderReq::default_instance());
  ::google::protobuf::MessageFactory::InternalRegisterGeneratedMessage(
    GetLeaderResp_descriptor_, &GetLeaderResp::default_instance());
}

}  // namespace

void protobuf_ShutdownFile_cli_2eproto() {
  delete GetLeaderReq::default_instance_;
  delete GetLeaderReq_reflection_;
  delete GetLeaderResp::default_instance_;
  delete GetLeaderResp_reflection_;
}

void protobuf_AddDesc_cli_2eproto() {
  static bool already_here = false;
  if (already_here) return;
  already_here = true;
  GOOGLE_PROTOBUF_VERIFY_VERSION;

  ::google::protobuf::DescriptorPool::InternalAddGeneratedFile(
    "\n\tcli.proto\022\014tinynet.raft\"\016\n\014GetLeaderRe"
    "q\":\n\rGetLeaderResp\022\022\n\nleaderName\030\001 \001(\t\022\025"
    "\n\rleaderAddress\030\002 \001(\t2Y\n\021RaftCliRpcServi"
    "ce\022D\n\tGetLeader\022\032.tinynet.raft.GetLeader"
    "Req\032\033.tinynet.raft.GetLeaderRespB\003\200\001\001", 197);
  ::google::protobuf::MessageFactory::InternalRegisterGeneratedFile(
    "cli.proto", &protobuf_RegisterTypes);
  GetLeaderReq::default_instance_ = new GetLeaderReq();
  GetLeaderResp::default_instance_ = new GetLeaderResp();
  GetLeaderReq::default_instance_->InitAsDefaultInstance();
  GetLeaderResp::default_instance_->InitAsDefaultInstance();
  ::google::protobuf::internal::OnShutdown(&protobuf_ShutdownFile_cli_2eproto);
}

// Force AddDescriptors() to be called at static initialization time.
struct StaticDescriptorInitializer_cli_2eproto {
  StaticDescriptorInitializer_cli_2eproto() {
    protobuf_AddDesc_cli_2eproto();
  }
} static_descriptor_initializer_cli_2eproto_;

// ===================================================================

#ifndef _MSC_VER
#endif  // !_MSC_VER

GetLeaderReq::GetLeaderReq()
  : ::google::protobuf::Message() {
  SharedCtor();
  // @@protoc_insertion_point(constructor:tinynet.raft.GetLeaderReq)
}

void GetLeaderReq::InitAsDefaultInstance() {
}

GetLeaderReq::GetLeaderReq(const GetLeaderReq& from)
  : ::google::protobuf::Message() {
  SharedCtor();
  MergeFrom(from);
  // @@protoc_insertion_point(copy_constructor:tinynet.raft.GetLeaderReq)
}

void GetLeaderReq::SharedCtor() {
  _cached_size_ = 0;
  ::memset(_has_bits_, 0, sizeof(_has_bits_));
}

GetLeaderReq::~GetLeaderReq() {
  // @@protoc_insertion_point(destructor:tinynet.raft.GetLeaderReq)
  SharedDtor();
}

void GetLeaderReq::SharedDtor() {
  if (this != default_instance_) {
  }
}

void GetLeaderReq::SetCachedSize(int size) const {
  GOOGLE_SAFE_CONCURRENT_WRITES_BEGIN();
  _cached_size_ = size;
  GOOGLE_SAFE_CONCURRENT_WRITES_END();
}
const ::google::protobuf::Descriptor* GetLeaderReq::descriptor() {
  protobuf_AssignDescriptorsOnce();
  return GetLeaderReq_descriptor_;
}

const GetLeaderReq& GetLeaderReq::default_instance() {
  if (default_instance_ == NULL) protobuf_AddDesc_cli_2eproto();
  return *default_instance_;
}

GetLeaderReq* GetLeaderReq::default_instance_ = NULL;

GetLeaderReq* GetLeaderReq::New() const {
  return new GetLeaderReq;
}

void GetLeaderReq::Clear() {
  ::memset(_has_bits_, 0, sizeof(_has_bits_));
  mutable_unknown_fields()->Clear();
}

bool GetLeaderReq::MergePartialFromCodedStream(
    ::google::protobuf::io::CodedInputStream* input) {
#define DO_(EXPRESSION) if (!(EXPRESSION)) goto failure
  ::google::protobuf::uint32 tag;
  // @@protoc_insertion_point(parse_start:tinynet.raft.GetLeaderReq)
  for (;;) {
    ::std::pair< ::google::protobuf::uint32, bool> p = input->ReadTagWithCutoff(127);
    tag = p.first;
    if (!p.second) goto handle_unusual;
  handle_unusual:
    if (tag == 0 ||
        ::google::protobuf::internal::WireFormatLite::GetTagWireType(tag) ==
        ::google::protobuf::internal::WireFormatLite::WIRETYPE_END_GROUP) {
      goto success;
    }
    DO_(::google::protobuf::internal::WireFormat::SkipField(
          input, tag, mutable_unknown_fields()));
  }
success:
  // @@protoc_insertion_point(parse_success:tinynet.raft.GetLeaderReq)
  return true;
failure:
  // @@protoc_insertion_point(parse_failure:tinynet.raft.GetLeaderReq)
  return false;
#undef DO_
}

void GetLeaderReq::SerializeWithCachedSizes(
    ::google::protobuf::io::CodedOutputStream* output) const {
  // @@protoc_insertion_point(serialize_start:tinynet.raft.GetLeaderReq)
  if (!unknown_fields().empty()) {
    ::google::protobuf::internal::WireFormat::SerializeUnknownFields(
        unknown_fields(), output);
  }
  // @@protoc_insertion_point(serialize_end:tinynet.raft.GetLeaderReq)
}

::google::protobuf::uint8* GetLeaderReq::SerializeWithCachedSizesToArray(
    ::google::protobuf::uint8* target) const {
  // @@protoc_insertion_point(serialize_to_array_start:tinynet.raft.GetLeaderReq)
  if (!unknown_fields().empty()) {
    target = ::google::protobuf::internal::WireFormat::SerializeUnknownFieldsToArray(
        unknown_fields(), target);
  }
  // @@protoc_insertion_point(serialize_to_array_end:tinynet.raft.GetLeaderReq)
  return target;
}

int GetLeaderReq::ByteSize() const {
  int total_size = 0;

  if (!unknown_fields().empty()) {
    total_size +=
      ::google::protobuf::internal::WireFormat::ComputeUnknownFieldsSize(
        unknown_fields());
  }
  GOOGLE_SAFE_CONCURRENT_WRITES_BEGIN();
  _cached_size_ = total_size;
  GOOGLE_SAFE_CONCURRENT_WRITES_END();
  return total_size;
}

void GetLeaderReq::MergeFrom(const ::google::protobuf::Message& from) {
  GOOGLE_CHECK_NE(&from, this);
  const GetLeaderReq* source =
    ::google::protobuf::internal::dynamic_cast_if_available<const GetLeaderReq*>(
      &from);
  if (source == NULL) {
    ::google::protobuf::internal::ReflectionOps::Merge(from, this);
  } else {
    MergeFrom(*source);
  }
}

void GetLeaderReq::MergeFrom(const GetLeaderReq& from) {
  GOOGLE_CHECK_NE(&from, this);
  mutable_unknown_fields()->MergeFrom(from.unknown_fields());
}

void GetLeaderReq::CopyFrom(const ::google::protobuf::Message& from) {
  if (&from == this) return;
  Clear();
  MergeFrom(from);
}

void GetLeaderReq::CopyFrom(const GetLeaderReq& from) {
  if (&from == this) return;
  Clear();
  MergeFrom(from);
}

bool GetLeaderReq::IsInitialized() const {

  return true;
}

void GetLeaderReq::Swap(GetLeaderReq* other) {
  if (other != this) {
    _unknown_fields_.Swap(&other->_unknown_fields_);
    std::swap(_cached_size_, other->_cached_size_);
  }
}

::google::protobuf::Metadata GetLeaderReq::GetMetadata() const {
  protobuf_AssignDescriptorsOnce();
  ::google::protobuf::Metadata metadata;
  metadata.descriptor = GetLeaderReq_descriptor_;
  metadata.reflection = GetLeaderReq_reflection_;
  return metadata;
}


// ===================================================================

#ifndef _MSC_VER
const int GetLeaderResp::kLeaderNameFieldNumber;
const int GetLeaderResp::kLeaderAddressFieldNumber;
#endif  // !_MSC_VER

GetLeaderResp::GetLeaderResp()
  : ::google::protobuf::Message() {
  SharedCtor();
  // @@protoc_insertion_point(constructor:tinynet.raft.GetLeaderResp)
}

void GetLeaderResp::InitAsDefaultInstance() {
}

GetLeaderResp::GetLeaderResp(const GetLeaderResp& from)
  : ::google::protobuf::Message() {
  SharedCtor();
  MergeFrom(from);
  // @@protoc_insertion_point(copy_constructor:tinynet.raft.GetLeaderResp)
}

void GetLeaderResp::SharedCtor() {
  ::google::protobuf::internal::GetEmptyString();
  _cached_size_ = 0;
  leadername_ = const_cast< ::std::string*>(&::google::protobuf::internal::GetEmptyStringAlreadyInited());
  leaderaddress_ = const_cast< ::std::string*>(&::google::protobuf::internal::GetEmptyStringAlreadyInited());
  ::memset(_has_bits_, 0, sizeof(_has_bits_));
}

GetLeaderResp::~GetLeaderResp() {
  // @@protoc_insertion_point(destructor:tinynet.raft.GetLeaderResp)
  SharedDtor();
}

void GetLeaderResp::SharedDtor() {
  if (leadername_ != &::google::protobuf::internal::GetEmptyStringAlreadyInited()) {
    delete leadername_;
  }
  if (leaderaddress_ != &::google::protobuf::internal::GetEmptyStringAlreadyInited()) {
    delete leaderaddress_;
  }
  if (this != default_instance_) {
  }
}

void GetLeaderResp::SetCachedSize(int size) const {
  GOOGLE_SAFE_CONCURRENT_WRITES_BEGIN();
  _cached_size_ = size;
  GOOGLE_SAFE_CONCURRENT_WRITES_END();
}
const ::google::protobuf::Descriptor* GetLeaderResp::descriptor() {
  protobuf_AssignDescriptorsOnce();
  return GetLeaderResp_descriptor_;
}

const GetLeaderResp& GetLeaderResp::default_instance() {
  if (default_instance_ == NULL) protobuf_AddDesc_cli_2eproto();
  return *default_instance_;
}

GetLeaderResp* GetLeaderResp::default_instance_ = NULL;

GetLeaderResp* GetLeaderResp::New() const {
  return new GetLeaderResp;
}

void GetLeaderResp::Clear() {
  if (_has_bits_[0 / 32] & 3) {
    if (has_leadername()) {
      if (leadername_ != &::google::protobuf::internal::GetEmptyStringAlreadyInited()) {
        leadername_->clear();
      }
    }
    if (has_leaderaddress()) {
      if (leaderaddress_ != &::google::protobuf::internal::GetEmptyStringAlreadyInited()) {
        leaderaddress_->clear();
      }
    }
  }
  ::memset(_has_bits_, 0, sizeof(_has_bits_));
  mutable_unknown_fields()->Clear();
}

bool GetLeaderResp::MergePartialFromCodedStream(
    ::google::protobuf::io::CodedInputStream* input) {
#define DO_(EXPRESSION) if (!(EXPRESSION)) goto failure
  ::google::protobuf::uint32 tag;
  // @@protoc_insertion_point(parse_start:tinynet.raft.GetLeaderResp)
  for (;;) {
    ::std::pair< ::google::protobuf::uint32, bool> p = input->ReadTagWithCutoff(127);
    tag = p.first;
    if (!p.second) goto handle_unusual;
    switch (::google::protobuf::internal::WireFormatLite::GetTagFieldNumber(tag)) {
      // optional string leaderName = 1;
      case 1: {
        if (tag == 10) {
          DO_(::google::protobuf::internal::WireFormatLite::ReadString(
                input, this->mutable_leadername()));
          ::google::protobuf::internal::WireFormat::VerifyUTF8StringNamedField(
            this->leadername().data(), this->leadername().length(),
            ::google::protobuf::internal::WireFormat::PARSE,
            "leadername");
        } else {
          goto handle_unusual;
        }
        if (input->ExpectTag(18)) goto parse_leaderAddress;
        break;
      }

      // optional string leaderAddress = 2;
      case 2: {
        if (tag == 18) {
         parse_leaderAddress:
          DO_(::google::protobuf::internal::WireFormatLite::ReadString(
                input, this->mutable_leaderaddress()));
          ::google::protobuf::internal::WireFormat::VerifyUTF8StringNamedField(
            this->leaderaddress().data(), this->leaderaddress().length(),
            ::google::protobuf::internal::WireFormat::PARSE,
            "leaderaddress");
        } else {
          goto handle_unusual;
        }
        if (input->ExpectAtEnd()) goto success;
        break;
      }

      default: {
      handle_unusual:
        if (tag == 0 ||
            ::google::protobuf::internal::WireFormatLite::GetTagWireType(tag) ==
            ::google::protobuf::internal::WireFormatLite::WIRETYPE_END_GROUP) {
          goto success;
        }
        DO_(::google::protobuf::internal::WireFormat::SkipField(
              input, tag, mutable_unknown_fields()));
        break;
      }
    }
  }
success:
  // @@protoc_insertion_point(parse_success:tinynet.raft.GetLeaderResp)
  return true;
failure:
  // @@protoc_insertion_point(parse_failure:tinynet.raft.GetLeaderResp)
  return false;
#undef DO_
}

void GetLeaderResp::SerializeWithCachedSizes(
    ::google::protobuf::io::CodedOutputStream* output) const {
  // @@protoc_insertion_point(serialize_start:tinynet.raft.GetLeaderResp)
  // optional string leaderName = 1;
  if (has_leadername()) {
    ::google::protobuf::internal::WireFormat::VerifyUTF8StringNamedField(
      this->leadername().data(), this->leadername().length(),
      ::google::protobuf::internal::WireFormat::SERIALIZE,
      "leadername");
    ::google::protobuf::internal::WireFormatLite::WriteStringMaybeAliased(
      1, this->leadername(), output);
  }

  // optional string leaderAddress = 2;
  if (has_leaderaddress()) {
    ::google::protobuf::internal::WireFormat::VerifyUTF8StringNamedField(
      this->leaderaddress().data(), this->leaderaddress().length(),
      ::google::protobuf::internal::WireFormat::SERIALIZE,
      "leaderaddress");
    ::google::protobuf::internal::WireFormatLite::WriteStringMaybeAliased(
      2, this->leaderaddress(), output);
  }

  if (!unknown_fields().empty()) {
    ::google::protobuf::internal::WireFormat::SerializeUnknownFields(
        unknown_fields(), output);
  }
  // @@protoc_insertion_point(serialize_end:tinynet.raft.GetLeaderResp)
}

::google::protobuf::uint8* GetLeaderResp::SerializeWithCachedSizesToArray(
    ::google::protobuf::uint8* target) const {
  // @@protoc_insertion_point(serialize_to_array_start:tinynet.raft.GetLeaderResp)
  // optional string leaderName = 1;
  if (has_leadername()) {
    ::google::protobuf::internal::WireFormat::VerifyUTF8StringNamedField(
      this->leadername().data(), this->leadername().length(),
      ::google::protobuf::internal::WireFormat::SERIALIZE,
      "leadername");
    target =
      ::google::protobuf::internal::WireFormatLite::WriteStringToArray(
        1, this->leadername(), target);
  }

  // optional string leaderAddress = 2;
  if (has_leaderaddress()) {
    ::google::protobuf::internal::WireFormat::VerifyUTF8StringNamedField(
      this->leaderaddress().data(), this->leaderaddress().length(),
      ::google::protobuf::internal::WireFormat::SERIALIZE,
      "leaderaddress");
    target =
      ::google::protobuf::internal::WireFormatLite::WriteStringToArray(
        2, this->leaderaddress(), target);
  }

  if (!unknown_fields().empty()) {
    target = ::google::protobuf::internal::WireFormat::SerializeUnknownFieldsToArray(
        unknown_fields(), target);
  }
  // @@protoc_insertion_point(serialize_to_array_end:tinynet.raft.GetLeaderResp)
  return target;
}

int GetLeaderResp::ByteSize() const {
  int total_size = 0;

  if (_has_bits_[0 / 32] & (0xffu << (0 % 32))) {
    // optional string leaderName = 1;
    if (has_leadername()) {
      total_size += 1 +
        ::google::protobuf::internal::WireFormatLite::StringSize(
          this->leadername());
    }

    // optional string leaderAddress = 2;
    if (has_leaderaddress()) {
      total_size += 1 +
        ::google::protobuf::internal::WireFormatLite::StringSize(
          this->leaderaddress());
    }

  }
  if (!unknown_fields().empty()) {
    total_size +=
      ::google::protobuf::internal::WireFormat::ComputeUnknownFieldsSize(
        unknown_fields());
  }
  GOOGLE_SAFE_CONCURRENT_WRITES_BEGIN();
  _cached_size_ = total_size;
  GOOGLE_SAFE_CONCURRENT_WRITES_END();
  return total_size;
}

void GetLeaderResp::MergeFrom(const ::google::protobuf::Message& from) {
  GOOGLE_CHECK_NE(&from, this);
  const GetLeaderResp* source =
    ::google::protobuf::internal::dynamic_cast_if_available<const GetLeaderResp*>(
      &from);
  if (source == NULL) {
    ::google::protobuf::internal::ReflectionOps::Merge(from, this);
  } else {
    MergeFrom(*source);
  }
}

void GetLeaderResp::MergeFrom(const GetLeaderResp& from) {
  GOOGLE_CHECK_NE(&from, this);
  if (from._has_bits_[0 / 32] & (0xffu << (0 % 32))) {
    if (from.has_leadername()) {
      set_leadername(from.leadername());
    }
    if (from.has_leaderaddress()) {
      set_leaderaddress(from.leaderaddress());
    }
  }
  mutable_unknown_fields()->MergeFrom(from.unknown_fields());
}

void GetLeaderResp::CopyFrom(const ::google::protobuf::Message& from) {
  if (&from == this) return;
  Clear();
  MergeFrom(from);
}

void GetLeaderResp::CopyFrom(const GetLeaderResp& from) {
  if (&from == this) return;
  Clear();
  MergeFrom(from);
}

bool GetLeaderResp::IsInitialized() const {

  return true;
}

void GetLeaderResp::Swap(GetLeaderResp* other) {
  if (other != this) {
    std::swap(leadername_, other->leadername_);
    std::swap(leaderaddress_, other->leaderaddress_);
    std::swap(_has_bits_[0], other->_has_bits_[0]);
    _unknown_fields_.Swap(&other->_unknown_fields_);
    std::swap(_cached_size_, other->_cached_size_);
  }
}

::google::protobuf::Metadata GetLeaderResp::GetMetadata() const {
  protobuf_AssignDescriptorsOnce();
  ::google::protobuf::Metadata metadata;
  metadata.descriptor = GetLeaderResp_descriptor_;
  metadata.reflection = GetLeaderResp_reflection_;
  return metadata;
}


// ===================================================================

RaftCliRpcService::~RaftCliRpcService() {}

const ::google::protobuf::ServiceDescriptor* RaftCliRpcService::descriptor() {
  protobuf_AssignDescriptorsOnce();
  return RaftCliRpcService_descriptor_;
}

const ::google::protobuf::ServiceDescriptor* RaftCliRpcService::GetDescriptor() {
  protobuf_AssignDescriptorsOnce();
  return RaftCliRpcService_descriptor_;
}

void RaftCliRpcService::GetLeader(::google::protobuf::RpcController* controller,
                         const ::tinynet::raft::GetLeaderReq*,
                         ::tinynet::raft::GetLeaderResp*,
                         ::google::protobuf::Closure* done) {
  controller->SetFailed("Method GetLeader() not implemented.");
  done->Run();
}

void RaftCliRpcService::CallMethod(const ::google::protobuf::MethodDescriptor* method,
                             ::google::protobuf::RpcController* controller,
                             const ::google::protobuf::Message* request,
                             ::google::protobuf::Message* response,
                             ::google::protobuf::Closure* done) {
  GOOGLE_DCHECK_EQ(method->service(), RaftCliRpcService_descriptor_);
  switch(method->index()) {
    case 0:
      GetLeader(controller,
             ::google::protobuf::down_cast<const ::tinynet::raft::GetLeaderReq*>(request),
             ::google::protobuf::down_cast< ::tinynet::raft::GetLeaderResp*>(response),
             done);
      break;
    default:
      GOOGLE_LOG(FATAL) << "Bad method index; this should never happen.";
      break;
  }
}

const ::google::protobuf::Message& RaftCliRpcService::GetRequestPrototype(
    const ::google::protobuf::MethodDescriptor* method) const {
  GOOGLE_DCHECK_EQ(method->service(), descriptor());
  switch(method->index()) {
    case 0:
      return ::tinynet::raft::GetLeaderReq::default_instance();
    default:
      GOOGLE_LOG(FATAL) << "Bad method index; this should never happen.";
      return *reinterpret_cast< ::google::protobuf::Message*>(NULL);
  }
}

const ::google::protobuf::Message& RaftCliRpcService::GetResponsePrototype(
    const ::google::protobuf::MethodDescriptor* method) const {
  GOOGLE_DCHECK_EQ(method->service(), descriptor());
  switch(method->index()) {
    case 0:
      return ::tinynet::raft::GetLeaderResp::default_instance();
    default:
      GOOGLE_LOG(FATAL) << "Bad method index; this should never happen.";
      return *reinterpret_cast< ::google::protobuf::Message*>(NULL);
  }
}

RaftCliRpcService_Stub::RaftCliRpcService_Stub(::google::protobuf::RpcChannel* channel)
  : channel_(channel), owns_channel_(false) {}
RaftCliRpcService_Stub::RaftCliRpcService_Stub(
    ::google::protobuf::RpcChannel* channel,
    ::google::protobuf::Service::ChannelOwnership ownership)
  : channel_(channel),
    owns_channel_(ownership == ::google::protobuf::Service::STUB_OWNS_CHANNEL) {}
RaftCliRpcService_Stub::~RaftCliRpcService_Stub() {
  if (owns_channel_) delete channel_;
}

void RaftCliRpcService_Stub::GetLeader(::google::protobuf::RpcController* controller,
                              const ::tinynet::raft::GetLeaderReq* request,
                              ::tinynet::raft::GetLeaderResp* response,
                              ::google::protobuf::Closure* done) {
  channel_->CallMethod(descriptor()->method(0),
                       controller, request, response, done);
}

// @@protoc_insertion_point(namespace_scope)

}  // namespace raft
}  // namespace tinynet

// @@protoc_insertion_point(global_scope)
