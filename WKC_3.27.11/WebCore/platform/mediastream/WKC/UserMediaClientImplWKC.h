/*
 * Copyright (C) 2011 Google Inc. All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions are
 * met:
 *
 *     * Redistributions of source code must retain the above copyright
 * notice, this list of conditions and the following disclaimer.
 *     * Redistributions in binary form must reproduce the above
 * copyright notice, this list of conditions and the following disclaimer
 * in the documentation and/or other materials provided with the
 * distribution.
 *     * Neither the name of Google Inc. nor the names of its
 * contributors may be used to endorse or promote products derived from
 * this software without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS
 * "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT
 * LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR
 * A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT
 * OWNER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL,
 * SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT
 * LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE,
 * DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY
 * THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
 * (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
 * OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */

#ifndef UserMediaClientImpl_h
#define UserMediaClientImpl_h

#if ENABLE(MEDIA_STREAM)

#include "UserMediaClient.h"

#include "RealtimeMediaSourceCenter.h"
#include "UserMediaRequest.h"
#include <wtf/PassRefPtr.h>

namespace WKC {
class WKCWebViewPrivate;

class UserMediaClientWKC : public WebCore::UserMediaClient {
    WTF_MAKE_FAST_ALLOCATED;
public:
    static UserMediaClientWKC* create(WKCWebViewPrivate*);
    virtual ~UserMediaClientWKC();

    // WebCore::UserMediaClient ----------------------------------------------
    virtual void pageDestroyed();
    virtual void requestPermission(WTF::Ref<WebCore::UserMediaRequest>&&);
    virtual void cancelRequest(WebCore::UserMediaRequest&);

private:
    UserMediaClientWKC(WKCWebViewPrivate*);
    bool construct();

private:
    WKCWebViewPrivate* m_view;
};

} // namespace

#endif // ENABLE(MEDIA_STREAM)

#endif // UserMediaClientImplWKC_h
