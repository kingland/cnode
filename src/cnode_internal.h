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

#ifndef SRC_NODE_INTERNALS_H_
#define SRC_NODE_INTERNALS_H_

#include "v8.h"

struct sockaddr;

namespace node{
  using v8::Local;
  using v8::Handle;
  using v8::Value;
  using v8::Object;
  using v8::String;
  using v8::Function;
  using v8::Message;
  using v8::AccessorGetterCallback;
  using v8::ObjectTemplate;

  #if defined(__x86_64__)
  # define BITS_PER_LONG 64
  #else
  # define BITS_PER_LONG 32
  #endif

  #ifndef ARRAY_SIZE
  # define ARRAY_SIZE(a) (sizeof((a)) / sizeof((a)[0]))
  #endif

  #ifndef ROUND_UP
  # define ROUND_UP(a, b) ((a) % (b) ? ((a) + (b)) - ((a) % (b)) : (a))
  #endif

  #if defined(__GNUC__) && __GNUC__ >= 4
  # define MUST_USE_RESULT __attribute__((warn_unused_result))
  # define NO_RETURN __attribute__((noreturn))
  #else
  # define MUST_USE_RESULT
  # define NO_RETURN
  #endif

  //NO_RETURN
  void FatalError(const char* location, const char* message);

  void AppendExceptionLine(Environment* env, Handle<Value> er, Handle<Message> message);

  //Call in file src/node_v8.cc
  // Call with valid HandleScope and while inside Context scope.
  Handle<Value> MakeCallback(Environment* env, Handle<Object> recv, const char* method,
     int argc = 0, Handle<Value>* argv = NULL);

  // Call with valid HandleScope and while inside Context scope.
  Handle<Value> MakeCallback(Environment* env, Handle<Object> recv, uint32_t index,
    int argc = 0, Handle<Value>* argv = NULL);

  // Call with valid HandleScope and while inside Context scope.
  Handle<Value> MakeCallback(Environment* env, Handle<Object> recv, Handle<String> symbol,
    int argc = 0, Handle<Value>* argv = NULL);

  // Call with valid HandleScope and while inside Context scope.
  Handle<Value> MakeCallback(Environment* env, Handle<Value> recv, Handle<Function> callback,
    int argc = 0, Handle<Value>* argv = NULL);


  // Convert a struct sockaddr to a { address: '1.2.3.4', port: 1234 } JS object.
  // Sets address and port properties on the info object and returns it.
  // If |info| is omitted, a new object is returned.
  Local<Object> AddressToJS(Environment* env,  const sockaddr* addr, Local<Object> info = Handle<Object>());

  enum Endianness {
    kLittleEndian,  // _Not_ LITTLE_ENDIAN, clashes with endian.h.
    kBigEndian
  };

  inline enum Endianness GetEndianness() {
    // Constant-folded by the compiler.
    const union {
      uint8_t u8[2];
      uint16_t u16;
    } u = {
      { 1, 0 }
    };
    return u.u16 == 1 ? kLittleEndian : kBigEndian;
  }

  inline bool IsLittleEndian() {
    return GetEndianness() == kLittleEndian;
  }


  inline bool IsBigEndian() {
    return GetEndianness() == kBigEndian;
  }

  // parse index for external array data
  inline MUST_USE_RESULT bool ParseArrayIndex(Handle<Value> arg, size_t def,size_t* ret) {
    if (arg->IsUndefined()) {
      *ret = def;
      return true;
    }

    int32_t tmp_i = arg->Int32Value();

    if (tmp_i < 0)
      return false;

    *ret = static_cast<size_t>(tmp_i);
    return true;
  }

  inline void NODE_SET_EXTERNAL(Handle<ObjectTemplate> target, const char* key, AccessorGetterCallback getter) {
    Isolate* isolate = Isolate::GetCurrent();
    HandleScope handle_scope(isolate);
    Local<String> prop = String::NewFromUtf8(isolate, key);
    target->SetAccessor(prop,
                        getter,
                        NULL,
                        Handle<Value>(),
                        v8::DEFAULT,
                        static_cast<v8::PropertyAttribute>(v8::ReadOnly |
                                                           v8::DontDelete));
  }

}//End Node Namespace

#endif //SRC_NODE_INTERNALS_H_
