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

#ifndef SRC_ENV_H_
#define SRC_ENV_H_

#include "cqueue.h"

#include "v8.h"
#include "uv.h"
#include "ares.h"
#include "ctree.h"
// Caveat emptor: we're going slightly crazy with macros here but the end
// hopefully justifies the means. We have a lot of per-context properties
// and adding and maintaining their getters and setters by hand would be
// a nightmare so let's make the preprocessor generate them for us.
//
// Make sure that any macros defined here are undefined again at the bottom
// of context-inl.h. The exceptions are NODE_CONTEXT_EMBEDDER_DATA_INDEX
// and NODE_ISOLATE_SLOT, they may have been defined externally.
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
  using v8::Eternal;
  using v8::GCType;
  using v8::GCCallbackFlags;
  using v8::kExternalUint32Array;

  // Pick an index that's hopefully out of the way when we're embedded inside
  // another application. Performance-wise or memory-wise it doesn't matter:
  // Context::SetAlignedPointerInEmbedderData() is backed by a FixedArray,
  // worst case we pay a one-time penalty for resizing the array.
  #ifndef NODE_CONTEXT_EMBEDDER_DATA_INDEX
  #define NODE_CONTEXT_EMBEDDER_DATA_INDEX 32
  #endif

  // The slot 0 and 1 had already been taken by "gin" and "blink" in Chrome,
  // and the size of isolate's slots is 4 by default, so using 3 should
  // hopefully make node work independently when embedded into other
  // application.
  #ifndef NODE_ISOLATE_SLOT
  #define NODE_ISOLATE_SLOT 3
  #endif

  // Strings are per-isolate primitives but Environment proxies them
  // for the sake of convenience.
  #define PER_ISOLATE_STRING_PROPERTIES(V)                                    \
  V(smalloc_p_string,             "_smalloc_p")                               \
  V(heap_size_limit_string,       "heap_size_limit")                          \
  V(heap_total_string,            "heapTotal")                                \
  V(heap_used_string,             "heapUsed")                                 \
  V(used_heap_size_string,        "used_heap_size")                           \
  V(total_physical_size_string,   "total_physical_size")                      \
  V(total_heap_size_executable_string, "total_heap_size_executable")          \
  V(total_heap_size_string,       "total_heap_size")                          \
  V(flags_string,                 "flags")                                    \
  V(timestamp_string,             "timestamp")                                \
  V(mark_sweep_compact_string,    "mark-sweep-compact")                       \
  V(scavenge_string,              "scavenge")                                 \
  V(type_string,                  "type")                                     \
  V(errno_string,                 "errno")                                    \
  V(code_string,                  "code")                                     \
  V(path_string,                  "path")                                     \
  V(syscall_string,               "syscall")                                  \
  V(exit_string,                  "exit")                                     \
  V(args_string,                  "args")                                     \
  V(address_string,               "address")                                  \
  V(argv_string,                  "argv")                                     \
  V(async,                        "async")                                    \
  V(async_queue_string,           "_asyncQueue")                              \
  V(atime_string,                 "atime")                                    \
  V(birthtime_string,             "birthtime")                                \
  V(blksize_string,               "blksize")                                  \
  V(blocks_string,                "blocks")                                   \
  V(buffer_string,                "buffer")                                   \
  V(bytes_string,                 "bytes")                                    \
  V(bytes_parsed_string,          "bytesParsed")                              \
  V(callback_string,              "callback")                                 \
  V(change_string,                "change")                                   \
  V(close_string,                 "close")                                    \
  V(compare_string,               "compare")                                  \
  V(ctime_string,                 "ctime")                                    \
  V(cwd_string,                   "cwd")                                      \
  V(debug_port_string,            "debugPort")                                \
  V(debug_string,                 "debug")                                    \
  V(detached_string,              "detached")                                 \
  V(dev_string,                   "dev")                                      \
  V(disposed_string,              "_disposed")                                \
  V(domain_string,                "domain")                                   \
  V(exchange_string,              "exchange")                                 \
  V(idle_string,                  "idle")                                     \
  V(irq_string,                   "irq")                                      \
  V(enter_string,                 "enter")                                    \
  V(env_pairs_string,             "envPairs")                                 \
  V(env_string,                   "env")                                      \
  V(error_string,                 "error")                                    \
  V(events_string,                "_events")                                  \
  V(exec_argv_string,             "execArgv")                                 \
  V(exec_path_string,             "execPath")                                 \
  V(exiting_string,               "_exiting")                                 \
  V(exit_code_string,             "exitCode")                                 \
  V(expire_string,                "expire")                                   \
  V(exponent_string,              "exponent")                                 \
  V(exports_string,               "exports")                                  \
  V(ext_key_usage_string,         "ext_key_usage")                            \
  V(family_string,                "family")                                   \
  V(fatal_exception_string,       "_fatalException")                          \
  V(fd_string,                    "fd")                                       \
  V(file_string,                  "file")                                     \
  V(fingerprint_string,           "fingerprint")                              \
  V(fsevent_string,               "FSEvent")                                  \
  V(gid_string,                   "gid")                                      \
  V(handle_string,                "handle")                                   \
  V(headers_string,               "headers")                                  \
  V(hostmaster_string,            "hostmaster")                               \
  V(ignore_string,                "ignore")                                   \
  V(immediate_callback_string,    "_immediateCallback")                       \
  V(infoaccess_string,            "infoAccess")                               \
  V(inherit_string,               "inherit")                                  \
  V(ino_string,                   "ino")                                      \
  V(input_string,                 "input")                                    \
  V(internal_string,              "internal")                                 \
  V(ipv4_string,                  "IPv4")                                     \
  V(ipv6_lc_string,               "ipv6")                                     \
  V(ipv6_string,                  "IPv6")                                     \
  V(issuer_string,                "issuer")                                   \
  V(issuercert_string,            "issuerCertificate")                        \
  V(kill_signal_string,           "killSignal")                               \
  V(mac_string,                   "mac")                                      \
  V(max_buffer_string,            "maxBuffer")                                \
  V(message_string,               "message")                                  \
  V(method_string,                "method")                                   \
  V(minttl_string,                "minttl")                                   \
  V(mode_string,                  "mode")                                     \
  V(model_string,                 "model")                                    \
  V(modulus_string,               "modulus")                                  \
  V(mtime_string,                 "mtime")                                    \
  V(name_string,                  "name")                                     \
  V(need_imm_cb_string,           "_needImmediateCallback")                   \
  V(netmask_string,               "netmask")                                  \
  V(nice_string,                  "nice")                                     \
  V(nlink_string,                 "nlink")                                    \
  V(nsname_string,                "nsname")                                   \
  V(ocsp_request_string,          "OCSPRequest")                              \
  V(offset_string,                "offset")                                   \
  V(onchange_string,              "onchange")                                 \
  V(onclienthello_string,         "onclienthello")                            \
  V(oncomplete_string,            "oncomplete")                               \
  V(onconnection_string,          "onconnection")                             \
  V(ondone_string,                "ondone")                                   \
  V(onerror_string,               "onerror")                                  \
  V(onexit_string,                "onexit")                                   \
  V(onhandshakedone_string,       "onhandshakedone")                          \
  V(onhandshakestart_string,      "onhandshakestart")                         \
  V(onmessage_string,             "onmessage")                                \
  V(onnewsession_string,          "onnewsession")                             \
  V(onnewsessiondone_string,      "onnewsessiondone")                         \
  V(onocspresponse_string,        "onocspresponse")                           \
  V(onread_string,                "onread")                                   \
  V(onselect_string,              "onselect")                                 \
  V(onsignal_string,              "onsignal")                                 \
  V(onstop_string,                "onstop")                                   \
  V(output_string,                "output")                                   \
  V(order_string,                 "order")                                    \
  V(owner_string,                 "owner")                                    \
  V(parse_error_string,           "Parse Error")                              \
  V(pbkdf2_error_string,          "PBKDF2 Error")                             \
  V(pid_string,                   "pid")                                      \
  V(pipe_string,                  "pipe")                                     \
  V(port_string,                  "port")                                     \
  V(preference_string,            "preference")                               \
  V(priority_string,              "priority")                                 \
  V(processed_string,             "processed")                                \
  V(prototype_string,             "prototype")                                \
  V(raw_string,                   "raw")                                      \
  V(rdev_string,                  "rdev")                                     \
  V(readable_string,              "readable")                                 \
  V(received_shutdown_string,     "receivedShutdown")                         \
  V(refresh_string,               "refresh")                                  \
  V(regexp_string,                "regexp")                                   \
  V(rename_string,                "rename")                                   \
  V(replacement_string,           "replacement")                              \
  V(retry_string,                 "retry")                                    \
  V(rss_string,                   "rss")                                      \
  V(serial_string,                "serial")                                   \
  V(scopeid_string,               "scopeid")                                  \
  V(sent_shutdown_string,         "sentShutdown")                             \
  V(serial_number_string,         "serialNumber")                             \
  V(service_string,               "service")                                  \
  V(servername_string,            "servername")                               \
  V(session_id_string,            "sessionId")                                \
  V(should_keep_alive_string,     "shouldKeepAlive")                          \
  V(signal_string,                "signal")                                   \
  V(size_string,                  "size")                                     \
  V(sni_context_err_string,       "Invalid SNI context")                      \
  V(sni_context_string,           "sni_context")                              \
  V(speed_string,                 "speed")                                    \
  V(stack_string,                 "stack")                                    \
  V(status_code_string,           "statusCode")                               \
  V(status_message_string,        "statusMessage")                            \
  V(status_string,                "status")                                   \
  V(stdio_string,                 "stdio")                                    \
  V(subject_string,               "subject")                                  \
  V(subjectaltname_string,        "subjectaltname")                           \
  V(sys_string,                   "sys")                                      \
  V(tick_callback_string,         "_tickCallback")                            \
  V(tick_domain_cb_string,        "_tickDomainCallback")                      \
  V(timeout_string,               "timeout")                                  \
  V(times_string,                 "times")                                    \
  V(title_string,                 "title")                                    \
  V(tls_npn_string,               "tls_npn")                                  \
  V(tls_ocsp_string,              "tls_ocsp")                                 \
  V(tls_sni_string,               "tls_sni")                                  \
  V(tls_string,                   "tls")                                      \
  V(tls_ticket_string,            "tlsTicket")                                \
  V(uid_string,                   "uid")                                      \
  V(unknown_string,               "<unknown>")                                \
  V(upgrade_string,               "upgrade")                                  \
  V(url_string,                   "url")                                      \
  V(user_string,                  "user")                                     \
  V(uv_string,                    "uv")                                       \
  V(valid_from_string,            "valid_from")                               \
  V(valid_to_string,              "valid_to")                                 \
  V(verify_error_string,          "verifyError")                              \
  V(version_major_string,         "versionMajor")                             \
  V(version_minor_string,         "versionMinor")                             \
  V(version_string,               "version")                                  \
  V(weight_string,                "weight")                                   \
  V(windows_verbatim_arguments_string, "windowsVerbatimArguments")            \
  V(wrap_string,                  "wrap")                                     \
  V(writable_string,              "writable")                                 \
  V(write_queue_size_string,      "writeQueueSize")                           \
  V(x_forwarded_string,           "x-forwarded-for")                          \
  V(zero_return_string,           "ZERO_RETURN")                              \

