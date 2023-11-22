// Generated by the protocol buffer compiler.  DO NOT EDIT!
// source: tdc.proto

#ifndef PROTOBUF_tdc_2eproto__INCLUDED
#define PROTOBUF_tdc_2eproto__INCLUDED

#include <string>

#include <google/protobuf/stubs/common.h>

#if GOOGLE_PROTOBUF_VERSION < 2006000
#error This file was generated by a newer version of protoc which is
#error incompatible with your Protocol Buffer headers.  Please update
#error your headers.
#endif
#if 2006001 < GOOGLE_PROTOBUF_MIN_PROTOC_VERSION
#error This file was generated by an older version of protoc which is
#error incompatible with your Protocol Buffer headers.  Please
#error regenerate this file with a newer version of protoc.
#endif

#include <google/protobuf/generated_message_util.h>
#include <google/protobuf/message.h>
#include <google/protobuf/repeated_field.h>
#include <google/protobuf/extension_set.h>
#include <google/protobuf/service.h>
#include <google/protobuf/unknown_field_set.h>
// @@protoc_insertion_point(includes)

namespace tinynet {
namespace tdc {

// Internal implementation detail -- do not call these.
void  protobuf_AddDesc_tdc_2eproto();
void protobuf_AssignDesc_tdc_2eproto();
void protobuf_ShutdownFile_tdc_2eproto();

class TransferRequest;
class TransferResponse;

// ===================================================================

class TransferRequest : public ::google::protobuf::Message {
 public:
  TransferRequest();
  virtual ~TransferRequest();

  TransferRequest(const TransferRequest& from);

  inline TransferRequest& operator=(const TransferRequest& from) {
    CopyFrom(from);
    return *this;
  }

  inline const ::google::protobuf::UnknownFieldSet& unknown_fields() const {
    return _unknown_fields_;
  }

  inline ::google::protobuf::UnknownFieldSet* mutable_unknown_fields() {
    return &_unknown_fields_;
  }

  static const ::google::protobuf::Descriptor* descriptor();
  static const TransferRequest& default_instance();

  void Swap(TransferRequest* other);

  // implements Message ----------------------------------------------

  TransferRequest* New() const;
  void CopyFrom(const ::google::protobuf::Message& from);
  void MergeFrom(const ::google::protobuf::Message& from);
  void CopyFrom(const TransferRequest& from);
  void MergeFrom(const TransferRequest& from);
  void Clear();
  bool IsInitialized() const;

  int ByteSize() const;
  bool MergePartialFromCodedStream(
      ::google::protobuf::io::CodedInputStream* input);
  void SerializeWithCachedSizes(
      ::google::protobuf::io::CodedOutputStream* output) const;
  ::google::protobuf::uint8* SerializeWithCachedSizesToArray(::google::protobuf::uint8* output) const;
  int GetCachedSize() const { return _cached_size_; }
  private:
  void SharedCtor();
  void SharedDtor();
  void SetCachedSize(int size) const;
  public:
  ::google::protobuf::Metadata GetMetadata() const;

  // nested types ----------------------------------------------------

  // accessors -------------------------------------------------------

  // optional int64 guid = 1;
  inline bool has_guid() const;
  inline void clear_guid();
  static const int kGuidFieldNumber = 1;
  inline ::google::protobuf::int64 guid() const;
  inline void set_guid(::google::protobuf::int64 value);

  // optional bytes body = 2;
  inline bool has_body() const;
  inline void clear_body();
  static const int kBodyFieldNumber = 2;
  inline const ::std::string& body() const;
  inline void set_body(const ::std::string& value);
  inline void set_body(const char* value);
  inline void set_body(const void* value, size_t size);
  inline ::std::string* mutable_body();
  inline ::std::string* release_body();
  inline void set_allocated_body(::std::string* body);

  // @@protoc_insertion_point(class_scope:tinynet.tdc.TransferRequest)
 private:
  inline void set_has_guid();
  inline void clear_has_guid();
  inline void set_has_body();
  inline void clear_has_body();

