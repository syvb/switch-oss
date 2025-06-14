/*
 * Copyright (C) 2019 Apple Inc. All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
 *
 * THIS SOFTWARE IS PROVIDED BY APPLE INC. AND ITS CONTRIBUTORS ``AS IS''
 * AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO,
 * THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR
 * PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL APPLE INC. OR ITS CONTRIBUTORS
 * BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR
 * CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF
 * SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS
 * INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN
 * CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE)
 * ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF
 * THE POSSIBILITY OF SUCH DAMAGE.
 */

#pragma once
#if PLATFORM(WKC)

#include "ResourceLoaderIdentifier.h"
#include <wtf/Deque.h>
#include <wtf/Function.h>
#include <wtf/HashMap.h>

namespace WebCore {

class WebResourceInterceptController {
public:
    bool isIntercepting(WebCore::ResourceLoaderIdentifier) const;

    // Start intercepting a response.
    void beginInterceptingResponse(WebCore::ResourceLoaderIdentifier);

    // Stop intercepting a response. An intercept response was not supplied. Send deferred networking callbacks.
    void continueResponse(WebCore::ResourceLoaderIdentifier);

    // Stop intercepting a response. An intercept response was supplied. Send no deferred networking callbacks.
    void interceptedResponse(WebCore::ResourceLoaderIdentifier);

    void defer(WebCore::ResourceLoaderIdentifier, Function<void()>&&);

private:
    HashMap<WebCore::ResourceLoaderIdentifier, Deque<Function<void()>>> m_interceptedResponseQueue;
};

} // namespace WebCore
#endif
