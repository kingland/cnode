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
#include "cnode.h"
#include "cnode_internal.h"
#include "cutil.h"
#include "cutil-inl.h"
#include "cenv.h"
#include "cenv-inl.h"
#include "cnode_js.h"
#include "cstring_bytes.h"

#include "v8.h"
#include "uv.h"

#include <stdlib.h>
#include <string.h>

#define DEBUG_PORT   5858

/*#if defined(_MSC_VER)
#include <direct.h>
#include <io.h>
#include <process.h>
#define strcasecmp _stricmp
#define getpid _getpid
#define umask _umask
typedef int mode_t;
#else
#include <sys/resource.h>  // getrlimit, setrlimit
#include <unistd.h>  // setuid, getuid
#endif

#if defined(__POSIX__) && !defined(__ANDROID__)
#include <pwd.h>  // getpwnam()
#include <grp.h>  // getgrnam()
#endif

#ifdef __APPLE__
#include <crt_externs.h>
#define environ (*_NSGetEnviron())
#elif !defined(_MSC_VER)
extern char **environ;
#endif*/

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

  static int debug_port = DEBUG_PORT;

  static bool print_eval = false;
  static bool force_repl = false;
  static bool trace_deprecation = false;
  static bool throw_deprecation = false;
  static const char* eval_string = NULL;
  static bool use_debug_agent = false;
  static bool debug_wait_connect = false;
  static bool v8_is_profiling = false;
  static bool node_is_initialized = false;

  static Isolate* node_isolate = NULL;

  //modules
  static node_module* modpending;
  static node_module* modlist_builtin;
  static node_module* modlist_linked;
  static node_module* modlist_addon;

  //case UTF8 in src/string_byte.cc
  int WRITE_UTF8_FLAGS = v8::String::HINT_MANY_WRITES_EXPECTED |
                       v8::String::NO_NULL_TERMINATION;

  // used by C++ modules as well
  bool no_deprecation = false;