  ::google::protobuf::UnknownFieldSet _unknown_fields_;

  ::google::protobuf::uint32 _has_bits_[1];
  mutable int _cached_size_;
  ::google::protobuf::int64 guid_;
  ::std::string* body_;
  friend void  protobuf_AddDesc_tdc_2eproto();
  friend void protobuf_AssignDesc_tdc_2eproto();
  friend void protobuf_ShutdownFile_tdc_2eproto();

  void InitAsDefaultInstance();
  static TransferRequest* default_instance_;
};
// -------------------------------------------------------------------

class TransferResponse : public ::google::protobuf::Message {
 public:
  TransferResponse();
  virtual ~TransferResponse();

  TransferResponse(const TransferResponse& from);

  inline TransferResponse& operator=(const TransferResponse& from) {
    CopyFrom(from);
    return *this;
  }

  inline const ::google::protobuf::UnknownFieldSet& unknown_fields() const {
    return _unknown_fields_;
  }

  inline ::google::protobuf::UnknownFieldSet* mutable_unknown_fields() {
    return &_unknown_fields_;
  }

  static const ::google::protobuf::Descriptor* descriptor();
  static const TransferResponse& default_instance();

  void Swap(TransferResponse* other);

  // implements Message ----------------------------------------------

  TransferResponse* New() const;
  void CopyFrom(const ::google::protobuf::Message& from);
  void MergeFrom(const ::google::protobuf::Message& from);
  void CopyFrom(const TransferResponse& from);
  void MergeFrom(const TransferResponse& from);
  void Clear();
  bool IsInitialized() const;

  int ByteSize() const;
  bool MergePartialFromCodedStream(
      ::google::protobuf::io::CodedInputStream* input);
  void SerializeWithCachedSizes(
      ::google::protobuf::io::CodedOutputStream* output) const;
  ::google::protobuf::uint8* SerializeWithCachedSizesToArray(::google::protobuf::uint8* output) const;
  int GetCachedSize() const { return _cached_size_; }
  private:
  void SharedCtor();
  void SharedDtor();
  void SetCachedSize(int size) const;
  public:
  ::google::protobuf::Metadata GetMetadata() const;

  // nested types ----------------------------------------------------

  // accessors -------------------------------------------------------

  // optional int64 guid = 1;
  inline bool has_guid() const;
  inline void clear_guid();
  static const int kGuidFieldNumber = 1;
  inline ::google::protobuf::int64 guid() const;
  inline void set_guid(::google::protobuf::int64 value);

  // optional int32 error_code = 2 [default = -5801];
  inline bool has_error_code() const;
  inline void clear_error_code();
  static const int kErrorCodeFieldNumber = 2;
  inline ::google::protobuf::int32 error_code() const;
  inline void set_error_code(::google::protobuf::int32 value);

  // @@protoc_insertion_point(class_scope:tinynet.tdc.TransferResponse)
 private:
  inline void set_has_guid();
  inline void clear_has_guid();
  inline void set_has_error_code();
  inline void clear_has_error_code();

  ::google::protobuf::UnknownFieldSet _unknown_fields_;

  ::google::protobuf::uint32 _has_bits_[1];
  mutable int _cached_size_;
  ::google::protobuf::int64 guid_;
  ::google::protobuf::int32 error_code_;
  friend void  protobuf_AddDesc_tdc_2eproto();
  friend void protobuf_AssignDesc_tdc_2eproto();
  friend void protobuf_ShutdownFile_tdc_2eproto();

  void InitAsDefaultInstance();
  static TransferResponse* default_instance_;
};
// ===================================================================

class TdcRpcService_Stub;

class TdcRpcService : public ::google::protobuf::Service {
 protected:
  // This class should be treated as an abstract interface.
  inline TdcRpcService() {};
 public:
  virtual ~TdcRpcService();

  typedef TdcRpcService_Stub Stub;

  static const ::google::protobuf::ServiceDescriptor* descriptor();

