// Copyright(c) 2015
//
// Permission is hereby granted, free of charge, to any person obtaining a
// copy of this software and associated documentation files (the
// "Software"), to deal in the Software without restriction, including
// without limitation the rights to use, copy, modify, merge, publish,
// distribute, sublicense, and/or sell copies of the Software, and to permit
// persons to whom the Software is furnished to do so, subject to the
// following conditions:
//
// The above copyright notice and this permission notice shall be included
// in all copies or substantial portions of the Software.
//
// THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS
// OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF
// MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN
// NO EVENT SHALL THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM,
// DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR
// OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE
// USE OR OTHER DEALINGS IN THE SOFTWARE
#ifndef SRC_ENV_INL_H_
#define SRC_ENV_INL_H_

#include "v8.h"

#include "cnode.h"
#include "cenv.h"
#include "cutil.h"
#include "cqueue.h"

#include "uv.h"

#include <stdlib.h>

namespace node {
  using v8::Array;
  using v8::ArrayBuffer;
  using v8::Boolean;
  using v8::Context;
  using v8::EscapableHandleScope;
  using v8::Exception;
  using v8::Function;
  using v8::FunctionCallbackInfo;
  using v8::FunctionTemplate;
  using v8::Handle;
  using v8::HandleScope;
  using v8::HeapStatistics;
  using v8::Integer;
  using v8::Isolate;
  using v8::Local;
  using v8::Locker;
  using v8::Message;
  using v8::Number;
  using v8::Object;
  using v8::ObjectTemplate;
  using v8::PropertyCallbackInfo;
  using v8::String;
  using v8::TryCatch;
  using v8::Uint32;
  using v8::V8;
  using v8::Value;
  using v8::kExternalUint32Array;

  v8::Local<v8::Value> BuildStatsObject(Environment* env, const uv_stat_t* s);

  inline Environment::GCInfo::GCInfo()
  : type_(static_cast<GCType>(0)),  flags_(static_cast<GCCallbackFlags>(0)), timestamp_(0) {

  }

  inline Environment::GCInfo::GCInfo(Isolate* isolate, GCType type, GCCallbackFlags flags, uint64_t timestamp)
  : type_(type), flags_(flags), timestamp_(timestamp) {
      isolate->GetHeapStatistics(&stats_);
  }

  inline GCType Environment::GCInfo::type() const {
    return type_;
  }

  inline GCCallbackFlags Environment::GCInfo::flags() const {
    return flags_;
  }

  inline HeapStatistics* Environment::GCInfo::stats() const {
    // TODO(bnoordhuis) Const-ify once https://codereview.chromium.org/63693005
    // lands and makes it way into a stable release.
    return const_cast<HeapStatistics*>(&stats_);
  }

  inline uint64_t Environment::GCInfo::timestamp() const {
    return timestamp_;
  }

  inline Environment::IsolateData* Environment::IsolateData::Get(Isolate* isolate) {
    return static_cast<IsolateData*>(isolate->GetData(kIsolateSlot));
  }

  inline Environment::IsolateData* Environment::IsolateData::GetOrCreate(Isolate* isolate, uv_loop_t* loop) {
    IsolateData* isolate_data = Get(isolate);
    if (isolate_data == NULL) {
      isolate_data = new IsolateData(isolate, loop);
      isolate->SetData(kIsolateSlot, isolate_data);
    }
    isolate_data->ref_count_ += 1;
    return isolate_data;
  }

  inline void Environment::IsolateData::Put() {
    if (--ref_count_ == 0) {
      isolate()->SetData(kIsolateSlot, NULL);
      delete this;
    }
  }

  inline Environment::IsolateData::IsolateData(Isolate* isolate, uv_loop_t* loop)
  : event_loop_(loop), isolate_(isolate),
    #define V(PropertyName, StringValue)                                          \
        PropertyName ## _(isolate, FIXED_ONE_BYTE_STRING(isolate, StringValue)),
        PER_ISOLATE_STRING_PROPERTIES(V)
    #undef V
    ref_count_(0) {
    QUEUE_INIT(&gc_tracker_queue_);
  }

  inline Isolate* Environment::IsolateData::isolate() const {
    return isolate_;
  }

  inline uv_loop_t* Environment::IsolateData::event_loop() const {
    return event_loop_;
  }

  inline Environment* Environment::New(Local<Context> context, uv_loop_t* loop) {
    Environment* env = new Environment(context, loop);
    env->AssignToContext(context);
    return env;
  }