static inline const char *errno_string(int errorno) {
  #define ERRNO_CASE(e)  case e: return #e;
    switch (errorno) {
  #ifdef EACCES
    ERRNO_CASE(EACCES);
  #endif

  #ifdef EADDRINUSE
    ERRNO_CASE(EADDRINUSE);
  #endif

  #ifdef EADDRNOTAVAIL
    ERRNO_CASE(EADDRNOTAVAIL);
  #endif

  #ifdef EAFNOSUPPORT
    ERRNO_CASE(EAFNOSUPPORT);
  #endif

  #ifdef EAGAIN
    ERRNO_CASE(EAGAIN);
  #endif

  #ifdef EWOULDBLOCK
  # if EAGAIN != EWOULDBLOCK
    ERRNO_CASE(EWOULDBLOCK);
  # endif
  #endif

  #ifdef EALREADY
    ERRNO_CASE(EALREADY);
  #endif

  #ifdef EBADF
    ERRNO_CASE(EBADF);
  #endif

  #ifdef EBADMSG
    ERRNO_CASE(EBADMSG);
  #endif

  #ifdef EBUSY
    ERRNO_CASE(EBUSY);
  #endif

  #ifdef ECANCELED
    ERRNO_CASE(ECANCELED);
  #endif

  #ifdef ECHILD
    ERRNO_CASE(ECHILD);
  #endif

  #ifdef ECONNABORTED
    ERRNO_CASE(ECONNABORTED);
  #endif

  #ifdef ECONNREFUSED
    ERRNO_CASE(ECONNREFUSED);
  #endif

  #ifdef ECONNRESET
    ERRNO_CASE(ECONNRESET);
  #endif

  #ifdef EDEADLK
    ERRNO_CASE(EDEADLK);
  #endif

  #ifdef EDESTADDRREQ
    ERRNO_CASE(EDESTADDRREQ);
  #endif

  #ifdef EDOM
    ERRNO_CASE(EDOM);
  #endif

  #ifdef EDQUOT
    ERRNO_CASE(EDQUOT);
  #endif

  #ifdef EEXIST
    ERRNO_CASE(EEXIST);
  #endif

  #ifdef EFAULT
    ERRNO_CASE(EFAULT);
  #endif

  #ifdef EFBIG
    ERRNO_CASE(EFBIG);
  #endif

  #ifdef EHOSTUNREACH
    ERRNO_CASE(EHOSTUNREACH);
  #endif

  #ifdef EIDRM
    ERRNO_CASE(EIDRM);
  #endif

  #ifdef EILSEQ
    ERRNO_CASE(EILSEQ);
  #endif

  #ifdef EINPROGRESS
    ERRNO_CASE(EINPROGRESS);
  #endif

  #ifdef EINTR
    ERRNO_CASE(EINTR);
  #endif

  #ifdef EINVAL
    ERRNO_CASE(EINVAL);
  #endif

  #ifdef EIO
    ERRNO_CASE(EIO);
  #endif

  #ifdef EISCONN
    ERRNO_CASE(EISCONN);
  #endif

  #ifdef EISDIR
    ERRNO_CASE(EISDIR);
  #endif

  #ifdef ELOOP
    ERRNO_CASE(ELOOP);
  #endif

  #ifdef EMFILE
    ERRNO_CASE(EMFILE);
  #endif

  #ifdef EMLINK
    ERRNO_CASE(EMLINK);
  #endif

  #ifdef EMSGSIZE
    ERRNO_CASE(EMSGSIZE);
  #endif

  #ifdef EMULTIHOP
    ERRNO_CASE(EMULTIHOP);
  #endif

  #ifdef ENAMETOOLONG
    ERRNO_CASE(ENAMETOOLONG);
  #endif

  #ifdef ENETDOWN
    ERRNO_CASE(ENETDOWN);
  #endif

  #ifdef ENETRESET
    ERRNO_CASE(ENETRESET);
  #endif

  #ifdef ENETUNREACH
    ERRNO_CASE(ENETUNREACH);
  #endif

  #ifdef ENFILE
    ERRNO_CASE(ENFILE);
  #endif

  #ifdef ENOBUFS
    ERRNO_CASE(ENOBUFS);
  #endif

  #ifdef ENODATA
    ERRNO_CASE(ENODATA);
  #endif

  #ifdef ENODEV
    ERRNO_CASE(ENODEV);
  #endif

  #ifdef ENOENT
    ERRNO_CASE(ENOENT);
  #endif

  #ifdef ENOEXEC
    ERRNO_CASE(ENOEXEC);
  #endif

  #ifdef ENOLINK
    ERRNO_CASE(ENOLINK);
  #endif

  #ifdef ENOLCK
  # if ENOLINK != ENOLCK
    ERRNO_CASE(ENOLCK);
  # endif
  #endif

  #ifdef ENOMEM
    ERRNO_CASE(ENOMEM);
  #endif

  #ifdef ENOMSG
    ERRNO_CASE(ENOMSG);
  #endif

  #ifdef ENOPROTOOPT
    ERRNO_CASE(ENOPROTOOPT);
  #endif

  #ifdef ENOSPC
    ERRNO_CASE(ENOSPC);
  #endif

  #ifdef ENOSR
    ERRNO_CASE(ENOSR);
  #endif

  #ifdef ENOSTR
    ERRNO_CASE(ENOSTR);
  #endif

  #ifdef ENOSYS
    ERRNO_CASE(ENOSYS);
  #endif

  #ifdef ENOTCONN
    ERRNO_CASE(ENOTCONN);
  #endif

  #ifdef ENOTDIR
    ERRNO_CASE(ENOTDIR);
  #endif

  #ifdef ENOTEMPTY
    ERRNO_CASE(ENOTEMPTY);
  #endif

  #ifdef ENOTSOCK
    ERRNO_CASE(ENOTSOCK);
  #endif

  #ifdef ENOTSUP
    ERRNO_CASE(ENOTSUP);
  #else
  # ifdef EOPNOTSUPP
    ERRNO_CASE(EOPNOTSUPP);
  # endif
  #endif

  #ifdef ENOTTY
    ERRNO_CASE(ENOTTY);
  #endif

  #ifdef ENXIO
    ERRNO_CASE(ENXIO);
  #endif


  #ifdef EOVERFLOW
    ERRNO_CASE(EOVERFLOW);
  #endif

  #ifdef EPERM
    ERRNO_CASE(EPERM);
  #endif

  #ifdef EPIPE
    ERRNO_CASE(EPIPE);
  #endif

  #ifdef EPROTO
    ERRNO_CASE(EPROTO);
  #endif

  #ifdef EPROTONOSUPPORT
    ERRNO_CASE(EPROTONOSUPPORT);
  #endif

  #ifdef EPROTOTYPE
    ERRNO_CASE(EPROTOTYPE);
  #endif

  #ifdef ERANGE
    ERRNO_CASE(ERANGE);
  #endif

  #ifdef EROFS
    ERRNO_CASE(EROFS);
  #endif

  #ifdef ESPIPE
    ERRNO_CASE(ESPIPE);
  #endif

  #ifdef ESRCH
    ERRNO_CASE(ESRCH);
  #endif

  #ifdef ESTALE
    ERRNO_CASE(ESTALE);
  #endif

  #ifdef ETIME
    ERRNO_CASE(ETIME);
  #endif

  #ifdef ETIMEDOUT
    ERRNO_CASE(ETIMEDOUT);
  #endif

  #ifdef ETXTBSY
    ERRNO_CASE(ETXTBSY);
  #endif

  #ifdef EXDEV
    ERRNO_CASE(EXDEV);
  #endif

    default: return "";
    }
  }


  const char *signo_string(int signo) {
  #define SIGNO_CASE(e)  case e: return #e;
    switch (signo) {
  #ifdef SIGHUP
    SIGNO_CASE(SIGHUP);
  #endif

  #ifdef SIGINT
    SIGNO_CASE(SIGINT);
  #endif

  #ifdef SIGQUIT
    SIGNO_CASE(SIGQUIT);
  #endif

  #ifdef SIGILL
    SIGNO_CASE(SIGILL);
  #endif

  #ifdef SIGTRAP
    SIGNO_CASE(SIGTRAP);
  #endif

  #ifdef SIGABRT
    SIGNO_CASE(SIGABRT);
  #endif

  #ifdef SIGIOT
  # if SIGABRT != SIGIOT
    SIGNO_CASE(SIGIOT);
  # endif
  #endif

  #ifdef SIGBUS
    SIGNO_CASE(SIGBUS);
  #endif

  #ifdef SIGFPE
    SIGNO_CASE(SIGFPE);
  #endif

  #ifdef SIGKILL
    SIGNO_CASE(SIGKILL);
  #endif

  #ifdef SIGUSR1
    SIGNO_CASE(SIGUSR1);
  #endif

  #ifdef SIGSEGV
    SIGNO_CASE(SIGSEGV);
  #endif

  #ifdef SIGUSR2
    SIGNO_CASE(SIGUSR2);
  #endif

  #ifdef SIGPIPE
    SIGNO_CASE(SIGPIPE);
  #endif

  #ifdef SIGALRM
    SIGNO_CASE(SIGALRM);
  #endif

  #ifdef SIGTERM
    SIGNO_CASE(SIGTERM);
  #endif

  #ifdef SIGCHLD
    SIGNO_CASE(SIGCHLD);
  #endif

  #ifdef SIGSTKFLT
    SIGNO_CASE(SIGSTKFLT);
  #endif


  #ifdef SIGCONT
    SIGNO_CASE(SIGCONT);
  #endif

  #ifdef SIGSTOP
    SIGNO_CASE(SIGSTOP);
  #endif

  #ifdef SIGTSTP
    SIGNO_CASE(SIGTSTP);
  #endif

  #ifdef SIGBREAK
    SIGNO_CASE(SIGBREAK);
  #endif

  #ifdef SIGTTIN
    SIGNO_CASE(SIGTTIN);
  #endif

  #ifdef SIGTTOU
    SIGNO_CASE(SIGTTOU);
  #endif

  #ifdef SIGURG
    SIGNO_CASE(SIGURG);
  #endif

  #ifdef SIGXCPU
    SIGNO_CASE(SIGXCPU);
  #endif

  #ifdef SIGXFSZ
    SIGNO_CASE(SIGXFSZ);
  #endif

  #ifdef SIGVTALRM
    SIGNO_CASE(SIGVTALRM);
  #endif

  #ifdef SIGPROF
    SIGNO_CASE(SIGPROF);
  #endif

  #ifdef SIGWINCH
    SIGNO_CASE(SIGWINCH);
  #endif

  #ifdef SIGIO
    SIGNO_CASE(SIGIO);
  #endif

  #ifdef SIGPOLL
  # if SIGPOLL != SIGIO
    SIGNO_CASE(SIGPOLL);
  # endif
  #endif

  #ifdef SIGLOST
    SIGNO_CASE(SIGLOST);
  #endif

  #ifdef SIGPWR
  # if SIGPWR != SIGLOST
    SIGNO_CASE(SIGPWR);
  # endif
  #endif

  #ifdef SIGSYS
    SIGNO_CASE(SIGSYS);
  #endif
    default: return "";
    }
  }


  //Class defined
  class ArrayBufferAllocator : public ArrayBuffer::Allocator {
   public:
    // Impose an upper limit to avoid out of memory errors that bring down
    // the process.
    static const size_t kMaxLength = 0x3fffffff;
    static ArrayBufferAllocator the_singleton;
    virtual ~ArrayBufferAllocator() {}
    virtual void* Allocate(size_t length);
    virtual void* AllocateUninitialized(size_t length);
    virtual void Free(void* data, size_t length);
   private:
    ArrayBufferAllocator() {}
    ArrayBufferAllocator(const ArrayBufferAllocator&);
    void operator=(const ArrayBufferAllocator&);
  };

  ArrayBufferAllocator ArrayBufferAllocator::the_singleton;

  void* ArrayBufferAllocator::Allocate(size_t length) {
    if (length > kMaxLength)
      return NULL;
    char* data = new char[length];
    memset(data, 0, length);
    return data;
  }

  void* ArrayBufferAllocator::AllocateUninitialized(size_t length) {
    if (length > kMaxLength)
      return NULL;
    return new char[length];
  }

  void ArrayBufferAllocator::Free(void* data, size_t length) {
    delete[] static_cast<char*>(data);
  }
  //End Class


  //ERROR
  static void OnFatalError(const char* location, const char* message) {
    if (location) {
      fprintf(stderr, "FATAL ERROR: %s %s\n", location, message);
    } else {
      fprintf(stderr, "FATAL ERROR: %s\n", message);
    }
    fflush(stderr);
    //abort();
  }

  //NO_RETURN
  void FatalError(const char* location, const char* message) {
    OnFatalError(location, message);
    // to supress compiler warning
    //abort();
  }

  void FatalException(Isolate* isolate, const TryCatch& try_catch) {
    HandleScope scope(isolate);
    // TODO(bajtos) do not call FatalException if try_catch is verbose
    // (requires V8 API to expose getter for try_catch.is_verbose_
    //TODO ::
    //FatalException(isolate, try_catch.Exception(), try_catch.Message());
  }


  void OnMessage(Handle<Message> message, Handle<Value> error) {
    // The current version of V8 sends messages for errors only
    // (thus `error` is always set).
    //TODO ::
    //FatalException(Isolate::GetCurrent(), error, message);
  }

  static void ReportException(Environment* env, Handle<Value> er, Handle<Message> message) {
    /*HandleScope scope(env->isolate());

    AppendExceptionLine(env, er, message);

    Local<Value> trace_value;

    if (er->IsUndefined() || er->IsNull())
      trace_value = Undefined(env->isolate());
    else
      trace_value = er->ToObject()->Get(env->stack_string());

    node::Utf8Value trace(trace_value);

    // range errors have a trace member set to undefined
    if (trace.length() > 0 && !trace_value->IsUndefined()) {
      fprintf(stderr, "%s\n", *trace);
    } else {
      // this really only happens for RangeErrors, since they're the only
      // kind that won't have all this info in the trace, or when non-Error
      // objects are thrown manually.
      Local<Value> message;
      Local<Value> name;

      if (er->IsObject()) {
        Local<Object> err_obj = er.As<Object>();
        message = err_obj->Get(env->message_string());
        name = err_obj->Get(FIXED_ONE_BYTE_STRING(env->isolate(), "name"));
      }

      if (message.IsEmpty() ||
          message->IsUndefined() ||
          name.IsEmpty() ||
          name->IsUndefined()) {
        // Not an error object. Just print as-is.
        node::Utf8Value message(er);
        fprintf(stderr, "%s\n", *message);
      } else {
        node::Utf8Value name_string(name);
        node::Utf8Value message_string(message);
        fprintf(stderr, "%s: %s\n", *name_string, *message_string);
      }
    }

    fflush(stderr);*/
  }


  static void ReportException(Environment* env, const TryCatch& try_catch) {
    ReportException(env, try_catch.Exception(), try_catch.Message());
  }


  Local<Value> ErrnoException(Isolate* isolate, int errorno, const char *syscall, const char *msg, const char *path) {
    Environment* env = Environment::GetCurrent(isolate);

    Local<Value> e;
    Local<String> estring = OneByteString(env->isolate(), errno_string(errorno));
    if (msg == NULL || msg[0] == '\0') {
      msg = strerror(errorno);
    }
    Local<String> message = OneByteString(env->isolate(), msg);

    Local<String> cons1 =
        String::Concat(estring, FIXED_ONE_BYTE_STRING(env->isolate(), ", "));
    Local<String> cons2 = String::Concat(cons1, message);

    if (path) {
      Local<String> cons3 =
          String::Concat(cons2, FIXED_ONE_BYTE_STRING(env->isolate(), " '"));
      Local<String> cons4 =
          String::Concat(cons3, String::NewFromUtf8(env->isolate(), path));
      Local<String> cons5 =
          String::Concat(cons4, FIXED_ONE_BYTE_STRING(env->isolate(), "'"));
      e = Exception::Error(cons5);
    } else {
      e = Exception::Error(cons2);
    }

    Local<Object> obj = e->ToObject();
    obj->Set(env->errno_string(), Integer::New(env->isolate(), errorno));
    obj->Set(env->code_string(), estring);

    if (path != NULL) {
      obj->Set(env->path_string(), String::NewFromUtf8(env->isolate(), path));
    }

    if (syscall != NULL) {
      obj->Set(env->syscall_string(), OneByteString(env->isolate(), syscall));
    }

    return e;
  }


  // hack alert! copy of ErrnoException, tuned for uv errors
  Local<Value> UVException(Isolate* isolate, int errorno,
    const char *syscall, const char *msg, const char *path) {
    Environment* env = Environment::GetCurrent(isolate);

    if (!msg || !msg[0])
      msg = uv_strerror(errorno);

    Local<String> estring = OneByteString(env->isolate(), uv_err_name(errorno));
    Local<String> message = OneByteString(env->isolate(), msg);
    Local<String> cons1 =
        String::Concat(estring, FIXED_ONE_BYTE_STRING(env->isolate(), ", "));
    Local<String> cons2 = String::Concat(cons1, message);

    Local<Value> e;

    Local<String> path_str;

    if (path) {
  #ifdef _WIN32
      if (strncmp(path, "\\\\?\\UNC\\", 8) == 0) {
        path_str = String::Concat(FIXED_ONE_BYTE_STRING(env->isolate(), "\\\\"),
                                  String::NewFromUtf8(env->isolate(), path + 8));
      } else if (strncmp(path, "\\\\?\\", 4) == 0) {
        path_str = String::NewFromUtf8(env->isolate(), path + 4);
      } else {
        path_str = String::NewFromUtf8(env->isolate(), path);
      }
  #else
      path_str = String::NewFromUtf8(env->isolate(), path);
  #endif

      Local<String> cons3 =
          String::Concat(cons2, FIXED_ONE_BYTE_STRING(env->isolate(), " '"));
      Local<String> cons4 =
          String::Concat(cons3, path_str);
      Local<String> cons5 =
          String::Concat(cons4, FIXED_ONE_BYTE_STRING(env->isolate(), "'"));
      e = Exception::Error(cons5);
    } else {
      e = Exception::Error(cons2);
    }

    Local<Object> obj = e->ToObject();
    // TODO(piscisaureus) errno should probably go
    obj->Set(env->errno_string(), Integer::New(env->isolate(), errorno));
    obj->Set(env->code_string(), estring);

    if (path != NULL) {
      obj->Set(env->path_string(), path_str);
    }

    if (syscall != NULL) {
      obj->Set(env->syscall_string(), OneByteString(env->isolate(), syscall));
    }

    return e;
  }


  //TODO: AppendExceptionLine
  void AppendExceptionLine(Environment* env,  Handle<Value> er, Handle<Message> message) {
    if (message.IsEmpty())
      return;

    /*HandleScope scope(env->isolate());
    Local<Object> err_obj;
    if (!er.IsEmpty() && er->IsObject()) {
      err_obj = er.As<Object>();

      // Do it only once per message
      if (!err_obj->GetHiddenValue(env->processed_string()).IsEmpty())
        return;
      err_obj->SetHiddenValue(env->processed_string(), True(env->isolate()));
    }

    static char arrow[1024];

    // Print (filename):(line number): (message).
    node::Utf8Value filename(message->GetScriptResourceName());
    const char* filename_string = *filename;
    int linenum = message->GetLineNumber();
    // Print line of source code.
    node::Utf8Value sourceline(message->GetSourceLine());
    const char* sourceline_string = *sourceline;

    // Because of how node modules work, all scripts are wrapped with a
    // "function (module, exports, __filename, ...) {"
    // to provide script local variables.
    //
    // When reporting errors on the first line of a script, this wrapper
    // function is leaked to the user. There used to be a hack here to
    // truncate off the first 62 characters, but it caused numerous other
    // problems when vm.runIn*Context() methods were used for non-module
    // code.
    //
    // If we ever decide to re-instate such a hack, the following steps
    // must be taken:
    //
    // 1. Pass a flag around to say "this code was wrapped"
    // 2. Update the stack frame output so that it is also correct.
    //
    // It would probably be simpler to add a line rather than add some
    // number of characters to the first line, since V8 truncates the
    // sourceline to 78 characters, and we end up not providing very much
    // useful debugging info to the user if we remove 62 characters.

    int start = message->GetStartColumn();
    int end = message->GetEndColumn();

    int off = snprintf(arrow,
                       sizeof(arrow),
                       "%s:%i\n%s\n",
                       filename_string,
                       linenum,
                       sourceline_string);
    assert(off >= 0);

    // Print wavy underline (GetUnderline is deprecated).
    for (int i = 0; i < start; i++) {
      if (sourceline_string[i] == '\0' ||
          static_cast<size_t>(off) >= sizeof(arrow)) {
        break;
      }
      assert(static_cast<size_t>(off) < sizeof(arrow));
      arrow[off++] = (sourceline_string[i] == '\t') ? '\t' : ' ';
    }
    for (int i = start; i < end; i++) {
      if (sourceline_string[i] == '\0' ||
          static_cast<size_t>(off) >= sizeof(arrow)) {
        break;
      }
      assert(static_cast<size_t>(off) < sizeof(arrow));
      arrow[off++] = '^';
    }
    assert(static_cast<size_t>(off - 1) <= sizeof(arrow) - 1);
    arrow[off++] = '\n';
    arrow[off] = '\0';

    Local<String> arrow_str = String::NewFromUtf8(env->isolate(), arrow);
    Local<Value> msg;
    Local<Value> stack;

    // Allocation failed, just print it out
    if (arrow_str.IsEmpty() || err_obj.IsEmpty() || !err_obj->IsNativeError())
      goto print;

    msg = err_obj->Get(env->message_string());
    stack = err_obj->Get(env->stack_string());

    if (msg.IsEmpty() || stack.IsEmpty())
      goto print;

    err_obj->Set(env->message_string(),
                 String::Concat(arrow_str, msg->ToString()));
    err_obj->Set(env->stack_string(),
                 String::Concat(arrow_str, stack->ToString()));
    return;

   print:
    if (env->printed_error())
      return;
    env->set_printed_error(true);
    uv_tty_reset_mode();
    fprintf(stderr, "\n%s", arrow);*/
  } //End Function AppendExceptionLine


  Handle<Value> MakeCallback(Environment* env,  Handle<Value> recv, const Handle<Function> callback, int argc,  Handle<Value> argv[]) {
    // If you hit this assertion, you forgot to enter the v8::Context first.
    CHECK(env->context() == env->isolate()->GetCurrentContext());

    Local<Object> process = env->process_object();
    Local<Object> object, domain;
    bool has_async_queue = false;
    bool has_domain = false;

    if (recv->IsObject()) {
      object = recv.As<Object>();
      Local<Value> async_queue_v = object->Get(env->async_queue_string());
      if (async_queue_v->IsObject())
        has_async_queue = true;
    }

    //TODO::
    /*if (env->using_domains()) {
      CHECK(recv->IsObject());
      Local<Value> domain_v = object->Get(env->domain_string());
      has_domain = domain_v->IsObject();
      if (has_domain) {
        domain = domain_v.As<Object>();
        if (domain->Get(env->disposed_string())->IsTrue())
          return Undefined(env->isolate());
      }
    }*/

    TryCatch try_catch;
    try_catch.SetVerbose(true);

    /*if (has_domain) {
      Local<Value> enter_v = domain->Get(env->enter_string());
      if (enter_v->IsFunction()) {
        enter_v.As<Function>()->Call(domain, 0, NULL);
        if (try_catch.HasCaught())
          return Undefined(env->isolate());
      }
    }

    if (has_async_queue) {
      try_catch.SetVerbose(false);
      env->async_hooks_pre_function()->Call(object, 0, NULL);
      if (try_catch.HasCaught())
        FatalError("node:;MakeCallback", "pre hook threw");
      try_catch.SetVerbose(true);
    }*/

    Local<Value> ret = callback->Call(recv, argc, argv);

    /*if (has_async_queue) {
      try_catch.SetVerbose(false);
      env->async_hooks_post_function()->Call(object, 0, NULL);
      if (try_catch.HasCaught())
        FatalError("node::MakeCallback", "post hook threw");
      try_catch.SetVerbose(true);
    }

    if (has_domain) {
      Local<Value> exit_v = domain->Get(env->exit_string());
      if (exit_v->IsFunction()) {
        exit_v.As<Function>()->Call(domain, 0, NULL);
        if (try_catch.HasCaught())
          return Undefined(env->isolate());
      }
    }*/

    if (try_catch.HasCaught()) {
      return Undefined(env->isolate());
    }

    /*Environment::TickInfo* tick_info = env->tick_info();

    if (tick_info->in_tick()) {
      return ret;
    }

    if (tick_info->length() == 0) {
      env->isolate()->RunMicrotasks();
    }

    if (tick_info->length() == 0) {
      tick_info->set_index(0);
      return ret;
    }

    tick_info->set_in_tick(true);

    // process nextTicks after call
    env->tick_callback_function()->Call(process, 0, NULL);

    tick_info->set_in_tick(false);

    if (try_catch.HasCaught()) {
      tick_info->set_last_threw(true);
      return Undefined(env->isolate());
    }*/

    return ret;
  }

  // Internal only.
  Handle<Value> MakeCallback(Environment* env,  Handle<Object> recv, uint32_t index, int argc, Handle<Value> argv[]) {
    Local<Value> cb_v = recv->Get(index);
    CHECK(cb_v->IsFunction());
    return MakeCallback(env, recv.As<Value>(), cb_v.As<Function>(), argc, argv);
  }


  Handle<Value> MakeCallback(Environment* env, Handle<Object> recv, Handle<String> symbol, int argc, Handle<Value> argv[]) {
    Local<Value> cb_v = recv->Get(symbol);
    CHECK(cb_v->IsFunction());
    return MakeCallback(env, recv.As<Value>(), cb_v.As<Function>(), argc, argv);
  }


  Handle<Value> MakeCallback(Environment* env, Handle<Object> recv, const char* method, int argc, Handle<Value> argv[]) {
    Local<String> method_string = OneByteString(env->isolate(), method);
    return MakeCallback(env, recv, method_string, argc, argv);
  }


  Handle<Value> MakeCallback(Isolate* isolate, Handle<Object> recv, const char* method, int argc, Handle<Value> argv[]) {
    EscapableHandleScope handle_scope(isolate);
    Local<Context> context = recv->CreationContext();
    Environment* env = Environment::GetCurrent(context);
    Context::Scope context_scope(context);
    return handle_scope.Escape( Local<Value>::New(isolate, MakeCallback(env, recv, method, argc, argv)));
  }


  Handle<Value> MakeCallback(Isolate* isolate, Handle<Object> recv, Handle<String> symbol, int argc, Handle<Value> argv[]) {
    EscapableHandleScope handle_scope(isolate);
    Local<Context> context = recv->CreationContext();
    Environment* env = Environment::GetCurrent(context);
    Context::Scope context_scope(context);
    return handle_scope.Escape( Local<Value>::New(isolate, MakeCallback(env, recv, symbol, argc, argv)));
  }


  Handle<Value> MakeCallback(Isolate* isolate, Handle<Object> recv, Handle<Function> callback, int argc, Handle<Value> argv[]) {
    EscapableHandleScope handle_scope(isolate);
    Local<Context> context = recv->CreationContext();
    Environment* env = Environment::GetCurrent(context);
    Context::Scope context_scope(context);
    return handle_scope.Escape(Local<Value>::New(isolate, MakeCallback(env, recv.As<Value>(), callback, argc, argv)));
  }

  //Predefine function
  static void ParseArgs(int* argc, const char** argv,
    int* exec_argc, const char*** exec_argv, int* v8_argc, const char*** v8_argv);
  static void PrintHelp();
  static bool ParseDebugOpt(const char* arg);
  Environment* CreateEnvironment(Isolate* isolate,/* uv_loop_t* loop,*/
    Handle<Context> context, int argc, const char* const* argv, int exec_argc, const char* const* exec_argv);
  void LoadEnvironment(Environment* env);
  // Executes a str within the current v8 context.
  static Local<Value> ExecuteString(Environment* env, Handle<String> source, Handle<String> filename);
  static void HandleCleanup(Environment* env, uv_handle_t* handle, void* arg);
  static void HandleCloseCb(uv_handle_t* handle) ;

  //Concreate Function
  void Init(int* argc, const char** argv, int* exec_argc, const char*** exec_argv) {
    // Should come before the call to V8::SetFlagsFromCommandLine()
    // so the user can disable a flag --foo at run-time by passing
    // --no_foo from the command line.
    #if defined(NODE_V8_OPTIONS)
      V8::SetFlagsFromString(NODE_V8_OPTIONS, sizeof(NODE_V8_OPTIONS) - 1);
    #endif

    // Parse a few arguments which are specific to Node.
    int v8_argc;
    const char** v8_argv;
    ParseArgs(argc, argv, exec_argc, exec_argv, &v8_argc, &v8_argv);

    // TODO(bnoordhuis) Intercept --prof arguments and start the CPU profiler
    // manually?  That would give us a little more control over its runtime
    // behavior but it could also interfere with the user's intentions in ways
    // we fail to anticipate.  Dillema.
    for (int i = 1; i < v8_argc; ++i) {
      if (strncmp(v8_argv[i], "--prof", sizeof("--prof") - 1) == 0) {
        v8_is_profiling = true;
        break;
      }
    }

    // Anything that's still in v8_argv is not a V8 or a node option.
    for (int i = 1; i < v8_argc; i++) {
      fprintf(stderr, "%s: bad option: %s\n", argv[0], v8_argv[i]);
    }
    delete[] v8_argv;
    v8_argv = NULL;

    if (v8_argc > 1) {
      exit(9);
    }
    if (debug_wait_connect) {
      const char expose_debug_as[] = "--expose_debug_as=v8debug";
      V8::SetFlagsFromString(expose_debug_as, sizeof(expose_debug_as) - 1);
    }

    V8::SetArrayBufferAllocator(&ArrayBufferAllocator::the_singleton);

    // Fetch a reference to the main isolate, so we have a reference to it
    // even when we need it to access it from another (debugger) thread.

    node_isolate = Isolate::New();
    Isolate::Scope isolate_scope(node_isolate);

    // Raise the open file descriptor limit.
    #ifdef __POSIX__
    {  // NOLINT (whitespace/braces)
        struct rlimit lim;
        if (getrlimit(RLIMIT_NOFILE, &lim) == 0 && lim.rlim_cur != lim.rlim_max) {
            // Do a binary search for the limit.
            rlim_t min = lim.rlim_cur;
            rlim_t max = 1 << 20;
            // But if there's a defined upper bound, don't search, just set it.
            if (lim.rlim_max != RLIM_INFINITY) {
              min = lim.rlim_max;
              max = lim.rlim_max;
            }
            do {
              lim.rlim_cur = min + (max - min) / 2;
              if (setrlimit(RLIMIT_NOFILE, &lim)) {
                max = lim.rlim_cur;
              } else {
                min = lim.rlim_cur;
              }
            } while (min + 1 < max);
          }
    }
    // Ignore SIGPIPE
    RegisterSignalHandler(SIGPIPE, SIG_IGN);
    RegisterSignalHandler(SIGINT, SignalExit, true);
    RegisterSignalHandler(SIGTERM, SignalExit, true);
    #endif  // __POSIX__

    /*if (!use_debug_agent) {
      RegisterDebugSignalHandler();
    }*/
  } //End Funtion Init

  int Start(int argc, char** argv) {
    const char* replaceInvalid = getenv("NODE_INVALID_UTF8");

    if (replaceInvalid == NULL)
      WRITE_UTF8_FLAGS |= String::REPLACE_INVALID_UTF8;

    /*#if !defined(_WIN32)
      // Try hard not to lose SIGUSR1 signals during the bootstrap process.
      InstallEarlyDebugSignalHandler();
    #endif*/

    assert(argc > 0);

    // Hack around with the argv pointer. Used for process.title = "blah".
    //argv = uv_setup_args(argc, argv);

    // This needs to run *before* V8::Initialize().  The const_cast is not
    // optional, in case you're wondering.
    int exec_argc;
    const char** exec_argv;
    Init(&argc, const_cast<const char**>(argv), &exec_argc, &exec_argv);

    /*#if HAVE_OPENSSL
    // V8 on Windows doesn't have a good source of entropy. Seed it from
    // OpenSSL's pool.
    V8::SetEntropySource(crypto::EntropySource);
    #endif*/

    int code;
    V8::Initialize();
    node_is_initialized = true;
    {
      Locker locker(node_isolate);
      Isolate::Scope isolate_scope(node_isolate);
      HandleScope handle_scope(node_isolate);
      Local<Context> context = Context::New(node_isolate);
      Environment* env = CreateEnvironment(
        node_isolate,
        //uv_default_loop(),
        context,
        argc,
        argv,
        exec_argc,
        exec_argv
      );
      Context::Scope context_scope(context);

      // Start debug agent when argv has --debug
      /*if (use_debug_agent)
        StartDebug(env, debug_wait_connect);*/

      LoadEnvironment(env);

      // Enable debugger
      /*if (use_debug_agent)
        EnableDebug(env);*/

      /*bool more;
      do {
        more = uv_run(env->event_loop(), UV_RUN_ONCE);
        if (more == false) {
          EmitBeforeExit(env);

          // Emit `beforeExit` if the loop became alive either after emitting
          // event, or after running some callbacks.
          more = uv_loop_alive(env->event_loop());
          if (uv_run(env->event_loop(), UV_RUN_NOWAIT) != 0)
            more = true;
        }
      } while (more == true);
      code = EmitExit(env);
      RunAtExit(env);

      env->Dispose();
      env = NULL;*/
    }

    /*CHECK_NE(node_isolate, NULL);
    node_isolate->Dispose();
    node_isolate = NULL;
    V8::Dispose();*/

    delete[] exec_argv;
    exec_argv = NULL;

    return code;
  } //End Function Start


  int Stop() {
    int code;
    CHECK_NE(node_isolate, NULL);
    node_isolate->Dispose();
    node_isolate = NULL;
    V8::Dispose();

    return code;
  }//End Function Stop

  Environment* CreateEnvironment(Isolate* isolate,
    uv_loop_t* loop,
    Handle<Context> context,
    int argc,
    const char* const* argv,
    int exec_argc,
    const char* const* exec_argv
  ) {
    HandleScope handle_scope(isolate);

    Context::Scope context_scope(context);
    Environment* env = Environment::New(context, loop);

    isolate->SetAutorunMicrotasks(false);

    uv_check_init(env->event_loop(), env->immediate_check_handle());
    uv_unref(reinterpret_cast<uv_handle_t*>(env->immediate_check_handle()));
    uv_idle_init(env->event_loop(), env->immediate_idle_handle());

    // Inform V8's CPU profiler when we're idle.  The profiler is sampling-based
    // but not all samples are created equal; mark the wall clock time spent in
    // epoll_wait() and friends so profiling tools can filter it out.  The samples
    // still end up in v8.log but with state=IDLE rather than state=EXTERNAL.
    // TODO(bnoordhuis) Depends on a libuv implementation detail that we should
    // probably fortify in the API contract, namely that the last started prepare
    // or check watcher runs first.  It's not 100% foolproof; if an add-on starts
    // a prepare or check watcher after us, any samples attributed to its callback
    // will be recorded with state=IDLE.
    uv_prepare_init(env->event_loop(), env->idle_prepare_handle());
    uv_check_init(env->event_loop(), env->idle_check_handle());
    uv_unref(reinterpret_cast<uv_handle_t*>(env->idle_prepare_handle()));
    uv_unref(reinterpret_cast<uv_handle_t*>(env->idle_check_handle()));

    // Register handle cleanups
    env->RegisterHandleCleanup(
        reinterpret_cast<uv_handle_t*>(env->immediate_check_handle()),
        HandleCleanup,
        NULL);
    env->RegisterHandleCleanup(
        reinterpret_cast<uv_handle_t*>(env->immediate_idle_handle()),
        HandleCleanup,
        NULL);
    env->RegisterHandleCleanup(
        reinterpret_cast<uv_handle_t*>(env->idle_prepare_handle()),
        HandleCleanup,
        NULL);
    env->RegisterHandleCleanup(
        reinterpret_cast<uv_handle_t*>(env->idle_check_handle()),
        HandleCleanup,
        NULL);

    //TODO ::
    /*if (v8_is_profiling) {
      StartProfilerIdleNotifier(env);
    }*/

    Local<FunctionTemplate> process_template = FunctionTemplate::New(isolate);
    process_template->SetClassName(FIXED_ONE_BYTE_STRING(isolate, "process"));

    Local<Object> process_object = process_template->GetFunction()->NewInstance();
    env->set_process_object(process_object);

    //TODO :: setup process
    //SetupProcessObject(env, argc, argv, exec_argc, exec_argv);

    return env;
  }

  void LoadEnvironment(Environment* env) {
    HandleScope handle_scope(env->isolate());

    V8::SetFatalErrorHandler(node::OnFatalError);
    V8::AddMessageListener(OnMessage);

    // Compile, execute the src/node.js file. (Which was included as static C
    // string in node_natives.h. 'natve_node' is the string containing that
    // source code.)

    // The node.js file returns a function 'f'
    //atexit(AtExit);

    TryCatch try_catch;

    // Disable verbose mode to stop FatalException() handler from trying
    // to handle the exception. Errors this early in the start-up phase
    // are not safe to ignore.
    try_catch.SetVerbose(false);

    Local<String> script_name = FIXED_ONE_BYTE_STRING(env->isolate(), "node.js");
    Local<Value> f_value = ExecuteString(env, MainSource(env), script_name);
    if (try_catch.HasCaught())  {
      ReportException(env, try_catch);
      //exit(10);
      return;
    }
    assert(f_value->IsFunction());
    Local<Function> f = Local<Function>::Cast(f_value);

    // Now we call 'f' with the 'process' variable that we've built up with
    // all our bindings. Inside node.js we'll take care of assigning things to
    // their places.

    // We start the process this way in order to be more modular. Developers
    // who do not like how 'src/node.js' setups the module system but do like
    // Node's I/O bindings may want to replace 'f' with their own function.

    // Add a reference to the global object
    Local<Object> global = env->context()->Global();

    #if defined HAVE_DTRACE || defined HAVE_ETW
      InitDTrace(env, global);
    #endif

    #if defined HAVE_PERFCTR
      InitPerfCounters(env, global);
    #endif

    // Enable handling of uncaught exceptions
    // (FatalException(), break on uncaught exception in debugger)
    //
    // This is not strictly necessary since it's almost impossible
    // to attach the debugger fast enought to break on exception
    // thrown during process startup.
    try_catch.SetVerbose(true);

    //TODO :: RawDebug
    //NODE_SET_METHOD(env->process_object(), "_rawDebug", RawDebug);

    Local<Value> arg = env->process_object();
    f->Call(global, 1, &arg);
  }

  // Executes a str within the current v8 context.
  static Local<Value> ExecuteString(Environment* env, Handle<String> source, Handle<String> filename) {
    EscapableHandleScope scope(env->isolate());
    TryCatch try_catch;

    // try_catch must be nonverbose to disable FatalException() handler,
    // we will handle exceptions ourself.
    try_catch.SetVerbose(false);

    Local<v8::Script> script = v8::Script::Compile(source, filename);
    if (script.IsEmpty()) {
      ReportException(env, try_catch);
      //exit(3);
    }

    Local<Value> result = script->Run();
    if (result.IsEmpty()) {
      ReportException(env, try_catch);
      //exit(4);
    }

    return scope.Escape(result);
  }

  static void ParseArgs(int* argc, const char** argv,  int* exec_argc, const char*** exec_argv, int* v8_argc, const char*** v8_argv) {
    const unsigned int nargs = static_cast<unsigned int>(*argc);
    const char** new_exec_argv = new const char*[nargs];
    const char** new_v8_argv = new const char*[nargs];
    const char** new_argv = new const char*[nargs];

    for (unsigned int i = 0; i < nargs; ++i) {
      new_exec_argv[i] = NULL;
      new_v8_argv[i] = NULL;
      new_argv[i] = NULL;
    }

    // exec_argv starts with the first option, the other two start with argv[0].
    unsigned int new_exec_argc = 0;
    unsigned int new_v8_argc = 1;
    unsigned int new_argc = 1;
    new_v8_argv[0] = argv[0];
    new_argv[0] = argv[0];

    unsigned int index = 1;
    while (index < nargs && argv[index][0] == '-') {
      const char* const arg = argv[index];
      unsigned int args_consumed = 1;

      if (ParseDebugOpt(arg)) {
        // Done, consumed by ParseDebugOpt().
      }
      /*else if (strcmp(arg, "--version") == 0 || strcmp(arg, "-v") == 0) {
        printf("%s\n", NODE_VERSION);
        exit(0);
      } else if (strcmp(arg, "--enable-ssl2") == 0) {
        SSL2_ENABLE = true;
      } else if (strcmp(arg, "--enable-ssl3") == 0) {
        SSL3_ENABLE = true;
      }*/
      else if (strcmp(arg, "--help") == 0 || strcmp(arg, "-h") == 0) {
        PrintHelp();
        exit(0);
      }
      else if (strcmp(arg, "--eval") == 0 ||
                 strcmp(arg, "-e") == 0 ||
                 strcmp(arg, "--print") == 0 ||
                 strcmp(arg, "-pe") == 0 ||
                 strcmp(arg, "-p") == 0) {
        bool is_eval = strchr(arg, 'e') != NULL;
        bool is_print = strchr(arg, 'p') != NULL;
        print_eval = print_eval || is_print;
        // --eval, -e and -pe always require an argument.
        if (is_eval == true) {
          args_consumed += 1;
          eval_string = argv[index + 1];
          if (eval_string == NULL) {
            fprintf(stderr, "%s: %s requires an argument\n", argv[0], arg);
            exit(9);
          }
        } else if ((index + 1 < nargs) &&
                   argv[index + 1] != NULL &&
                   argv[index + 1][0] != '-') {
          args_consumed += 1;
          eval_string = argv[index + 1];
          if (strncmp(eval_string, "\\-", 2) == 0) {
            // Starts with "\\-": escaped expression, drop the backslash.
            eval_string += 1;
          }
        }
      } else if (strcmp(arg, "--interactive") == 0 || strcmp(arg, "-i") == 0) {
        force_repl = true;
      } else if (strcmp(arg, "--no-deprecation") == 0) {
        no_deprecation = true;
      } else if (strcmp(arg, "--trace-deprecation") == 0) {
        trace_deprecation = true;
      } else if (strcmp(arg, "--throw-deprecation") == 0) {
        throw_deprecation = true;
      } else if (strcmp(arg, "--v8-options") == 0) {
        new_v8_argv[new_v8_argc] = "--help";
        new_v8_argc += 1;
      /*#if defined(NODE_HAVE_I18N_SUPPORT)
      } else if (strncmp(arg, "--icu-data-dir=", 15) == 0) {
        icu_data_dir = arg + 15;
      #endif*/
      } else {
        // V8 option.  Pass through as-is.
        new_v8_argv[new_v8_argc] = arg;
        new_v8_argc += 1;
      }

      memcpy(new_exec_argv + new_exec_argc,
             argv + index,
             args_consumed * sizeof(*argv));

      new_exec_argc += args_consumed;
      index += args_consumed;
    }

    // Copy remaining arguments.
    const unsigned int args_left = nargs - index;
    memcpy(new_argv + new_argc, argv + index, args_left * sizeof(*argv));
    new_argc += args_left;

    *exec_argc = new_exec_argc;
    *exec_argv = new_exec_argv;
    *v8_argc = new_v8_argc;
    *v8_argv = new_v8_argv;

    // Copy new_argv over argv and update argc.
    memcpy(argv, new_argv, new_argc * sizeof(*argv));
    delete[] new_argv;
    *argc = static_cast<int>(new_argc);
  } //End Function ParseArgs


  static bool ParseDebugOpt(const char* arg) {
    const char* port = NULL;
    if (!strcmp(arg, "--debug")) {
      use_debug_agent = true;
    } else if (!strncmp(arg, "--debug=", sizeof("--debug=") - 1)) {
      use_debug_agent = true;
      port = arg + sizeof("--debug=") - 1;
    } else if (!strcmp(arg, "--debug-brk")) {
      use_debug_agent = true;
      debug_wait_connect = true;
    } else if (!strncmp(arg, "--debug-brk=", sizeof("--debug-brk=") - 1)) {
      use_debug_agent = true;
      debug_wait_connect = true;
      port = arg + sizeof("--debug-brk=") - 1;
    } else if (!strncmp(arg, "--debug-port=", sizeof("--debug-port=") - 1)) {
      port = arg + sizeof("--debug-port=") - 1;
    } else {
      return false;
    }

    if (port != NULL) {
      debug_port = atoi(port);
      if (debug_port < 1024 || debug_port > 65535) {
        fprintf(stderr, "Debug port must be in range 1024 to 65535.\n");
        PrintHelp();
        exit(12);
      }
    }
    return true;

  } //End Funtion ParseDebugOpt


  static void PrintHelp() {
    printf("Usage: node [options] [ -e script | script.js ] [arguments] \n"
           "       node debug script.js [arguments] \n"
           "\n"
           "Options:\n"
           "  -v, --version        print node's version\n"
           "  -e, --eval script    evaluate script\n"
           "  -p, --print          evaluate script and print result\n"
           "  -i, --interactive    always enter the REPL even if stdin\n"
           "                       does not appear to be a terminal\n"
           "  --no-deprecation     silence deprecation warnings\n"
           "  --throw-deprecation  throw an exception anytime a deprecated "
           "function is used\n"
           "  --trace-deprecation  show stack traces on deprecations\n"
           "  --v8-options         print v8 command line options\n"
           "  --max-stack-size=val set max v8 stack size (bytes)\n"
  /*#if defined(NODE_HAVE_I18N_SUPPORT)
           "  --icu-data-dir=dir   set ICU data load path to dir\n"
           "                         (overrides NODE_ICU_DATA)\n"
  #if !defined(NODE_HAVE_SMALL_ICU)
           "                       Note: linked-in ICU data is\n"
           "                       present.\n"
  #endif
  #endif*/
           "  --enable-ssl2        enable ssl2\n"
           "  --enable-ssl3        enable ssl3\n"
           "\n"
           "Environment variables:\n"
  #ifdef _WIN32
           "NODE_PATH              ';'-separated list of directories\n"
  #else
           "NODE_PATH              ':'-separated list of directories\n"
  #endif
           "                       prefixed to the module search path.\n"
           "NODE_MODULE_CONTEXTS   Set to 1 to load modules in their own\n"
           "                       global contexts.\n"
           "NODE_DISABLE_COLORS    Set to 1 to disable colors in the REPL\n"
  /*#if defined(NODE_HAVE_I18N_SUPPORT)
           "NODE_ICU_DATA          Data path for ICU (Intl object) data\n"
  #if !defined(NODE_HAVE_SMALL_ICU)
           "                       (will extend linked-in data)\n"
  #endif
  #endif*/
           "\n"
           "Documentation can be found at http://nodejs.org/\n");

  } //End Funtion PrintHelp

  enum encoding ParseEncoding(Isolate* isolate, Handle<Value> encoding_v, enum encoding _default) {
    HandleScope scope(isolate);

    if (!encoding_v->IsString())
      return _default;

    node::Utf8Value encoding(encoding_v);

    if (strcasecmp(*encoding, "utf8") == 0) {
      return UTF8;
    } else if (strcasecmp(*encoding, "utf-8") == 0) {
      return UTF8;
    } else if (strcasecmp(*encoding, "ascii") == 0) {
      return ASCII;
    } else if (strcasecmp(*encoding, "base64") == 0) {
      return BASE64;
    } else if (strcasecmp(*encoding, "ucs2") == 0) {
      return UCS2;
    } else if (strcasecmp(*encoding, "ucs-2") == 0) {
      return UCS2;
    } else if (strcasecmp(*encoding, "utf16le") == 0) {
      return UCS2;
    } else if (strcasecmp(*encoding, "utf-16le") == 0) {
      return UCS2;
    } else if (strcasecmp(*encoding, "binary") == 0) {
      return BINARY;
    } else if (strcasecmp(*encoding, "buffer") == 0) {
      return BUFFER;
    } else if (strcasecmp(*encoding, "hex") == 0) {
      return HEX;
    } else if (strcasecmp(*encoding, "raw") == 0) {
      if (!no_deprecation) {
        fprintf(stderr, "'raw' (array of integers) has been removed. "
                        "Use 'binary'.\n");
      }
      return BINARY;
    } else if (strcasecmp(*encoding, "raws") == 0) {
      if (!no_deprecation) {
        fprintf(stderr, "'raws' encoding has been renamed to 'binary'. "
                        "Please update your code.\n");
      }
      return BINARY;
    } else {
      return _default;
    }
  } //End Function ParseEncoding

  Local<Value> Encode(Isolate* isolate, const void* buf,  size_t len,enum encoding encoding) {
    return StringBytes::Encode(isolate,
                               static_cast<const char*>(buf),
                               len,
                               encoding);
  }

  // Returns -1 if the handle was not valid for decoding
  ssize_t DecodeBytes(Isolate* isolate,  Handle<Value> val, enum encoding encoding) {
    HandleScope scope(isolate);

    if (val->IsArray()) {
      //fprintf(stderr, "'raw' encoding (array of integers) has been removed. "
                      //"Use 'binary'.\n");
      assert(0);
      return -1;
    }

    return StringBytes::Size(isolate, val, encoding);
  }

  #ifndef MIN
  # define MIN(a, b) ((a) < (b) ? (a) : (b))
  #endif

  // Returns number of bytes written.
  ssize_t DecodeWrite(Isolate* isolate, char* buf, size_t buflen, Handle<Value> val, enum encoding encoding) {
    return StringBytes::Write(isolate, buf, buflen, val, encoding, NULL);
  }

  extern "C" void node_module_register(void* m) {
    struct node_module* mp = reinterpret_cast<struct node_module*>(m);

    if (mp->nm_flags & NM_F_BUILTIN) {
      mp->nm_link = modlist_builtin;
      modlist_builtin = mp;
    } else if (!node_is_initialized) {
      // "Linked" modules are included as part of the node project.
      // Like builtins they are registered *before* node::Init runs.
      mp->nm_flags = NM_F_LINKED;
      mp->nm_link = modlist_linked;
      modlist_linked = mp;
    } else {
      // Once node::Init was called we can only register dynamic modules.
      // See DLOpen.
      assert(modpending == NULL);
      modpending = mp;
    }
  } //End Function node_module_register

  struct node_module* get_builtin_module(const char* name) {
    struct node_module* mp;

    for (mp = modlist_builtin; mp != NULL; mp = mp->nm_link) {
      if (strcmp(mp->nm_modname, name) == 0)
        break;
    }

    assert(mp == NULL || (mp->nm_flags & NM_F_BUILTIN) != 0);
    return (mp);
  } //ENd Function get_builtin_module

  struct node_module* get_linked_module(const char* name) {
    struct node_module* mp;

    for (mp = modlist_linked; mp != NULL; mp = mp->nm_link) {
      if (strcmp(mp->nm_modname, name) == 0)
        break;
    }

    CHECK(mp == NULL || (mp->nm_flags & NM_F_LINKED) != 0);
    return mp;
  } //End Function get_linked_module

  static void HandleCleanup(Environment* env, uv_handle_t* handle, void* arg) {
    handle->data = env;
    uv_close(handle, HandleCloseCb);
  }//End Function HandleCleanup

  static void HandleCloseCb(uv_handle_t* handle) {
    Environment* env = reinterpret_cast<Environment*>(handle->data);
    env->FinishHandleCleanup(handle);
  }//End Function HandleCloseCb

}//End Node Namespace