#define ENVIRONMENT_STRONG_PERSISTENT_PROPERTIES(V)                           \
  V(buffer_constructor_function,  Function)                                   \
  V(context,                      Context)                                    \
  V(gc_info_callback_function,    Function)                                   \
  V(process_object,               Object)                                     \
  V(script_context_constructor_template, FunctionTemplate)                    \
  V(script_data_constructor_function, Function)                               \
  V(async_hooks_init_function,    Function)                                   \
  V(async_hooks_pre_function,     Function)                                   \
  V(async_hooks_post_function,    Function)                                   \
  V(binding_cache_object,         Object)                                     \
  V(domain_array,                 Array)                                      \
  V(fs_stats_constructor_function,Function)                                   \
  V(module_load_list_array,       Array)                                      \
  V(pipe_constructor_template,    FunctionTemplate)                           \
  V(secure_context_constructor_template, FunctionTemplate)                    \
  V(tcp_constructor_template,     FunctionTemplate)                           \
  V(tick_callback_function,       Function)                                   \
  V(tls_wrap_constructor_function,Function)                                   \
  V(tty_constructor_template,     FunctionTemplate)                           \
  V(udp_constructor_function,     Function)                                   \


  class Environment;

  struct ares_task_t {
    Environment* env;
    ares_socket_t sock;
    uv_poll_t poll_watcher;
    RB_ENTRY(ares_task_t) node;
  };
  RB_HEAD(ares_task_list, ares_task_t);

  class Environment{
    ///////////////////////
    //PUBLIC EVIRONMENT
    ///////////////////////
  public:

    class AsyncHooks;
    class HandleCleanup;
    class DomainFlag;

    typedef void (*HandleCleanupCb)(Environment* env, uv_handle_t* handle, void* arg);

    static inline Environment* GetCurrent(Isolate* isolate);
    static inline Environment* GetCurrent(Local<Context> context);
    static inline Environment* New(Local<Context> context, uv_loop_t* loop);
    //inline void CleanupHandles();
    inline void Dispose();

    // Defined in src/node_v8.cc.
    void StartGarbageCollectionTracking(Local<Function> callback);
    void StopGarbageCollectionTracking();

    void AssignToContext(Local<Context> context);
    inline v8::Isolate* isolate() const;
    inline uv_loop_t* event_loop() const;
    inline bool in_domain() const;
    inline DomainFlag* domain_flag();
    inline bool using_domains() const;
    inline void set_using_domains(bool value);

    //Called in src/csmalloc.cc.
    inline bool using_smalloc_alloc_cb() const;
    inline void set_using_smalloc_alloc_cb(bool value);

    //Called in src/casync_wrap.cc.
    inline bool using_asyncwrap() const;
    inline void set_using_asyncwrap(bool value);

    //Called in src/casync_wrap.cc.
    inline bool call_async_init_hook() const;
    inline AsyncHooks* async_hooks();

    //Called in src/cares_wrap.cc.
    static inline Environment* from_cares_timer_handle(uv_timer_t* handle);
    inline uv_timer_t* cares_timer_handle();
    inline ares_channel cares_channel();
    inline ares_channel* cares_channel_ptr();
    inline ares_task_list* cares_task_list();

    //Called in src/cnode.cc.
    inline uv_check_t* immediate_check_handle();
    inline uv_idle_t* immediate_idle_handle();
    inline uv_prepare_t* idle_prepare_handle();
    inline uv_check_t* idle_check_handle();
    // Register clean-up cb to be called on env->Dispose()
    inline void RegisterHandleCleanup(uv_handle_t* handle, HandleCleanupCb cb, void *arg);
    inline void FinishHandleCleanup(uv_handle_t* handle);

    inline void ThrowError(const char* errmsg);
    inline void ThrowTypeError(const char* errmsg);
    inline void ThrowRangeError(const char* errmsg);
    inline void ThrowErrnoException(int errorno,
      const char* syscall = NULL,  const char* message = NULL,const char* path = NULL);
    inline void ThrowUVException(int errorno,const
      char* syscall = NULL, const char* message = NULL,const char* path = NULL);

    // Convenience methods for contextify
    inline static void ThrowError(Isolate* isolate, const char* errmsg);
    inline static void ThrowTypeError(Isolate* isolate, const char* errmsg);
    inline static void ThrowRangeError(Isolate* isolate, const char* errmsg);


    // Strings are shared across shared contexts. The getters simply proxy to
    // the per-isolate primitive.
    #define V(PropertyName, StringValue)                                      \
      inline Local<String> PropertyName() const;
      PER_ISOLATE_STRING_PROPERTIES(V)
    #undef V

    #define V(PropertyName, TypeName)                                         \
      inline Local<TypeName> PropertyName() const;                            \
      inline void set_ ## PropertyName(Local<TypeName> value);
      ENVIRONMENT_STRONG_PERSISTENT_PROPERTIES(V)
    #undef V

    inline QUEUE* handle_wrap_queue() { return &handle_wrap_queue_; }
    inline QUEUE* req_wrap_queue() { return &req_wrap_queue_; }

    class AsyncHooks {
     public:
      inline uint32_t* fields();
      inline int fields_count() const;
      inline bool call_init_hook();

     private:
      friend class Environment;  // So we can call the constructor.
      inline AsyncHooks();

      enum Fields {
        // Set this to not zero if the init hook should be called.
        kCallInitHook,
        kFieldsCount
      };

      uint32_t fields_[kFieldsCount];

      DISALLOW_COPY_AND_ASSIGN(AsyncHooks);
    };

    class DomainFlag {
     public:
      inline uint32_t* fields();
      inline int fields_count() const;
      inline uint32_t count() const;

     private:
      friend class Environment;  // So we can call the constructor.
      inline DomainFlag();

      enum Fields {
        kCount,
        kFieldsCount
      };

      uint32_t fields_[kFieldsCount];

      DISALLOW_COPY_AND_ASSIGN(DomainFlag);
    };

    class HandleCleanup {
     private:
      friend class Environment;

      HandleCleanup(uv_handle_t* handle, HandleCleanupCb cb, void* arg)
          : handle_(handle),
            cb_(cb),
            arg_(arg) {
        QUEUE_INIT(&handle_cleanup_queue_);
      }

      uv_handle_t* handle_;
      HandleCleanupCb cb_;
      void* arg_;
      QUEUE handle_cleanup_queue_;
    };
    ///////////////////////
    //PRIVATE EVIRONMENT
    ///////////////////////
  private:
    class GCInfo;
    class IsolateData;

    static const int kIsolateSlot = NODE_ISOLATE_SLOT;
    inline Environment(Local<Context> context, uv_loop_t* loop);
    inline ~Environment();
    inline IsolateData* isolate_data() const;
    void AfterGarbageCollectionCallback( const GCInfo* before,  const GCInfo* after);

    enum ContextEmbedderDataIndex {
      kContextEmbedderDataIndex = NODE_CONTEXT_EMBEDDER_DATA_INDEX
    };

    Isolate* const isolate_;
    IsolateData* const isolate_data_;
    bool using_smalloc_alloc_cb_;
    bool using_asyncwrap_;
    QUEUE gc_tracker_queue_;
    bool printed_error_;
    //debugger::Agent debugger_agent_;
    AsyncHooks async_hooks_;

    //uv
    uv_check_t immediate_check_handle_;
    uv_idle_t immediate_idle_handle_;
    uv_prepare_t idle_prepare_handle_;
    uv_check_t idle_check_handle_;

    QUEUE handle_wrap_queue_;
    QUEUE req_wrap_queue_;
    QUEUE handle_cleanup_queue_;
    int handle_cleanup_waiting_;

    //ares
    uv_timer_t cares_timer_handle_;
    ares_channel cares_channel_;
    ares_task_list cares_task_list_;

    //domain
    DomainFlag domain_flag_;
    bool using_domains_;

    #define V(PropertyName, TypeName)                                         \
      v8::Persistent<TypeName> PropertyName ## _;
      ENVIRONMENT_STRONG_PERSISTENT_PROPERTIES(V)
    #undef V

    class GCInfo {
       public:
        inline GCInfo();
        inline GCInfo(Isolate* isolate, GCType type, GCCallbackFlags flags, uint64_t timestamp);
        inline GCType type() const;
        inline GCCallbackFlags flags() const;
        // TODO(bnoordhuis) Const-ify once https://codereview.chromium.org/63693005
        // lands and makes it way into a stable release.
        inline HeapStatistics* stats() const;
        inline uint64_t timestamp() const;
       private:
        GCType type_;
        GCCallbackFlags flags_;
        HeapStatistics stats_;
        uint64_t timestamp_;
    }; //End Clas GCInfo

    // Per-thread, reference-counted singleton.
    class IsolateData {
     public:
      static inline IsolateData* GetOrCreate(Isolate* isolate, uv_loop_t* loop);
      inline void Put();
      inline uv_loop_t* event_loop() const;

      // Defined in src/node_v8.cc.
      void StartGarbageCollectionTracking(Environment* env);
      void StopGarbageCollectionTracking(Environment* env);

      #define V(PropertyName, StringValue)                                    \
        inline Local<String> PropertyName() const;
        PER_ISOLATE_STRING_PROPERTIES(V)
      #undef V

     private:
      inline static IsolateData* Get(Isolate* isolate);
      inline explicit IsolateData(Isolate* isolate, uv_loop_t* loop);
      inline Isolate* isolate() const;

      // Defined in src/node_v8.cc.
      static void BeforeGarbageCollection(Isolate* isolate, GCType type, GCCallbackFlags flags);
      static void AfterGarbageCollection(Isolate* isolate, GCType type, GCCallbackFlags flags);
      void BeforeGarbageCollection(GCType type, GCCallbackFlags flags);
      void AfterGarbageCollection(GCType type, GCCallbackFlags flags);

      uv_loop_t* const event_loop_;
      Isolate* const isolate_;

      #define V(PropertyName, StringValue)                                    \
        Eternal<String> PropertyName ## _;
        PER_ISOLATE_STRING_PROPERTIES(V)
      #undef V

      unsigned int ref_count_;
      QUEUE gc_tracker_queue_;
      GCInfo gc_info_before_;
      GCInfo gc_info_after_;

      DISALLOW_COPY_AND_ASSIGN(IsolateData);
    };//End Class IsolateData

  }; //End Class Environment

} //End Node Namespace
#endif  // SRC_ENV_H_