  inline void Environment::AssignToContext(Local<Context> context) {
    context->SetAlignedPointerInEmbedderData(kContextEmbedderDataIndex, this);
  }

  inline Environment* Environment::GetCurrent(Isolate* isolate) {
    return GetCurrent(isolate->GetCurrentContext());
  }

  inline Environment* Environment::GetCurrent(Local<Context> context) {
    return static_cast<Environment*>(
        context->GetAlignedPointerFromEmbedderData(kContextEmbedderDataIndex));
  }

  inline Environment::~Environment() {
    HandleScope handle_scope(isolate());

    context()->SetAlignedPointerInEmbedderData(kContextEmbedderDataIndex, NULL);
    #define V(PropertyName, TypeName) PropertyName ## _.Reset();
    ENVIRONMENT_STRONG_PERSISTENT_PROPERTIES(V)
    #undef V
    isolate_data()->Put();
  }

  inline void Environment::Dispose() {
    delete this;
  }

  inline Isolate* Environment::isolate() const {
    return isolate_;
  }

  inline Environment::IsolateData* Environment::isolate_data() const {
    return isolate_data_;
  }

  inline uv_loop_t* Environment::event_loop() const {
    return isolate_data()->event_loop();
  }

  inline uv_check_t* Environment::immediate_check_handle() {
    return &immediate_check_handle_;
  }

  inline uv_idle_t* Environment::immediate_idle_handle() {
    return &immediate_idle_handle_;
  }

  inline uv_prepare_t* Environment::idle_prepare_handle() {
    return &idle_prepare_handle_;
  }

  inline uv_check_t* Environment::idle_check_handle() {
    return &idle_check_handle_;
  }

  inline Environment::DomainFlag* Environment::domain_flag() {
    return &domain_flag_;
  }

  inline bool Environment::using_domains() const {
    return using_domains_;
  }

  inline void Environment::set_using_domains(bool value) {
    using_domains_ = value;
  }

  inline bool Environment::in_domain() const {
    // The const_cast is okay, it doesn't violate conceptual const-ness.
    return using_domains() && const_cast<Environment*>(this)->domain_flag()->count() > 0;
  }

  inline void Environment::RegisterHandleCleanup(uv_handle_t* handle, HandleCleanupCb cb, void *arg) {
    HandleCleanup* hc = new HandleCleanup(handle, cb, arg);
    QUEUE_INSERT_TAIL(&handle_cleanup_queue_, &hc->handle_cleanup_queue_);
  }

  inline void Environment::FinishHandleCleanup(uv_handle_t* handle) {
    handle_cleanup_waiting_--;
  }

  inline Environment::Environment(Local<Context> context, uv_loop_t* loop)
  : isolate_(context->GetIsolate()),
      isolate_data_(IsolateData::GetOrCreate(context->GetIsolate(), loop)),
      using_smalloc_alloc_cb_(false),
      using_domains_(false),
      using_asyncwrap_(false),
      printed_error_(false),
      //debugger_agent_(this),
      context_(context->GetIsolate(), context) {
    // We'll be creating new objects so make sure we've entered the context.
    v8::HandleScope handle_scope(isolate());
    v8::Context::Scope context_scope(context);
    //TODO::
    set_binding_cache_object(v8::Object::New(isolate()));
    set_module_load_list_array(v8::Array::New(isolate()));
    RB_INIT(&cares_task_list_);
    QUEUE_INIT(&gc_tracker_queue_);
    QUEUE_INIT(&req_wrap_queue_);
    QUEUE_INIT(&handle_wrap_queue_);
    QUEUE_INIT(&handle_cleanup_queue_);
    handle_cleanup_waiting_ = 0;
  }

  inline bool Environment::using_smalloc_alloc_cb() const {
    return using_smalloc_alloc_cb_;
  }

  inline void Environment::set_using_smalloc_alloc_cb(bool value) {
    using_smalloc_alloc_cb_ = value;
  }

  inline bool Environment::using_asyncwrap() const {
    return using_asyncwrap_;
  }

  inline void Environment::set_using_asyncwrap(bool value) {
    using_asyncwrap_ = value;
  }

  inline bool Environment::call_async_init_hook() const {
    // The const_cast is okay, it doesn't violate conceptual const-ness.
    return const_cast<Environment*>(this)->async_hooks()->call_init_hook();
  }

  inline Environment::AsyncHooks* Environment::async_hooks() {
    return &async_hooks_;
  }

