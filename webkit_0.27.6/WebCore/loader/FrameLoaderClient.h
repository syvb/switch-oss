/*
 * Copyright (C) 2006, 2007, 2008, 2009, 2010, 2011, 2012 Apple Inc. All rights reserved.
 * Copyright (C) 2012 Google Inc. All rights reserved.
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

#ifndef FrameLoaderClient_h
#define FrameLoaderClient_h

#include "FrameLoaderTypes.h"
#include "IconURL.h"
#include "LayoutMilestones.h"
#include "ResourceLoadPriority.h"
#include <functional>
#if PLATFORM(WKC)
#include <wtf/Function.h>
#endif
#include <wtf/Forward.h>
#include <wtf/Vector.h>
#include <wtf/text/WTFString.h>
#include <wtf/Optional.h>

#if ENABLE(CONTENT_FILTERING)
#include "ContentFilterUnblockHandler.h"
#endif

#if PLATFORM(COCOA)
#ifdef __OBJC__ 
#import <Foundation/Foundation.h>
typedef id RemoteAXObjectRef;
#else
typedef void* RemoteAXObjectRef;
#endif
#endif

typedef class _jobject* jobject;

#if PLATFORM(COCOA)
OBJC_CLASS NSCachedURLResponse;
OBJC_CLASS NSView;
#endif

namespace WebCore {

    class AuthenticationChallenge;
    class CachedFrame;
    class CachedResourceRequest;
    class Color;
    class DOMWindowExtension;
    class DOMWrapperWorld;
    class DocumentLoader;
    class Element;
    class FormState;
    class Frame;
    class FrameLoader;
    class FrameNetworkingContext;
    class HistoryItem;
    class HTMLAppletElement;
    class HTMLFormElement;
    class HTMLFrameOwnerElement;
    class HTMLPlugInElement;
    class IntSize;
    class URL;
    class MessageEvent;
    class NavigationAction;
    class Page;
    class ProtectionSpace;
    class PluginViewBase;
    class PolicyChecker;
    class ResourceError;
    class ResourceHandle;
    class ResourceRequest;
    class ResourceResponse;
#if ENABLE(MEDIA_STREAM)
    class RTCPeerConnectionHandler;
#endif
    class SecurityOrigin;
    class SharedBuffer;
    class StringWithDirection;
    class SubstituteData;
    class Widget;

#if USE(QUICK_LOOK)
    class QuickLookHandle;
#endif

#if PLATFORM(WKC)
    typedef WTF::Function<void (PolicyAction)> FramePolicyFunction;
#else
    typedef std::function<void (PolicyAction)> FramePolicyFunction;
#endif

    class WEBCORE_EXPORT FrameLoaderClient {
    public:
        // An inline function cannot be the first non-abstract virtual function declared
        // in the class as it results in the vtable being generated as a weak symbol.
        // This hurts performance (in Mac OS X at least, when loadig frameworks), so we
        // don't want to do it in WebKit.
        virtual bool hasHTMLView() const;

        virtual ~FrameLoaderClient() { }

        virtual void frameLoaderDestroyed() = 0;

        virtual bool hasWebView() const = 0; // mainly for assertions

        virtual void makeRepresentation(DocumentLoader*) = 0;
        
#if PLATFORM(IOS)
        // Returns true if the client forced the layout.
        virtual bool forceLayoutOnRestoreFromPageCache() = 0;
#endif
        virtual void forceLayoutForNonHTML() = 0;

        virtual void setCopiesOnScroll() = 0;

        virtual void detachedFromParent2() = 0;
        virtual void detachedFromParent3() = 0;

        virtual void assignIdentifierToInitialRequest(unsigned long identifier, DocumentLoader*, const ResourceRequest&) = 0;

        virtual void dispatchWillSendRequest(DocumentLoader*, unsigned long identifier, ResourceRequest&, const ResourceResponse& redirectResponse) = 0;
        virtual bool shouldUseCredentialStorage(DocumentLoader*, unsigned long identifier) = 0;
        virtual void dispatchDidReceiveAuthenticationChallenge(DocumentLoader*, unsigned long identifier, const AuthenticationChallenge&) = 0;
        virtual void dispatchDidCancelAuthenticationChallenge(DocumentLoader*, unsigned long identifier, const AuthenticationChallenge&) = 0;        
#if USE(PROTECTION_SPACE_AUTH_CALLBACK)
        virtual bool canAuthenticateAgainstProtectionSpace(DocumentLoader*, unsigned long identifier, const ProtectionSpace&) = 0;
#endif

#if PLATFORM(IOS)
        virtual RetainPtr<CFDictionaryRef> connectionProperties(DocumentLoader*, unsigned long identifier) = 0;
#endif

        virtual void dispatchDidReceiveResponse(DocumentLoader*, unsigned long identifier, const ResourceResponse&) = 0;
        virtual void dispatchDidReceiveContentLength(DocumentLoader*, unsigned long identifier, int dataLength) = 0;
        virtual void dispatchDidFinishLoading(DocumentLoader*, unsigned long identifier) = 0;
        virtual void dispatchDidFailLoading(DocumentLoader*, unsigned long identifier, const ResourceError&) = 0;
        virtual bool dispatchDidLoadResourceFromMemoryCache(DocumentLoader*, const ResourceRequest&, const ResourceResponse&, int length) = 0;

        virtual void dispatchDidHandleOnloadEvents() = 0;
        virtual void dispatchDidReceiveServerRedirectForProvisionalLoad() = 0;
        virtual void dispatchDidChangeProvisionalURL() { }
        virtual void dispatchDidCancelClientRedirect() = 0;
        virtual void dispatchWillPerformClientRedirect(const URL&, double interval, double fireDate) = 0;
        virtual void dispatchDidPerformClientRedirect() { }
        virtual void dispatchDidNavigateWithinPage() { }
        virtual void dispatchDidChangeLocationWithinPage() = 0;
        virtual void dispatchDidPushStateWithinPage() = 0;
        virtual void dispatchDidReplaceStateWithinPage() = 0;
        virtual void dispatchDidPopStateWithinPage() = 0;
        virtual void dispatchWillClose() = 0;
        virtual void dispatchDidReceiveIcon() = 0;
        virtual void dispatchDidStartProvisionalLoad() = 0;
        virtual void dispatchDidReceiveTitle(const StringWithDirection&) = 0;
        virtual void dispatchDidChangeIcons(IconType) = 0;
        virtual void dispatchDidCommitLoad(Optional<HasInsecureContent>) = 0;
        virtual void dispatchDidFailProvisionalLoad(const ResourceError&) = 0;
        virtual void dispatchDidFailLoad(const ResourceError&) = 0;
        virtual void dispatchDidFinishDocumentLoad() = 0;
        virtual void dispatchDidFinishLoad() = 0;

        virtual void dispatchDidLayout() { }
        virtual void dispatchDidReachLayoutMilestone(LayoutMilestones) { }

        virtual Frame* dispatchCreatePage(const NavigationAction&) = 0;
        virtual void dispatchShow() = 0;

        virtual void dispatchDecidePolicyForResponse(const ResourceResponse&, const ResourceRequest&, FramePolicyFunction) = 0;
        virtual void dispatchDecidePolicyForNewWindowAction(const NavigationAction&, const ResourceRequest&, PassRefPtr<FormState>, const String& frameName, FramePolicyFunction) = 0;
        virtual void dispatchDecidePolicyForNavigationAction(const NavigationAction&, const ResourceRequest&, PassRefPtr<FormState>, FramePolicyFunction) = 0;
        virtual void cancelPolicyCheck() = 0;

        virtual void dispatchUnableToImplementPolicy(const ResourceError&) = 0;

        virtual void dispatchWillSendSubmitEvent(PassRefPtr<FormState>) = 0;
        virtual void dispatchWillSubmitForm(PassRefPtr<FormState>, FramePolicyFunction) = 0;

        virtual void revertToProvisionalState(DocumentLoader*) = 0;
        virtual void setMainDocumentError(DocumentLoader*, const ResourceError&) = 0;

        virtual void setMainFrameDocumentReady(bool) = 0;

        virtual void startDownload(const ResourceRequest&, const String& suggestedName = String()) = 0;

        virtual void willChangeTitle(DocumentLoader*) = 0;
        virtual void didChangeTitle(DocumentLoader*) = 0;

        virtual void willReplaceMultipartContent() = 0;
        virtual void didReplaceMultipartContent() = 0;

        virtual void committedLoad(DocumentLoader*, const char*, int) = 0;
        virtual void finishedLoading(DocumentLoader*) = 0;
        
        virtual void updateGlobalHistory() = 0;
        virtual void updateGlobalHistoryRedirectLinks() = 0;

        virtual bool shouldGoToHistoryItem(HistoryItem*) const = 0;
        virtual void updateGlobalHistoryItemForPage() { }

        // This frame has set its opener to null, disowning it for the lifetime of the frame.
        // See http://html.spec.whatwg.org/#dom-opener.
        // FIXME: JSC should allow disowning opener. - <https://bugs.webkit.org/show_bug.cgi?id=103913>.
        virtual void didDisownOpener() { }

        // This frame has displayed inactive content (such as an image) from an
        // insecure source.  Inactive content cannot spread to other frames.
        virtual void didDisplayInsecureContent() = 0;

        // The indicated security origin has run active content (such as a
        // script) from an insecure source.  Note that the insecure content can
        // spread to other frames in the same origin.
        virtual void didRunInsecureContent(SecurityOrigin&, const URL&) = 0;
        virtual void didDetectXSS(const URL&, bool didBlockEntirePage) = 0;

        virtual ResourceError cancelledError(const ResourceRequest&) = 0;
        virtual ResourceError blockedError(const ResourceRequest&) = 0;
        virtual ResourceError cannotShowURLError(const ResourceRequest&) = 0;
        virtual ResourceError interruptedForPolicyChangeError(const ResourceRequest&) = 0;

        virtual ResourceError cannotShowMIMETypeError(const ResourceResponse&) = 0;
        virtual ResourceError fileDoesNotExistError(const ResourceResponse&) = 0;
        virtual ResourceError pluginWillHandleLoadError(const ResourceResponse&) = 0;

        virtual bool shouldFallBack(const ResourceError&) = 0;

        virtual bool canHandleRequest(const ResourceRequest&) const = 0;
        virtual bool canShowMIMEType(const String& MIMEType) const = 0;
        virtual bool canShowMIMETypeAsHTML(const String& MIMEType) const = 0;
        virtual bool representationExistsForURLScheme(const String& URLScheme) const = 0;
        virtual String generatedMIMETypeForURLScheme(const String& URLScheme) const = 0;

        virtual void frameLoadCompleted() = 0;
        virtual void saveViewStateToItem(HistoryItem*) = 0;
        virtual void restoreViewState() = 0;
        virtual void provisionalLoadStarted() = 0;
        virtual void didFinishLoad() = 0;
        virtual void prepareForDataSourceReplacement() = 0;

        virtual PassRefPtr<DocumentLoader> createDocumentLoader(const ResourceRequest&, const SubstituteData&) = 0;
        virtual void updateCachedDocumentLoader(DocumentLoader&) = 0;
        virtual void setTitle(const StringWithDirection&, const URL&) = 0;

        virtual String userAgent(const URL&) = 0;
        
        virtual void savePlatformDataToCachedFrame(CachedFrame*) = 0;
        virtual void transitionToCommittedFromCachedFrame(CachedFrame*) = 0;
#if PLATFORM(IOS)
        virtual void didRestoreFrameHierarchyForCachedFrame() = 0;
#endif
        virtual void transitionToCommittedForNewPage() = 0;

        virtual void didSaveToPageCache() = 0;
        virtual void didRestoreFromPageCache() = 0;

        virtual void dispatchDidBecomeFrameset(bool) = 0; // Can change due to navigation or DOM modification.

        virtual bool canCachePage() const = 0;
        virtual void convertMainResourceLoadToDownload(DocumentLoader*, const ResourceRequest&, const ResourceResponse&) = 0;

        virtual PassRefPtr<Frame> createFrame(const URL& url, const String& name, HTMLFrameOwnerElement* ownerElement, const String& referrer, bool allowsScrolling, int marginWidth, int marginHeight) = 0;
#if ENABLE(NETSCAPE_PLUGIN_API)
        virtual PassRefPtr<Widget> createPlugin(const IntSize&, HTMLPlugInElement*, const URL&, const Vector<String>&, const Vector<String>&, const String&, bool loadManually) = 0;
        virtual void recreatePlugin(Widget*) = 0;
        virtual void redirectDataToPlugin(Widget* pluginWidget) = 0;
#endif

        virtual PassRefPtr<Widget> createJavaAppletWidget(const IntSize&, HTMLAppletElement*, const URL& baseURL, const Vector<String>& paramNames, const Vector<String>& paramValues) = 0;

        virtual void dispatchDidFailToStartPlugin(const PluginViewBase*) const { }

        virtual ObjectContentType objectContentType(const URL&, const String& mimeType, bool shouldPreferPlugInsForImages) = 0;
        virtual String overrideMediaType() const = 0;

        virtual void dispatchDidClearWindowObjectInWorld(DOMWrapperWorld&) = 0;

        virtual void registerForIconNotification(bool listen = true) = 0;

#if PLATFORM(COCOA)
        // Allow an accessibility object to retrieve a Frame parent if there's no PlatformWidget.
        virtual RemoteAXObjectRef accessibilityRemoteObject() = 0;
        virtual NSCachedURLResponse* willCacheResponse(DocumentLoader*, unsigned long identifier, NSCachedURLResponse*) const = 0;
#endif
#if PLATFORM(WIN) && USE(CFNETWORK)
        // FIXME: Windows should use willCacheResponse - <https://bugs.webkit.org/show_bug.cgi?id=57257>.
        virtual bool shouldCacheResponse(DocumentLoader*, unsigned long identifier, const ResourceResponse&, const unsigned char* data, unsigned long long length) = 0;
#endif

        virtual bool shouldAlwaysUsePluginDocument(const String& /*mimeType*/) const { return false; }
        virtual bool shouldLoadMediaElementURL(const URL&) const { return true; }

        virtual void didChangeScrollOffset() { }

        virtual bool allowScript(bool enabledPerSettings) { return enabledPerSettings; }

        // Clients that generally disallow universal access can make exceptions for particular URLs.
        virtual bool shouldForceUniversalAccessFromLocalURL(const URL&) { return false; }

        virtual PassRefPtr<FrameNetworkingContext> createNetworkingContext() = 0;

