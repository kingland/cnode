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
#ifndef SRC_NODE_WRAP_H_
#define SRC_NODE_WRAP_H_

#include "cenv.h"
#include "cenv-inl.h"
#include "cpipe_wrap.h"
#include "ctcp_wrap.h"
#include "ctty_wrap.h"
#include "cudp_wrap.h"
#include "cutil.h"
#include "cutil-inl.h"

#include "uv.h"
#include "v8.h"

namespace node {
  #define WITH_GENERIC_STREAM(env, obj, BODY)                                 \
    do {                                                                      \
      if (env->tcp_constructor_template().IsEmpty() == false &&               \
          env->tcp_constructor_template()->HasInstance(obj)) {                \
        TCPWrap* const wrap = Unwrap<TCPWrap>(obj);                           \
        BODY                                                                  \
      } else if (env->tty_constructor_template().IsEmpty() == false &&        \
                 env->tty_constructor_template()->HasInstance(obj)) {         \
        TTYWrap* const wrap = Unwrap<TTYWrap>(obj);                           \
        BODY                                                                  \
      } else if (env->pipe_constructor_template().IsEmpty() == false &&       \
                 env->pipe_constructor_template()->HasInstance(obj)) {        \
        PipeWrap* const wrap = Unwrap<PipeWrap>(obj);                         \
        BODY                                                                  \
      }                                                                       \
    } while (0)

  inline uv_stream_t* HandleToStream(Environment* env,
                                     v8::Local<v8::Object> obj) {
    v8::HandleScope scope(env->isolate());

    WITH_GENERIC_STREAM(env, obj, {
      return reinterpret_cast<uv_stream_t*>(wrap->UVHandle());
    });

    return NULL;
  }

}

#endif //SRC_NODE_WRAP_H_
