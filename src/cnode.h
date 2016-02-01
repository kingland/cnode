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

#ifndef SRC_NODE_H_
#define SRC_NODE_H_

#include "v8.h"

#ifdef _WIN32
//#   ifndef BUILDING_NODE_EXTENSION
//#     define NODE_EXTERN __declspec(dllexport)
//#   else
//#     define NODE_EXTERN __declspec(dllimport)
//#   endif
#define NODE_EXTERN __declspec(dllexport)
#else
#    define NODE_EXTERN /* nothing */
#endif

#ifdef BUILDING_NODE_EXTENSION
#   undef BUILDING_V8_SHARED
#   undef BUILDING_UV_SHARED
#   define USING_V8_SHARED 1
#   define USING_UV_SHARED 1
#endif

// This should be defined in make system.
// See issue https://github.com/joyent/node/issues/1236
#if defined(__MINGW32__) || defined(_MSC_VER)
#   ifndef _WIN32_WINNT
#     define _WIN32_WINNT   0x0501
#   endif
#     define NOMINMAX
#endif

#if defined(_MSC_VER)
#   define PATH_MAX MAX_PATH
#endif


#ifdef _WIN32
#   define SIGKILL         9
#endif

#define NODE_DEPRECATED(msg, fn) V8_DEPRECATED(msg, fn)

#define NM_F_BUILTIN 0x01
#define NM_F_LINKED  0x02

//#include "v8.h"  // NOLINT(build/include_order)
//#include "node_version.h"  // NODE_MODULE_VERSION

namespace node {
  using v8::Isolate;
  using v8::Context;
  using v8::HandleScope;
  using v8::Handle;
  using v8::Value;
  using v8::Local;
  using v8::Signature;
  using v8::FunctionCallback;
  using v8::FunctionTemplate;
  using v8::Function;
  using v8::String;
  using v8::Date;
  using v8::Object;
  using v8::Number;
  using v8::PropertyAttribute;


  NODE_EXTERN int Start(int argc, char *argv[]);

  NODE_EXTERN int Stop();

  NODE_EXTERN void Init(int* argc, const char** argv, int* exec_argc, const char*** exec_argv);

  NODE_EXTERN Local<Value> ErrnoException(Isolate* isolate, int errorno,
    const char* syscall = NULL, const char* message = NULL, const char* path = NULL);

  NODE_EXTERN v8::Local<v8::Value> UVException(v8::Isolate* isolate, int errorno,
    const char* syscall = NULL, const char* message = NULL, const char* path = NULL);

  inline v8::Local<v8::Value> UVException(int errorno,
    const char* syscall = NULL, const char* message = NULL,  const char* path = NULL) {
    return UVException(v8::Isolate::GetCurrent(), errorno, syscall,  message, path);
  }

  //Class define
  class Environment;

  NODE_EXTERN Environment* CreateEnvironment(v8::Isolate* isolate,
    struct uv_loop_s* loop,
    v8::Handle<v8::Context> context,
    int argc,
    const char* const* argv,
    int exec_argc,
    const char* const* exec_argv);

  enum encoding {ASCII, UTF8, BASE64, UCS2, BINARY, HEX, BUFFER};
  enum encoding ParseEncoding(Isolate* isolate, Handle<Value> encoding_v,enum encoding _default = BINARY);

  NODE_EXTERN Local<Value> Encode(Isolate* isolate, const void* buf, size_t len, enum encoding encoding = BINARY);
  // Returns -1 if the handle was not valid for decoding
  NODE_EXTERN ssize_t DecodeBytes(Isolate* isolate, Handle<Value>,  enum encoding encoding = BINARY);
  // returns bytes written.
  NODE_EXTERN ssize_t DecodeWrite(Isolate* isolate, char* buf, size_t buflen, Handle<Value>, enum encoding encoding = BINARY);

  const char *signo_string(int errorno);

  typedef void (*addon_register_func)(Handle<Object> exports, Handle<Value> module,void* priv);

  typedef void (*addon_context_register_func)(Handle<Object> exports, Handle<Value> module, Handle<Context> context, void* priv);

  //Node Module
  struct node_module {
    int nm_version;
    unsigned int nm_flags;
    void* nm_dso_handle;
    const char* nm_filename;
    node::addon_register_func nm_register_func;
    node::addon_context_register_func nm_context_register_func;
    const char* nm_modname;
    void* nm_priv;
    struct node_module* nm_link;
  };

  node_module* get_builtin_module(const char *name);
  node_module* get_linked_module(const char *name);

  extern "C" NODE_EXTERN void node_module_register(void* mod);

  // Used to be a macro, hence the uppercase name.
  template <typename TypeName>
  inline void NodeSetMethode(const TypeName& recv, const char* name, FunctionCallback callback) {
    Isolate* isolate = Isolate::GetCurrent();
    HandleScope handle_scope(isolate);
    Local<FunctionTemplate> t = FunctionTemplate::New(isolate,  callback);
    Local<Function> fn = t->GetFunction();
    Local<String> fn_name = String::NewFromUtf8(isolate, name);
    fn->SetName(fn_name);
    recv->Set(fn_name, fn);
  }