  virtual void Transfer(::google::protobuf::RpcController* controller,
                       const ::tinynet::tdc::TransferRequest* request,
                       ::tinynet::tdc::TransferResponse* response,
                       ::google::protobuf::Closure* done);

  // implements Service ----------------------------------------------

  const ::google::protobuf::ServiceDescriptor* GetDescriptor();
  void CallMethod(const ::google::protobuf::MethodDescriptor* method,
                  ::google::protobuf::RpcController* controller,
                  const ::google::protobuf::Message* request,
                  ::google::protobuf::Message* response,
                  ::google::protobuf::Closure* done);
  const ::google::protobuf::Message& GetRequestPrototype(
    const ::google::protobuf::MethodDescriptor* method) const;
  const ::google::protobuf::Message& GetResponsePrototype(
    const ::google::protobuf::MethodDescriptor* method) const;

 private:
  GOOGLE_DISALLOW_EVIL_CONSTRUCTORS(TdcRpcService);
};

class TdcRpcService_Stub : public TdcRpcService {
 public:
  TdcRpcService_Stub(::google::protobuf::RpcChannel* channel);
  TdcRpcService_Stub(::google::protobuf::RpcChannel* channel,
                   ::google::protobuf::Service::ChannelOwnership ownership);
  ~TdcRpcService_Stub();

  inline ::google::protobuf::RpcChannel* channel() { return channel_; }

  // implements TdcRpcService ------------------------------------------

