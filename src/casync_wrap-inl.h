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
#ifndef SRC_ASYNC_WRAP_INL_H_
#define SRC_ASYNC_WRAP_INL_H_

#include "cnode.h"
#include "cnode_internal.h"
#include "cenv.h"
#include "cenv-inl.h"
#include "cutil.h"
#include "cutil-inl.h"

#include "v8.h"

namespace node {
  using v8::Handle;
  using v8::Object;
  using v8::Value;
  using v8::String;
  using v8::Function;
  using v8::HandleScope;
  using v8::TryCatch;

  inline AsyncWrap::AsyncWrap(Environment* env, Handle<Object> object, ProviderType provider, AsyncWrap* parent)
  : BaseObject(env, object), has_async_queue_(false),  provider_type_(provider) {
    // Check user controlled flag to see if the init callback should run.
    if (!env->using_asyncwrap())
      return;
    if (!env->call_async_init_hook() && parent == NULL)
      return;

    // TODO(trevnorris): Until it's verified all passed object's are not weak,
    // add a HandleScope to make sure there's no leak.
    HandleScope scope(env->isolate());

    Local<Object> parent_obj;

    TryCatch try_catch;

    // If a parent value was sent then call its pre/post functions to let it know
    // a conceptual "child" is being instantiated (e.g. that a server has
    // received a connection).
    if (parent != NULL) {
      parent_obj = parent->object();
      env->async_hooks_pre_function()->Call(parent_obj, 0, NULL);
      if (try_catch.HasCaught())
        FatalError("node::AsyncWrap::AsyncWrap", "parent pre hook threw");
    }

    env->async_hooks_init_function()->Call(object, 0, NULL);

    if (try_catch.HasCaught())
      FatalError("node::AsyncWrap::AsyncWrap", "init hook threw");

    has_async_queue_ = true;

    if (parent != NULL) {
      env->async_hooks_post_function()->Call(parent_obj, 0, NULL);
      if (try_catch.HasCaught())
        FatalError("node::AsyncWrap::AsyncWrap", "parent post hook threw");
    }
  }


  inline AsyncWrap::~AsyncWrap() {
  }


  inline uint32_t AsyncWrap::provider_type() const {
    return provider_type_;
  }


  inline Handle<Value> AsyncWrap::MakeCallback(const Handle<String> symbol,int argc, Handle<Value>* argv) {
    Local<Value> cb_v = object()->Get(symbol);
    ASSERT(cb_v->IsFunction());
    return MakeCallback(cb_v.As<Function>(), argc, argv);
  }


  inline Handle<Value> AsyncWrap::MakeCallback( uint32_t index, int argc, Handle<Value>* argv) {
    Local<Value> cb_v = object()->Get(index);
    ASSERT(cb_v->IsFunction());
    return MakeCallback(cb_v.As<Function>(), argc, argv);
  }
}//End Node Namespace
#endif //SRC_ASYNC_WRAP_INL_H_
