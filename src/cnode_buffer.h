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

#ifndef SRC_NODE_BUFFER_H_
#define SRC_NODE_BUFFER_H_

#include "cnode.h"
#include "csmalloc.h"
#include "v8.h"

namespace node {
namespace Buffer {

  using v8::Isolate;
  using v8::Handle;
  using v8::String;
  using v8::Object;
  using v8::Value;

  static const unsigned int kMaxLength = smalloc::kMaxLength;

  NODE_EXTERN bool HasInstance(Handle<Value> val);
  NODE_EXTERN bool HasInstance(Handle<Object> val);
  NODE_EXTERN char* Data(Handle<Value> val);
  NODE_EXTERN char* Data(Handle<Object> val);
  NODE_EXTERN size_t Length(Handle<Value> val);
  NODE_EXTERN size_t Length(Handle<Object> val);

  NODE_EXTERN Local<Object> New(Isolate* isolate, size_t length);
  NODE_DEPRECATED("Use New(isolate, ...)",
  inline Local<Object> New(size_t length) {
    return New(Isolate::GetCurrent(), length);
  })

  // public constructor from string
  NODE_EXTERN Local<Object> New(Isolate* isolate, Handle<String> string,  enum encoding enc = UTF8);
  NODE_DEPRECATED("Use New(isolate, ...)",
  inline Local<Object> New(Handle<String> string, enum encoding enc = UTF8) {
    return New(Isolate::GetCurrent(), string, enc);
  })

  // public constructor - data is copied
  // TODO(trevnorris): should be something like Copy()
  NODE_EXTERN Local<Object> New(Isolate* isolate,  const char* data,  size_t len);
  NODE_DEPRECATED("Use New(isolate, ...)",
  inline Local<Object> New(const char* data, size_t len) {
    return New(Isolate::GetCurrent(), data, len);
  })

  // public constructor - data is used, callback is passed data on object gc
  NODE_EXTERN Local<Object> New(Isolate* isolate,  char* data,  size_t length, smalloc::FreeCallback callback,   void* hint);
  NODE_DEPRECATED("Use New(isolate, ...)",
  inline Local<Object> New(char* data, size_t length, smalloc::FreeCallback callback, void* hint) {
    return New(Isolate::GetCurrent(), data, length, callback, hint);
  })

  // public constructor - data is used.
  // TODO(trevnorris): should be New() for consistency
  NODE_EXTERN Local<Object> Use(Isolate* isolate, char* data, uint32_t len);
  NODE_DEPRECATED("Use Use(isolate, ...)",
  inline Local<Object> Use(char* data, uint32_t len) {
    return Use(v8::Isolate::GetCurrent(), data, len);
  })


  // This is verbose to be explicit with inline commenting
  static inline bool IsWithinBounds(size_t off, size_t len, size_t max) {
    // Asking to seek too far into the buffer
    // check to avoid wrapping in subsequent subtraction
    if (off > max)
      return false;

    // Asking for more than is left over in the buffer
    if (max - off < len)
      return false;

    // Otherwise we're in bounds
    return true;
  }


  // Internal. Not for public consumption. We can't define these in
  // src/node_internals.h due to a circular dependency issue with
  // the smalloc.h and node_internals.h headers.
  //#if defined(NODE_WANT_INTERNALS)
  Local<Object> New(Environment* env, size_t size);
  Local<Object> New(Environment* env, const char* data, size_t len);
  Local<Object> New(Environment* env,  char* data, size_t length, smalloc::FreeCallback callback,  void* hint);
  Local<Object> Use(Environment* env, char* data, uint32_t length);
  //#endif  // defined(NODE_WANT_INTERNALS)

} //End Buffer Namespace
} //End Node Namespace

#endif //SRC_NODE_BUFFER_H_