  void Transfer(::google::protobuf::RpcController* controller,
                       const ::tinynet::tdc::TransferRequest* request,
                       ::tinynet::tdc::TransferResponse* response,
                       ::google::protobuf::Closure* done);
 private:
  ::google::protobuf::RpcChannel* channel_;
  bool owns_channel_;
  GOOGLE_DISALLOW_EVIL_CONSTRUCTORS(TdcRpcService_Stub);
};


// ===================================================================


// ===================================================================

// TransferRequest

// optional int64 guid = 1;
inline bool TransferRequest::has_guid() const {
  return (_has_bits_[0] & 0x00000001u) != 0;
}
inline void TransferRequest::set_has_guid() {
  _has_bits_[0] |= 0x00000001u;
}
inline void TransferRequest::clear_has_guid() {
  _has_bits_[0] &= ~0x00000001u;
}
inline void TransferRequest::clear_guid() {
  guid_ = GOOGLE_LONGLONG(0);
  clear_has_guid();
}
inline ::google::protobuf::int64 TransferRequest::guid() const {
  // @@protoc_insertion_point(field_get:tinynet.tdc.TransferRequest.guid)
  return guid_;
}
inline void TransferRequest::set_guid(::google::protobuf::int64 value) {
  set_has_guid();
  guid_ = value;
  // @@protoc_insertion_point(field_set:tinynet.tdc.TransferRequest.guid)
}

// optional bytes body = 2;
inline bool TransferRequest::has_body() const {
  return (_has_bits_[0] & 0x00000002u) != 0;
}
inline void TransferRequest::set_has_body() {
  _has_bits_[0] |= 0x00000002u;
}
inline void TransferRequest::clear_has_body() {
  _has_bits_[0] &= ~0x00000002u;
}
inline void TransferRequest::clear_body() {
  if (body_ != &::google::protobuf::internal::GetEmptyStringAlreadyInited()) {
    body_->clear();
  }
  clear_has_body();
}
inline const ::std::string& TransferRequest::body() const {
  // @@protoc_insertion_point(field_get:tinynet.tdc.TransferRequest.body)
  return *body_;
}
inline void TransferRequest::set_body(const ::std::string& value) {
  set_has_body();
  if (body_ == &::google::protobuf::internal::GetEmptyStringAlreadyInited()) {
    body_ = new ::std::string;
  }
  body_->assign(value);
  // @@protoc_insertion_point(field_set:tinynet.tdc.TransferRequest.body)
}
inline void TransferRequest::set_body(const char* value) {
  set_has_body();
  if (body_ == &::google::protobuf::internal::GetEmptyStringAlreadyInited()) {
    body_ = new ::std::string;
  }
  body_->assign(value);
  // @@protoc_insertion_point(field_set_char:tinynet.tdc.TransferRequest.body)
}
inline void TransferRequest::set_body(const void* value, size_t size) {
  set_has_body();
  if (body_ == &::google::protobuf::internal::GetEmptyStringAlreadyInited()) {
    body_ = new ::std::string;
  }
  body_->assign(reinterpret_cast<const char*>(value), size);
  // @@protoc_insertion_point(field_set_pointer:tinynet.tdc.TransferRequest.body)
}
inline ::std::string* TransferRequest::mutable_body() {
  set_has_body();
  if (body_ == &::google::protobuf::internal::GetEmptyStringAlreadyInited()) {
    body_ = new ::std::string;
  }
  // @@protoc_insertion_point(field_mutable:tinynet.tdc.TransferRequest.body)
  return body_;
}
inline ::std::string* TransferRequest::release_body() {
  clear_has_body();
  if (body_ == &::google::protobuf::internal::GetEmptyStringAlreadyInited()) {
    return NULL;
  } else {
    ::std::string* temp = body_;
    body_ = const_cast< ::std::string*>(&::google::protobuf::internal::GetEmptyStringAlreadyInited());
    return temp;
  }
}
inline void TransferRequest::set_allocated_body(::std::string* body) {
  if (body_ != &::google::protobuf::internal::GetEmptyStringAlreadyInited()) {
    delete body_;
  }
  if (body) {
    set_has_body();
    body_ = body;
  } else {
    clear_has_body();
    body_ = const_cast< ::std::string*>(&::google::protobuf::internal::GetEmptyStringAlreadyInited());
  }
  // @@protoc_insertion_point(field_set_allocated:tinynet.tdc.TransferRequest.body)
}

// -------------------------------------------------------------------

// TransferResponse

// optional int64 guid = 1;
inline bool TransferResponse::has_guid() const {
  return (_has_bits_[0] & 0x00000001u) != 0;
}
inline void TransferResponse::set_has_guid() {
  _has_bits_[0] |= 0x00000001u;
}
inline void TransferResponse::clear_has_guid() {
  _has_bits_[0] &= ~0x00000001u;
}
inline void TransferResponse::clear_guid() {
  guid_ = GOOGLE_LONGLONG(0);
  clear_has_guid();
}
inline ::google::protobuf::int64 TransferResponse::guid() const {
  // @@protoc_insertion_point(field_get:tinynet.tdc.TransferResponse.guid)
  return guid_;
}
inline void TransferResponse::set_guid(::google::protobuf::int64 value) {
  set_has_guid();
  guid_ = value;
  // @@protoc_insertion_point(field_set:tinynet.tdc.TransferResponse.guid)
}

// optional int32 error_code = 2 [default = -5801];
inline bool TransferResponse::has_error_code() const {
  return (_has_bits_[0] & 0x00000002u) != 0;
}
inline void TransferResponse::set_has_error_code() {
  _has_bits_[0] |= 0x00000002u;
}
inline void TransferResponse::clear_has_error_code() {
  _has_bits_[0] &= ~0x00000002u;
}
inline void TransferResponse::clear_error_code() {
  error_code_ = -5801;
  clear_has_error_code();
}
inline ::google::protobuf::int32 TransferResponse::error_code() const {
  // @@protoc_insertion_point(field_get:tinynet.tdc.TransferResponse.error_code)
  return error_code_;
}
inline void TransferResponse::set_error_code(::google::protobuf::int32 value) {
  set_has_error_code();
  error_code_ = value;
  // @@protoc_insertion_point(field_set:tinynet.tdc.TransferResponse.error_code)
}


// @@protoc_insertion_point(namespace_scope)

}  // namespace tdc
}  // namespace tinynet

#ifndef SWIG
namespace google {
namespace protobuf {


}  // namespace google
}  // namespace protobuf
#endif  // SWIG

// @@protoc_insertion_point(global_scope)

#endif  // PROTOBUF_tdc_2eproto__INCLUDED
