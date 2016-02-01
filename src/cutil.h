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
#ifndef SRC_UTIL_H_
#define SRC_UTIL_H_

#include "v8.h"

#include <assert.h>
#include <stddef.h>
#include <stdlib.h>

namespace node {
  using v8::Handle;
  using v8::Local;
  using v8::Isolate;
  using v8::Persistent;
  using v8::String;
  using v8::Object;

  #define FIXED_ONE_BYTE_STRING(isolate, string)                                \
    (node::OneByteString((isolate), (string), sizeof(string) - 1))

  #define DISALLOW_COPY_AND_ASSIGN(TypeName)                                    \
    void operator=(const TypeName&);                                            \
    TypeName(const TypeName&)

  #if defined(NDEBUG)
    #define ASSERT(expression)
    #define CHECK(expression)                                                    \
      do {                                                                        \
        if (!(expression)) abort();                                               \
      } while (0)
  #else
    #define ASSERT(expression)  assert(expression)
    #define CHECK(expression)   assert(expression)
  #endif

  #define CHECK_EQ(a, b) CHECK((a) == (b))
  #define CHECK_GE(a, b) CHECK((a) >= (b))
  #define CHECK_GT(a, b) CHECK((a) > (b))
  #define CHECK_LE(a, b) CHECK((a) <= (b))
  #define CHECK_LT(a, b) CHECK((a) < (b))
  #define CHECK_NE(a, b) CHECK((a) != (b))

  #define UNREACHABLE() abort()


  // The helper is for doing safe downcasts from base types to derived types.
  template <typename Inner, typename Outer>
  class ContainerOfHelper {
   public:
    inline ContainerOfHelper(Inner Outer::*field, Inner* pointer);
    template <typename TypeName>
    inline operator TypeName*() const;
   private:
    Outer* const pointer_;
  }; //End Class ContainerOfHelper

  // Calculate the address of the outer (i.e. embedding) struct from
  // the interior pointer to a data member.
  template <typename Inner, typename Outer>
  inline ContainerOfHelper<Inner, Outer> ContainerOf(Inner Outer::*field, Inner* pointer);

  // If persistent.IsWeak() == false, then do not call persistent.Reset()
  // while the returned Local<T> is still in scope, it will destroy the
  // reference to the object.
  template <class TypeName>
  inline Local<TypeName> PersistentToLocal(Isolate* isolate, const v8::Persistent<TypeName>& persistent);

  // Unchecked conversion from a non-weak Persistent<T> to Local<TLocal<T>,
  // use with care!
  //
  // Do not call persistent.Reset() while the returned Local<T> is still in
  // scope, it will destroy the reference to the object.
  template <class TypeName>
  inline Local<TypeName> StrongPersistentToLocal(const Persistent<TypeName>& persistent);

  template <class TypeName>
  inline Local<TypeName> WeakPersistentToLocal(Isolate* isolate,  const Persistent<TypeName>& persistent);

  // Convenience wrapper around v8::String::NewFromOneByte().
  inline Local<String> OneByteString(Isolate* isolate, const char* data, int length = -1);

  // For the people that compile with -funsigned-char.
  inline Local<String> OneByteString(Isolate* isolate, const signed char* data, int length = -1);

  inline Local<String> OneByteString(Isolate* isolate, const unsigned char* data, int length = -1);

  inline void Wrap(Local<Object> object, void* pointer);

  inline void ClearWrap(Local<Object> object);

  template <typename TypeName>
  inline TypeName* Unwrap(Local<Object> object);

  class Utf8Value {
    public:
      explicit Utf8Value(v8::Handle<v8::Value> value);

      ~Utf8Value() {
        free(str_);
      }

      char* operator*() {
        return str_;
      };

      const char* operator*() const {
        return str_;
      };

      size_t length() const {
        return length_;
      };

    private:
      size_t length_;
      char* str_;
  }; //End Class Utf8Value

}//End Node Namespace
#endif  // SRC_UTIL_H_