#if ENABLE(REQUEST_AUTOCOMPLETE)
        virtual void didRequestAutocomplete(PassRefPtr<FormState>) = 0;
#endif

        virtual bool shouldPaintBrokenImage(const URL&) const { return true; }

        virtual void dispatchGlobalObjectAvailable(DOMWrapperWorld&) { }
        virtual void dispatchWillDisconnectDOMWindowExtensionFromGlobalObject(DOMWindowExtension*) { }
        virtual void dispatchDidReconnectDOMWindowExtensionToGlobalObject(DOMWindowExtension*) { }
        virtual void dispatchWillDestroyGlobalObjectForDOMWindowExtension(DOMWindowExtension*) { }

#if ENABLE(MEDIA_STREAM)
        virtual void dispatchWillStartUsingPeerConnectionHandler(RTCPeerConnectionHandler*) { }
#endif

#if ENABLE(WEBGL)
        virtual bool allowWebGL(bool enabledPerSettings) { return enabledPerSettings; }
        // Informs the embedder that a WebGL canvas inside this frame received a lost context
        // notification with the given GL_ARB_robustness guilt/innocence code (see Extensions3D.h).
        virtual void didLoseWebGLContext(int) { }
        virtual WebGLLoadPolicy webGLPolicyForURL(const String&) const { return WebGLAllowCreation; }
        virtual WebGLLoadPolicy resolveWebGLPolicyForURL(const String&) const { return WebGLAllowCreation; }
#endif

        virtual void forcePageTransitionIfNeeded() { }

        // FIXME (bug 116233): We need to get rid of EmptyFrameLoaderClient completely, then this will no longer be needed.
        virtual bool isEmptyFrameLoaderClient() { return false; }

        virtual FrameLoader* dataProtocolLoader() const { return nullptr; }

#if USE(QUICK_LOOK)
        virtual void didCreateQuickLookHandle(QuickLookHandle&) { }
#endif

#if ENABLE(CONTENT_FILTERING)
        virtual void contentFilterDidBlockLoad(ContentFilterUnblockHandler) { }
#endif
#if PLATFORM(WKC)
        virtual bool byWKC() { return false; }
#endif
    };

} // namespace WebCore

#endif // FrameLoaderClient_h