  inline Environment* Environment::from_cares_timer_handle(uv_timer_t* handle) {
    return ContainerOf(&Environment::cares_timer_handle_, handle);
  }

  inline uv_timer_t* Environment::cares_timer_handle() {
    return &cares_timer_handle_;
  }

  inline ares_channel Environment::cares_channel() {
    return cares_channel_;
  }

  // Only used in the call to ares_init_options().
  inline ares_channel* Environment::cares_channel_ptr() {
    return &cares_channel_;
  }

  inline ares_task_list* Environment::cares_task_list() {
    return &cares_task_list_;
  }

  //AsyncHooks
  inline Environment::AsyncHooks::AsyncHooks() {
    for (int i = 0; i < kFieldsCount; i++) fields_[i] = 0;
  }

  inline uint32_t* Environment::AsyncHooks::fields() {
    return fields_;
  }

  inline int Environment::AsyncHooks::fields_count() const {
    return kFieldsCount;
  }

  inline bool Environment::AsyncHooks::call_init_hook() {
    return fields_[kCallInitHook] != 0;
  }

  //Class DomainFlag
  inline Environment::DomainFlag::DomainFlag() {
    for (int i = 0; i < kFieldsCount; ++i) fields_[i] = 0;
  }

  inline uint32_t* Environment::DomainFlag::fields() {
    return fields_;
  }

  inline int Environment::DomainFlag::fields_count() const {
    return kFieldsCount;
  }

  inline uint32_t Environment::DomainFlag::count() const {
    return fields_[kCount];
  }

  // this would have been a template function were it not for the fact that g++
  // sometimes fails to resolve it...
  #define THROW_ERROR(fun)                                                      \
    do {                                                                        \
      v8::HandleScope scope(isolate);                                           \
      isolate->ThrowException(fun(OneByteString(isolate, errmsg)));             \
    }                                                                           \
    while (0)

  inline void Environment::ThrowError(Isolate* isolate, const char* errmsg) {
    THROW_ERROR(Exception::Error);
  }

  inline void Environment::ThrowTypeError(Isolate* isolate,  const char* errmsg) {
    THROW_ERROR(Exception::TypeError);
  }

  inline void Environment::ThrowRangeError(Isolate* isolate, const char* errmsg) {
    THROW_ERROR(Exception::RangeError);
  }

  inline void Environment::ThrowError(const char* errmsg) {
    ThrowError(isolate(), errmsg);
  }

  inline void Environment::ThrowTypeError(const char* errmsg) {
    ThrowTypeError(isolate(), errmsg);
  }

  inline void Environment::ThrowRangeError(const char* errmsg) {
    ThrowRangeError(isolate(), errmsg);
  }

  inline void Environment::ThrowErrnoException(int errorno,
    const char* syscall,   const char* message,   const char* path) {
    isolate()->ThrowException(  ErrnoException(isolate(), errorno, syscall, message, path));
  }

  inline void Environment::ThrowUVException(int errorno,
      const char* syscall,  const char* message,  const char* path) {
    isolate()->ThrowException( UVException(isolate(), errorno, syscall, message, path));
  }

  #define V(PropertyName, StringValue)                                          \
    inline Local<String> Environment::IsolateData::PropertyName() const {       \
      /* Strings are immutable so casting away const-ness here is okay. */      \
      return const_cast<IsolateData*>(this)->PropertyName ## _.Get(isolate());  \
    }
    PER_ISOLATE_STRING_PROPERTIES(V)
  #undef V

  #define V(PropertyName, StringValue)                                          \
    inline Local<String> Environment::PropertyName() const {                    \
      return isolate_data()->PropertyName();                                    \
    }
    PER_ISOLATE_STRING_PROPERTIES(V)
  #undef V

  #define V(PropertyName, TypeName)                                             \
    inline v8::Local<TypeName> Environment::PropertyName() const {              \
      return StrongPersistentToLocal(PropertyName ## _);                        \
    }                                                                           \
    inline void Environment::set_ ## PropertyName(v8::Local<TypeName> value) {  \
      PropertyName ## _.Reset(isolate(), value);                                \
    }
    ENVIRONMENT_STRONG_PERSISTENT_PROPERTIES(V)
  #undef V

  #undef ENVIRONMENT_STRONG_PERSISTENT_PROPERTIES
  #undef PER_ISOLATE_STRING_PROPERTIES

} //End Node Namespace

#endif //SRC_ENV_INL_H_