  // Used to be a macro, hence the uppercase name.
  // Not a template because it only makes sense for FunctionTemplates.
  inline void NodeSetPrototypeMethod(Handle<FunctionTemplate> recv, const char* name, FunctionCallback callback) {
    Isolate* isolate = Isolate::GetCurrent();
    HandleScope handle_scope(isolate);
    Handle<Signature> s = Signature::New(isolate, recv);
    Local<FunctionTemplate> t = FunctionTemplate::New(isolate, callback, Handle<Value>(), s);
    Local<Function> fn = t->GetFunction();
    recv->PrototypeTemplate()->Set(String::NewFromUtf8(isolate, name), fn);
    Local<String> fn_name = String::NewFromUtf8(isolate, name);
    fn->SetName(fn_name);
  }

  #define NODE_SET_METHOD node::NodeSetMethode
  #define NODE_SET_PROTOTYPE_METHOD node::NodeSetPrototypeMethod

  /* Converts a unixtime to V8 Date */
  #define NODE_UNIXTIME_V8(t) Date::New(Isolate::GetCurrent(),                  \
      1000 * static_cast<double>(t))

  #define NODE_V8_UNIXTIME(v) (static_cast<double>((v)->NumberValue())/1000.0);

  // Used to be a macro, hence the uppercase name.
  #define NODE_DEFINE_CONSTANT(target, constant)                                \
    do {                                                                        \
      Isolate* isolate = Isolate::GetCurrent();                                 \
      Local<String> constant_name = String::NewFromUtf8(isolate, #constant);                          \
      Local<Number> constant_value = Number::New(isolate, static_cast<double>(constant));              \
      PropertyAttribute constant_attributes =  static_cast<v8::PropertyAttribute>(v8::ReadOnly | v8::DontDelete);    \
      (target)->ForceSet(constant_name, constant_value, constant_attributes);   \
    }                                                                           \
    while (0)



  #define NM_F_BUILTIN 0x01
  #define NM_F_LINKED  0x02

  #ifdef _WIN32
    # define NODE_MODULE_EXPORT __declspec(dllexport)
  #else
    # define NODE_MODULE_EXPORT __attribute__((visibility("default")))
  #endif

  #if defined(_MSC_VER)
  #pragma section(".CRT$XCU", read)
  #define NODE_C_CTOR(fn)                                               \
    static void __cdecl fn(void);                                       \
    __declspec(dllexport, allocate(".CRT$XCU"))                         \
        void (__cdecl*fn ## _)(void) = fn;                              \
    static void __cdecl fn(void)
  #else
  #define NODE_C_CTOR(fn)                                               \
    static void fn(void) __attribute__((constructor));                  \
    static void fn(void)
  #endif

  #define NODE_MODULE_X(modname, regfunc, priv, flags)                  \
    extern "C" {                                                        \
      static node::node_module _module =                                \
      {                                                                 \
        NODE_MODULE_VERSION,                                            \
        flags,                                                          \
        NULL,                                                           \
        __FILE__,                                                       \
        (node::addon_register_func) (regfunc),                          \
        NULL,                                                           \
        NODE_STRINGIFY(modname),                                        \
        priv,                                                           \
        NULL                                                            \
      };                                                                \
      NODE_C_CTOR(_register_ ## modname) {                              \
        node_module_register(&_module);                                 \
      }                                                                 \
    }

  #define NODE_MODULE_CONTEXT_AWARE_X(modname, regfunc, priv, flags)    \
    extern "C" {                                                        \
      static node::node_module _module =                                \
      {                                                                 \
        NODE_MODULE_VERSION,                                            \
        flags,                                                          \
        NULL,                                                           \
        __FILE__,                                                       \
        NULL,                                                           \
        (node::addon_context_register_func) (regfunc),                  \
        NODE_STRINGIFY(modname),                                        \
        priv,                                                           \
        NULL                                                            \
      };                                                                \
      NODE_C_CTOR(_register_ ## modname) {                              \
        node_module_register(&_module);                                 \
      }                                                                 \
    }

  #define NODE_MODULE(modname, regfunc)                                 \
    NODE_MODULE_X(modname, regfunc, NULL, 0)

  #define NODE_MODULE_CONTEXT_AWARE(modname, regfunc)                   \
    NODE_MODULE_CONTEXT_AWARE_X(modname, regfunc, NULL, 0)

  #define NODE_MODULE_CONTEXT_AWARE_BUILTIN(modname, regfunc)           \
    NODE_MODULE_CONTEXT_AWARE_X(modname, regfunc, NULL, NM_F_BUILTIN)   \

  #define NODE_MODULE_DECL /* nothing */

}//End Node Namespace

#endif  // SRC_NODE_H_
