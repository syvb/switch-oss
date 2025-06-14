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

#ifndef PolicyChecker_h
#define PolicyChecker_h

#include "FrameLoaderTypes.h"
#include "PolicyCallback.h"
#include "ResourceRequest.h"
#include <wtf/PassRefPtr.h>
#include <wtf/text/WTFString.h>

#if ENABLE(CONTENT_FILTERING)
#include "ContentFilterUnblockHandler.h"
#endif

#if PLATFORM(WKC)
#include "FrameLoaderClient.h"
#endif

namespace WebCore {

class DocumentLoader;
class FormState;
class Frame;
class NavigationAction;
class ResourceError;
class ResourceResponse;

class PolicyChecker {
    WTF_MAKE_NONCOPYABLE(PolicyChecker);
#if PLATFORM(WKC)
    WTF_MAKE_FAST_ALLOCATED;
#endif
public:
    explicit PolicyChecker(Frame&);

    void checkNavigationPolicy(const ResourceRequest&, DocumentLoader*, PassRefPtr<FormState>, NavigationPolicyDecisionFunction);
    void checkNavigationPolicy(const ResourceRequest&, NavigationPolicyDecisionFunction);
    void checkNewWindowPolicy(const NavigationAction&, const ResourceRequest&, PassRefPtr<FormState>, const String& frameName, NewWindowPolicyDecisionFunction);
    void checkContentPolicy(const ResourceResponse&, ContentPolicyDecisionFunction);

    // FIXME: These are different.  They could use better names.
    void cancelCheck();
    void stopCheck();

    void cannotShowMIMEType(const ResourceResponse&);

    FrameLoadType loadType() const { return m_loadType; }
    void setLoadType(FrameLoadType loadType) { m_loadType = loadType; }

    bool delegateIsDecidingNavigationPolicy() const { return m_delegateIsDecidingNavigationPolicy; }
    bool delegateIsHandlingUnimplementablePolicy() const { return m_delegateIsHandlingUnimplementablePolicy; }

    // FIXME: This function is a cheat.  Basically, this is just an asynchronouc callback
    // from the FrameLoaderClient, but this callback uses the policy types and so has to
    // live on this object.  In the long term, we should create a type for non-policy
    // callbacks from the FrameLoaderClient and remove this vestige.  I just don't have
    // the heart to hack on all the platforms to make that happen right now.
    void continueLoadAfterWillSubmitForm(PolicyAction);

#if ENABLE(CONTENT_FILTERING)
    void setContentFilterUnblockHandler(ContentFilterUnblockHandler unblockHandler) { m_contentFilterUnblockHandler = WTF::move(unblockHandler); }
#endif

private:
    void continueAfterNavigationPolicy(PolicyAction);
    void continueAfterNewWindowPolicy(PolicyAction);
    void continueAfterContentPolicy(PolicyAction);

    void handleUnimplementablePolicy(const ResourceError&);

    Frame& m_frame;

    bool m_delegateIsDecidingNavigationPolicy;
    bool m_delegateIsHandlingUnimplementablePolicy;

    // This identifies the type of navigation action which prompted this load. Note 
    // that WebKit conveys this value as the WebActionNavigationTypeKey value
    // on navigation action delegate callbacks.
    FrameLoadType m_loadType;
    PolicyCallback m_callback;

#if ENABLE(CONTENT_FILTERING)
    ContentFilterUnblockHandler m_contentFilterUnblockHandler;
#endif
};

} // namespace WebCore

#endif // PolicyChecker_h
