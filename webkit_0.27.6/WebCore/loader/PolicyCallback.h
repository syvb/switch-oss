/*
 * Copyright (C) 2006, 2007, 2008, 2009 Apple Inc. All rights reserved.
 * Copyright (C) 2008, 2009 Torch Mobile Inc. All rights reserved. (http://www.torchmobile.com/)
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 *
 * 1.  Redistributions of source code must retain the above copyright
 *     notice, this list of conditions and the following disclaimer. 
 * 2.  Redistributions in binary form must reproduce the above copyright
 *     notice, this list of conditions and the following disclaimer in the
 *     documentation and/or other materials provided with the distribution. 
 * 3.  Neither the name of Apple Inc. ("Apple") nor the names of
 *     its contributors may be used to endorse or promote products derived
 *     from this software without specific prior written permission. 
 *
 * THIS SOFTWARE IS PROVIDED BY APPLE AND ITS CONTRIBUTORS "AS IS" AND ANY
 * EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED
 * WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE
 * DISCLAIMED. IN NO EVENT SHALL APPLE OR ITS CONTRIBUTORS BE LIABLE FOR ANY
 * DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES
 * (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES;
 * LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND
 * ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
 * (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF
 * THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */

#ifndef PolicyCallback_h
#define PolicyCallback_h

#include "FrameLoaderTypes.h"
#include "NavigationAction.h"
#include "ResourceRequest.h"
#include <functional>
#if PLATFORM(WKC)
#include <wtf/Function.h>
#endif
#include <wtf/RefPtr.h>
#include <wtf/text/WTFString.h>

namespace WebCore {

class FormState;

#if PLATFORM(WKC)
typedef WTF::Function<void (const ResourceRequest&, PassRefPtr<FormState>, bool shouldContinue)> NavigationPolicyDecisionFunction;
typedef WTF::Function<void (const ResourceRequest&, PassRefPtr<FormState>, const String& frameName, const NavigationAction&, bool shouldContinue)> NewWindowPolicyDecisionFunction;
typedef WTF::Function<void (PolicyAction)> ContentPolicyDecisionFunction;
#else
typedef std::function<void (const ResourceRequest&, PassRefPtr<FormState>, bool shouldContinue)> NavigationPolicyDecisionFunction;
typedef std::function<void (const ResourceRequest&, PassRefPtr<FormState>, const String& frameName, const NavigationAction&, bool shouldContinue)> NewWindowPolicyDecisionFunction;
typedef std::function<void (PolicyAction)> ContentPolicyDecisionFunction;
#endif

class PolicyCallback {
#if PLATFORM(WKC)
    WTF_MAKE_FAST_ALLOCATED;
#endif
public:
    PolicyCallback();
    ~PolicyCallback();

#if PLATFORM(WKC)
    PolicyCallback(PolicyCallback&&) = default;
#endif

    void clear();
    void set(const ResourceRequest&, PassRefPtr<FormState>, NavigationPolicyDecisionFunction);
    void set(const ResourceRequest&, PassRefPtr<FormState>, const String& frameName, const NavigationAction&, NewWindowPolicyDecisionFunction);
    void set(ContentPolicyDecisionFunction);

    const ResourceRequest& request() const { return m_request; }
    void clearRequest();

    void call(bool shouldContinue);
    void call(PolicyAction);
    void cancel();

private:
    ResourceRequest m_request;
    RefPtr<FormState> m_formState;
    String m_frameName;
    NavigationAction m_navigationAction;

    NavigationPolicyDecisionFunction m_navigationFunction;
    NewWindowPolicyDecisionFunction m_newWindowFunction;
    ContentPolicyDecisionFunction m_contentFunction;
};

} // namespace WebCore

#endif // PolicyCallback_h
