/*
 *  Copyright (C) 2007, 2008 Alp Toker <alp@atoker.com>
 *  Copyright (C) 2007, 2008, 2009 Holger Hans Peter Freyther
 *  Copyright (C) 2007 Christian Dywan <christian@twotoasts.de>
 *  Copyright (C) 2008, 2009 Collabora Ltd.  All rights reserved.
 *  Copyright (C) 2009 Gustavo Noronha Silva <gns@gnome.org>
 *  Copyright (C) Research In Motion Limited 2009. All rights reserved.
 *
 *  This library is free software; you can redistribute it and/or
 *  modify it under the terms of the GNU Lesser General Public
 *  License as published by the Free Software Foundation; either
 *  version 2 of the License, or (at your option) any later version.
 *
 *  This library is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 *  Lesser General Public License for more details.
 *
 *  You should have received a copy of the GNU Lesser General Public
 *  License along with this library; if not, write to the Free Software
 *  Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301  USA
 */

#include "config.h"

#include "EventHandler.h"
#include "FrameLoaderClientWKC.h"
#include "FrameLoader.h"
#include "FrameLoaderTypes.h"
#include "FrameView.h"
#include "DocumentLoader.h"
#include "DocumentWriter.h"
#include "IntRect.h"
#include "WTFString.h"
#include "SharedBuffer.h"
#include "NetworkStorageSession.h"

#include "WKCWebView.h"
#include "WKCWebViewPrivate.h"
#include "WKCWebFrame.h"
#include "WKCWebFramePrivate.h"

#include "helpers/FrameLoaderClientIf.h"

#include "helpers/WKCFrame.h"
#include "helpers/WKCFrameLoaderTypes.h"
#include "helpers/WKCKURL.h"
#include "helpers/WKCString.h"

#ifdef WKC_ENABLE_CUSTOMJS
#include "helpers/WKCSettings.h"
#include <APICast.h>
#include <JSStringRef.h>
#include <JSObjectRef.h>
#include "CustomJS/privates/CustomJSCallBackWKC.h"
#endif // WKC_ENABLE_CUSTOMJS

#include "helpers/privates/WKCAuthenticationChallengePrivate.h"
#include "helpers/privates/WKCCachedFramePrivate.h"
#include "helpers/privates/WKCCertificatePrivate.h"
#include "helpers/privates/WKCDOMWrapperWorldPrivate.h"
#include "helpers/privates/WKCDocumentLoaderPrivate.h"
#include "helpers/privates/WKCFormStatePrivate.h"
#include "helpers/privates/WKCFramePrivate.h"
#include "helpers/privates/WKCHelpersEnumsPrivate.h"
#include "helpers/privates/WKCHTMLFrameOwnerElementPrivate.h"
#include "helpers/privates/WKCHistoryItemPrivate.h"
#include "helpers/privates/WKCPagePrivate.h"
#include "helpers/privates/WKCResourceErrorPrivate.h"
#include "helpers/privates/WKCResourceLoaderPrivate.h"
#include "helpers/privates/WKCNavigationActionPrivate.h"
#include "helpers/privates/WKCResourceRequestPrivate.h"
#include "helpers/privates/WKCResourceResponsePrivate.h"
#include "helpers/privates/WKCSecurityOriginPrivate.h"

#include "NotImplemented.h"

// implementations

namespace WKC {

class FramePolicyFunctionPrivate : public FramePolicyFunction
{
WTF_MAKE_FAST_ALLOCATED;
public:
    FramePolicyFunctionPrivate(void* parent, void* func)
        : FramePolicyFunction(parent, func)
    {}
    ~FramePolicyFunctionPrivate()
    {}
};

FrameLoaderClientWKC::FrameLoaderClientWKC(WKCWebFramePrivate* frame)
     : m_frame(frame),
       m_appClient(0)
{
#if ENABLE(NETSCAPE_PLUGIN_API)
    m_hasSentResponseToPlugin = false;
    m_pluginView = 0;
#endif
    m_policyDecision = 0;
}
FrameLoaderClientWKC::~FrameLoaderClientWKC()
{
    if (m_appClient) {
        if (m_frame) {
            m_frame->clientBuilders().deleteFrameLoaderClient(m_appClient);
        }
        m_appClient = 0;
    }
}

FrameLoaderClientWKC*
FrameLoaderClientWKC::create(WKCWebFramePrivate* frame)
{
    FrameLoaderClientWKC* self = 0;
    self = new FrameLoaderClientWKC(frame);
    if (!self) return 0;
    if (!self->construct()) {
        delete self;
        return 0;
    }
    return self;
}

bool
FrameLoaderClientWKC::construct()
{
    if (!m_frame)
        return true;
    m_appClient = m_frame->clientBuilders().createFrameLoaderClient(m_frame->parent());
    if (!m_appClient) return false;
    return true;
}

bool
FrameLoaderClientWKC::setMainFrame(WKCWebFramePrivate* frame)
{
    m_frame = frame;
    return construct();
}

void
FrameLoaderClientWKC::notifyStatus(WKC::LoadStatus loadStatus)
{
    m_frame->m_loadStatus = loadStatus;
    WKCWebViewPrivate* webView = m_frame->m_view;
    if (m_frame->m_parent == webView->m_mainFrame) {
        webView->m_loadStatus = loadStatus;
    }
}

RefPtr<WebCore::Widget> FrameLoaderClientWKC::createPlugin(const WebCore::IntSize&, WebCore::HTMLPlugInElement&, const URL&, const Vector<WTF::String>&, const Vector<WTF::String>&, const WTF::String&, bool loadManually)
{
    notImplemented();
    return nullptr;
}

void FrameLoaderClientWKC::redirectDataToPlugin(WebCore::Widget&)
{
    notImplemented();
}

void FrameLoaderClientWKC::sendH2Ping(const URL&, CompletionHandler<void(Expected<Seconds, WebCore::ResourceError>&&)>&&)
{
    notImplemented();
}

WKCWebFrame*
FrameLoaderClientWKC::webFrame() const
{
    return m_frame->parent();
}

bool
FrameLoaderClientWKC::hasWebView() const
{
    return true;
}

// callbacks

void
FrameLoaderClientWKC::makeRepresentation(WebCore::DocumentLoader* loader)
{
    DocumentLoaderPrivate ldr(loader);
    m_appClient->makeRepresentation(&ldr.wkc());
}

std::optional<WebCore::PageIdentifier>
FrameLoaderClientWKC::pageID() const
{
    notImplemented();
    return std::nullopt;
}

std::optional<WebCore::FrameIdentifier>
FrameLoaderClientWKC::frameID() const
{
    notImplemented();
    return std::nullopt;
}

void
FrameLoaderClientWKC::forceLayoutForNonHTML()
{
    m_appClient->forceLayoutForNonHTML();
}


void
FrameLoaderClientWKC::setCopiesOnScroll()
{
    m_appClient->setCopiesOnScroll();
}


void
FrameLoaderClientWKC::detachedFromParent2()
{
    m_appClient->detachedFromParent2();

    WebCore::FrameLoader* frameLoader = &m_frame->core()->loader();
    if (!frameLoader)
        return;
    if (!m_frame->core() || !m_frame->core()->loader().activeDocumentLoader())
        return;

    WebCore::DocumentLoader* documentLoader = frameLoader->activeDocumentLoader();
    if (!documentLoader)
        return;

    if (frameLoader->state() == WebCore::FrameState::CommittedPage)
        documentLoader->mainReceivedError(frameLoader->cancelledError(documentLoader->request()));
}

void
FrameLoaderClientWKC::detachedFromParent3()
{
    m_appClient->detachedFromParent3();
    // If we are pan-scrolling when frame is detached, stop the pan scrolling.
    m_frame->core()->eventHandler().stopAutoscrollTimer();
}


void
FrameLoaderClientWKC::assignIdentifierToInitialRequest(WebCore::ResourceLoaderIdentifier identifier, WebCore::DocumentLoader* loader, const WebCore::ResourceRequest& resource)
{
    ResourceRequestPrivate req(resource);
    DocumentLoaderPrivate ldr(loader);
    m_appClient->assignIdentifierToInitialRequest(identifier.toUInt64(), &ldr.wkc(), req.wkc());
}


void
FrameLoaderClientWKC::dispatchWillSendRequest(WebCore::DocumentLoader* loader, WebCore::ResourceLoaderIdentifier identifier, WebCore::ResourceRequest& request, const WebCore::ResourceResponse& redirect_response)
{
    DocumentLoaderPrivate ldr(loader);
    ResourceRequestPrivate req(request);
    ResourceResponsePrivate res(redirect_response);
    m_appClient->dispatchWillSendRequest(&ldr.wkc(), identifier.toUInt64(), req.wkc(), res.wkc());
}

bool
FrameLoaderClientWKC::shouldUseCredentialStorage(WebCore::DocumentLoader* loader, WebCore::ResourceLoaderIdentifier identifier)
{
    bool ret = false;
    DocumentLoaderPrivate ldr(loader);
    ret = m_appClient->shouldUseCredentialStorage(&ldr.wkc(), identifier.toUInt64());
    return ret;
}

void
FrameLoaderClientWKC::dispatchDidReceiveAuthenticationChallenge(WebCore::DocumentLoader* loader, WebCore::ResourceLoaderIdentifier identifier, const WebCore::AuthenticationChallenge& challenge)
{
    DocumentLoaderPrivate ldr(loader);
    AuthenticationChallengePrivate wc(challenge);
    m_appClient->dispatchDidReceiveAuthenticationChallenge(&ldr.wkc(), identifier.toUInt64(), wc.wkc());
}

void
FrameLoaderClientWKC::dispatchDidReceiveResponse(WebCore::DocumentLoader* loader, WebCore::ResourceLoaderIdentifier identifier, const WebCore::ResourceResponse& response)
{
    DocumentLoaderPrivate ldr(loader);
    ResourceResponsePrivate res(response);
    m_appClient->dispatchDidReceiveResponse(&ldr.wkc(), identifier.toUInt64(), res.wkc());
}

void
FrameLoaderClientWKC::dispatchDidReceiveContentLength(WebCore::DocumentLoader* loader, WebCore::ResourceLoaderIdentifier identifier, int lengthReceived)
{
    DocumentLoaderPrivate ldr(loader);
    m_appClient->dispatchDidReceiveContentLength(&ldr.wkc(), identifier.toUInt64(), lengthReceived);
}

void
FrameLoaderClientWKC::dispatchDidFinishLoading(WebCore::DocumentLoader* loader, WebCore::ResourceLoaderIdentifier identifier)
{
    DocumentLoaderPrivate ldr(loader);
    m_appClient->dispatchDidFinishLoading(&ldr.wkc(), identifier.toUInt64());
}

void
FrameLoaderClientWKC::dispatchDidFailLoading(WebCore::DocumentLoader* loader, WebCore::ResourceLoaderIdentifier identifier, const WebCore::ResourceError& error)
{
    DocumentLoaderPrivate ldr(loader);
    ResourceErrorPrivate wobj(error);
    m_appClient->dispatchDidFailLoading(&ldr.wkc(), identifier.toUInt64(), wobj.wkc());
}

bool
FrameLoaderClientWKC::dispatchDidLoadResourceFromMemoryCache(WebCore::DocumentLoader* loader, const WebCore::ResourceRequest& request, const WebCore::ResourceResponse& response, int length)
{
    bool ret = false;
    DocumentLoaderPrivate ldr(loader);
    ResourceRequestPrivate req(request);
    ResourceResponsePrivate res(response);
    ret = m_appClient->dispatchDidLoadResourceFromMemoryCache(&ldr.wkc(), req.wkc(), res.wkc(), length);
    return ret;
}

void
FrameLoaderClientWKC::dispatchDidDispatchOnloadEvents()
{
    m_appClient->dispatchDidDispatchOnloadEvents();
}

void
FrameLoaderClientWKC::dispatchDidReceiveServerRedirectForProvisionalLoad()
{
    m_appClient->dispatchDidReceiveServerRedirectForProvisionalLoad();
}

void
FrameLoaderClientWKC::dispatchDidCancelClientRedirect()
{
    m_appClient->dispatchDidCancelClientRedirect();
}

void
FrameLoaderClientWKC::dispatchWillPerformClientRedirect(const WTF::URL& uri, double a, WallTime b, WebCore::LockBackForwardList)
{
    m_appClient->dispatchWillPerformClientRedirect(uri, a, b.secondsSinceEpoch().value());
}

void
FrameLoaderClientWKC::dispatchDidChangeLocationWithinPage()
{
    if (m_frame->m_uri) {
        fastFree(m_frame->m_uri);
        m_frame->m_uri = 0;
    }
    if (!m_frame->core() || !m_frame->core()->loader().activeDocumentLoader())
        return;

    m_frame->m_uri = wkc_strdup(m_frame->core()->loader().activeDocumentLoader()->url().string().utf8().data());

    m_appClient->dispatchDidChangeLocationWithinPage();
}

void
FrameLoaderClientWKC::dispatchDidNavigateWithinPage()
{
    if (m_frame->m_uri) {
        fastFree(m_frame->m_uri);
        m_frame->m_uri = 0;
    }
    if (!m_frame->core() || !m_frame->core()->loader().activeDocumentLoader())
        return;

    m_frame->m_uri = wkc_strdup(m_frame->core()->loader().activeDocumentLoader()->url().string().utf8().data());

    m_appClient->dispatchDidNavigateWithinPage();
}

void
FrameLoaderClientWKC::dispatchDidPushStateWithinPage()
{
    if (!m_frame)
        return;

    if (m_frame->m_uri) {
        fastFree(m_frame->m_uri);
        m_frame->m_uri = 0;
    }
    if (!m_frame->core() || !m_frame->core()->loader().activeDocumentLoader())
        return;

    m_frame->m_uri = wkc_strdup(m_frame->core()->loader().activeDocumentLoader()->url().string().utf8().data());

    m_appClient->dispatchDidPushStateWithinPage();
}

void
FrameLoaderClientWKC::dispatchDidReplaceStateWithinPage()
{
    if (m_frame->m_uri) {
        fastFree(m_frame->m_uri);
        m_frame->m_uri = 0;
    }
    if (!m_frame->core() || !m_frame->core()->loader().activeDocumentLoader())
        return;

    m_frame->m_uri = wkc_strdup(m_frame->core()->loader().activeDocumentLoader()->url().string().utf8().data());

    m_appClient->dispatchDidReplaceStateWithinPage();
}

void
FrameLoaderClientWKC::dispatchDidPopStateWithinPage()
{
    if (m_frame->m_uri) {
        fastFree(m_frame->m_uri);
        m_frame->m_uri = 0;
    }
    if (!m_frame->core() || !m_frame->core()->loader().activeDocumentLoader())
        return;

    m_frame->m_uri = wkc_strdup(m_frame->core()->loader().activeDocumentLoader()->url().string().utf8().data());

    m_appClient->dispatchDidPopStateWithinPage();
}

void
FrameLoaderClientWKC::dispatchWillClose()
{
    m_appClient->dispatchWillClose();
}

void
FrameLoaderClientWKC::dispatchDidReceiveIcon()
{
    m_appClient->dispatchDidReceiveIcon();
}

void
FrameLoaderClientWKC::dispatchDidStartProvisionalLoad()
{
    if (m_frame->m_uri) {
        fastFree(m_frame->m_uri);
        m_frame->m_uri = 0;
    }
    if (!m_frame->core() || !m_frame->core()->loader().activeDocumentLoader())
        return;

    m_frame->m_uri = wkc_strdup(m_frame->core()->loader().activeDocumentLoader()->url().string().utf8().data());

    m_appClient->dispatchDidStartProvisionalLoad();
    notifyStatus(WKC::ELoadStatusProvisional);
}

void
FrameLoaderClientWKC::dispatchDidReceiveTitle(const WebCore::StringWithDirection& title)
{
    WKC::String str(title.string);
    str.setDirection(title.direction==WebCore::TextDirection::LTR ? WKC::LTR : WKC::RTL);
    m_appClient->dispatchDidReceiveTitle(str);

    if (m_frame->m_title) {
        fastFree(m_frame->m_title);
        m_frame->m_title = 0;
    }
    m_frame->m_title = wkc_wstrndup(str.characters(), str.length());
    // just ignore the error
}

void
FrameLoaderClientWKC::dispatchDidCommitLoad(std::optional<WebCore::HasInsecureContent>, std::optional<WebCore::UsedLegacyTLS>)
{
    m_appClient->dispatchDidCommitLoad();

    if (m_frame->m_uri) {
        fastFree(m_frame->m_uri);
        m_frame->m_uri = 0;
    }
    if (!m_frame->core() || !m_frame->core()->loader().activeDocumentLoader())
        return;

    m_frame->m_uri = wkc_strdup(m_frame->core()->loader().activeDocumentLoader()->url().string().utf8().data());

    if (m_frame->m_title) {
        fastFree(m_frame->m_title);
        m_frame->m_title = 0;
    }

    notifyStatus(WKC::ELoadStatusCommitted);
}

void
FrameLoaderClientWKC::dispatchDidFailProvisionalLoad(const WebCore::ResourceError& error, WebCore::WillContinueLoading)
{
    ResourceErrorPrivate wobj(error);
    m_appClient->dispatchDidFailProvisionalLoad(wobj.wkc());
    notifyStatus(WKC::ELoadStatusNone);
}

void
FrameLoaderClientWKC::dispatchDidFailLoad(const WebCore::ResourceError& error)
{
    ResourceErrorPrivate wobj(error);
    m_appClient->dispatchDidFailLoad(wobj.wkc());
    notifyStatus(WKC::ELoadStatusFailed);
}

void
FrameLoaderClientWKC::dispatchDidFinishDocumentLoad()
{
    m_appClient->dispatchDidFinishDocumentLoad();
}

void
FrameLoaderClientWKC::dispatchDidFinishLoad()
{
    m_appClient->dispatchDidFinishLoad();
    notifyStatus(WKC::ELoadStatusFinished);
}

void
FrameLoaderClientWKC::dispatchDidLayout()
{
    m_appClient->dispatchDidLayout();
}

void
FrameLoaderClientWKC::dispatchDidReachLayoutMilestone(OptionSet<WebCore::LayoutMilestone> milestones)
{
    m_appClient->dispatchDidReachLayoutMilestone((WKC::LayoutMilestones)milestones.toRaw());
}

// WKC extension
bool
FrameLoaderClientWKC::dispatchWillReceiveData(WebCore::ResourceHandle* handle, int length)
{
    ResourceHandlePrivate ldr(handle);
    return m_appClient->dispatchWillReceiveData(&ldr.wkc(), length);
}

bool
FrameLoaderClientWKC::notifySSLHandshakeStatus(WebCore::ResourceHandle* handle, int status)
{
    ResourceHandlePrivate hdl(handle);
    return m_appClient->notifySSLHandshakeStatus(&hdl.wkc(), (WKC::SSLHandshakeStatus)status);
}

int
FrameLoaderClientWKC::requestSSLClientCertSelect(WebCore::ResourceHandle* handle, const char* requester, void* certs, int num)
{
    ResourceHandlePrivate hdl(handle);

    WebCore::ClientCertificate** clientCerts;
    clientCerts = (WebCore::ClientCertificate**)certs;
    ClientCertificatePrivate crts(clientCerts);

    return m_appClient->requestSSLClientCertSelect(&hdl.wkc(), requester, &crts.wkc(), num);
}

int
FrameLoaderClientWKC::dispatchWillPermitSendRequest(WebCore::ResourceHandle* handle, const char* url, int composition, bool isSync, const WebCore::ResourceResponse& redirect_response)
{
    ResourceResponsePrivate res(redirect_response);
    return m_appClient->dispatchWillPermitSendRequest((void*)handle, url, (WKC::ContentComposition)composition, isSync, res.wkc());
}

void
FrameLoaderClientWKC::didRestoreFromHTTPCache(WebCore::ResourceHandle* handle, const WTF::URL& url)
{
    ResourceHandlePrivate hdl(handle);
    m_appClient->didRestoreFromHTTPCache(&hdl.wkc(), url);
}

#ifdef WKC_ENABLE_CUSTOMJS
bool
FrameLoaderClientWKC::dispatchWillCallCustomJS(WKCCustomJSAPIList* api, void** context)
{
    return m_appClient->dispatchWillCallCustomJS(api, context);
}
#endif // WKC_ENABLE_CUSTOMJS

WebCore::Frame*
FrameLoaderClientWKC::dispatchCreatePage(const WebCore::NavigationAction&, WebCore::NewFrameOpenerPolicy)
{
    WKC::Frame* ret = m_appClient->dispatchCreatePage();
    if (!ret)
        return 0;
    return (WebCore::Frame *)ret->priv().webcore();
}

void
FrameLoaderClientWKC::dispatchShow()
{
    m_appClient->dispatchShow();
}

void
FrameLoaderClientWKC::dispatchDecidePolicyForNewWindowAction(const WebCore::NavigationAction& action, const WebCore::ResourceRequest& request, WebCore::FormState* state, const WTF::String& frame_name, WebCore::PolicyCheckIdentifier identifier, WebCore::FramePolicyFunction&& function)
{
    WKC::FramePolicyFunctionPrivate* f = new WKC::FramePolicyFunctionPrivate(m_frame->core(), (void *)&function);
    ResourceRequestPrivate req(request);
    NavigationActionPrivate nav(action);
    FormStatePrivate st(state);
    WKC::PolicyCheckIdentifier wkcIdentifier(identifier);

    m_appClient->dispatchDecidePolicyForNewWindowAction(*f, nav.wkc(), req.wkc(), &st.wkc(), frame_name, wkcIdentifier);

    delete f;
}

void
FrameLoaderClientWKC::dispatchDecidePolicyForNavigationAction(const WebCore::NavigationAction& action, const WebCore::ResourceRequest& request, const WebCore::ResourceResponse& redirectResponse, WebCore::FormState* state, WebCore::PolicyDecisionMode, WebCore::PolicyCheckIdentifier identifier, WebCore::FramePolicyFunction&& function)
{
    WKC::FramePolicyFunctionPrivate* f = new WKC::FramePolicyFunctionPrivate(m_frame->core(), (void *)&function);
    ResourceRequestPrivate req(request);
    ResourceResponsePrivate res(redirectResponse);
    NavigationActionPrivate nav(action);
    FormStatePrivate st(state);
    WKC::PolicyCheckIdentifier wkcIdentifier(identifier);

    m_appClient->dispatchDecidePolicyForNavigationAction(*f, nav.wkc(), req.wkc(), res.wkc(), &st.wkc(), wkcIdentifier);

    delete f;
}

void
FrameLoaderClientWKC::cancelPolicyCheck()
{
    m_appClient->cancelPolicyCheck();
}


void
FrameLoaderClientWKC::dispatchUnableToImplementPolicy(const WebCore::ResourceError& error)
{
    ResourceErrorPrivate wobj(error);
    m_appClient->dispatchUnableToImplementPolicy(wobj.wkc());
}

void
FrameLoaderClientWKC::dispatchWillSubmitForm(WebCore::FormState& state, CompletionHandler<void()>&& function)
{
    FormStatePrivate st(&state);

    m_appClient->dispatchWillSubmitForm(&st.wkc());
    function();
}

void
FrameLoaderClientWKC::revertToProvisionalState(WebCore::DocumentLoader* loader)
{
    DocumentLoaderPrivate ldr(loader);
    m_appClient->revertToProvisionalState(&ldr.wkc());
}

void
FrameLoaderClientWKC::setMainDocumentError(WebCore::DocumentLoader* loader, const WebCore::ResourceError& error)
{
#if ENABLE(NETSCAPE_PLUGIN_API)
    if (m_pluginView) {
        m_pluginView->didFail(error);
        m_pluginView = 0;
        m_hasSentResponseToPlugin = false;
    } else
#endif // ENABLE(NETSCAPE_PLUGIN_API)
    {
        DocumentLoaderPrivate ldr(loader);
        ResourceErrorPrivate wobj(error);
        m_appClient->setMainDocumentError(&ldr.wkc(), wobj.wkc());
    }
}

RefPtr<WebCore::Frame>
FrameLoaderClientWKC::createFrame(const WTF::String& name, WebCore::HTMLFrameOwnerElement& ownerElement)
{
    WebCore::Frame* frame = m_frame->core();
    if (!frame || !frame->loader().activeDocumentLoader())
        return nullptr;

    WKC::WKCWebFrame* child = 0;
    HTMLFrameOwnerElementPrivate o(&ownerElement);
    child = WKC::WKCWebFrame::create(m_frame->m_view, m_frame->clientBuilders(), &o.wkc(), false);
    // The parent frame page may be removed by JavaScript.
    if (!frame->page()) {
        return nullptr;
    }

    if (!child) return nullptr;

    RefPtr<WebCore::Frame> childframe = adoptRef(child->privateFrame()->core());

    childframe->tree().setName(name);

    if (!childframe->page()) {
        return nullptr;
    }
    frame->tree().appendChild(*childframe);
    childframe->init();

    if (!frame->loader().activeDocumentLoader()) {
        return nullptr;
    }

    return childframe;
}

#if ENABLE(NETSCAPE_PLUGIN_API)
RefPtr<WebCore::Widget>
FrameLoaderClientWKC::createPlugin(const WebCore::IntSize& size, WebCore::HTMLPlugInElement* element, const WTF::URL& uri, const WTF::Vector<WTF::String>& paramNames, const WTF::Vector<WTF::String>& paramValues, const WTF::String& mimeType, bool loadManually)
{
    RefPtr<WebCore::PluginView> pluginView = WebCore::PluginView::create(m_frame->core(), size, element, uri, paramNames, paramValues, mimeType, loadManually);

    if (pluginView->status() == WebCore::PluginStatusLoadedSuccessfully)
        return pluginView;
    return 0;
}

void
FrameLoaderClientWKC::recreatePlugin(WebCore::Widget*)
{
    notImplemented();
}

void
FrameLoaderClientWKC::redirectDataToPlugin(WebCore::Widget* pluginWidget)
{
    m_pluginView = static_cast<WebCore::PluginView*>(pluginWidget);
    if (pluginWidget)
        m_hasSentResponseToPlugin = false;
}
#endif // ENABLE(NETSCAPE_PLUGIN_API)

#if ENABLE(NETSCAPE_PLUGIN_API)
void
FrameLoaderClientWKC::dispatchDidFailToStartPlugin(const WebCore::PluginView*) const
{
    notImplemented();
}
#endif

WTF::String
FrameLoaderClientWKC::overrideMediaType() const
{
    return WTF::String();
}

#ifdef WKC_ENABLE_CUSTOMJS
static void
_setCustomJS(CustomJSAPIListHashMap *hashMap, JSGlobalContextRef ctx, JSObjectRef windowObject, bool isStringApi)
{
    CustomJSAPIListHashMap::iterator end;
    JSStringRef jsFuncName;
    JSObjectRef jsFunc;

    if (!hashMap || !hashMap->size())
        return;

    end = hashMap->end();
    for (CustomJSAPIListHashMap::iterator iter = hashMap->begin(); iter != end; ++iter) {
        jsFuncName = JSStringCreateWithUTF8CString(iter->key.utf8().data());
        if (isStringApi) {
            jsFunc = JSObjectMakeFunctionWithCallback(ctx, jsFuncName, (JSObjectCallAsFunctionCallback)CustomJSStringCallBackWKC);
        } else {
            jsFunc = JSObjectMakeFunctionWithCallback(ctx, jsFuncName, (JSObjectCallAsFunctionCallback)CustomJSCallBackWKC);
        }
        JSObjectSetProperty(ctx, windowObject, jsFuncName, jsFunc, kJSPropertyAttributeNone, NULL);
        JSStringRelease(jsFuncName);
    }
}
#endif // WKC_ENABLE_CUSTOMJS

void
FrameLoaderClientWKC::dispatchDidClearWindowObjectInWorld(WebCore::DOMWrapperWorld& world)
{
//    if (world. != WebCore::mainThreadNormalWorld()) {
//        return;
//    }
    DOMWrapperWorldPrivate w(&world);

#ifdef WKC_ENABLE_CUSTOMJS
    WKCSettings* settings = m_frame->m_view->settings();
    bool isCustomJSEnable = (settings && settings->isScriptEnabled());
    if (isCustomJSEnable) {
        m_frame->initCustomJSAPIList();
    }
#endif // WKC_ENABLE_CUSTOMJS

    m_appClient->dispatchDidClearWindowObjectInWorld(&w.wkc());

#ifdef WKC_ENABLE_CUSTOMJS
    if (isCustomJSEnable) {
        WebCore::Frame* coreFrame = m_frame->core();
        if (!coreFrame)
            return;

        JSGlobalContextRef ctx = toGlobalRef(coreFrame->script().globalObject(world)->globalExec());
        JSObjectRef windowObject = toRef(coreFrame->script().globalObject(world));
        if (!windowObject)
            return;

        //integer external
        _setCustomJS(m_frame->getCustomJSAPIList(), ctx, windowObject, false);

        //integer internal
        _setCustomJS(m_frame->getCustomJSAPIListInternal(), ctx, windowObject, false);

        //string external
        _setCustomJS(m_frame->getCustomJSStringAPIList(), ctx, windowObject, true);

        //string internal
        _setCustomJS(m_frame->getCustomJSStringAPIListInternal(), ctx, windowObject, true);
    }
#endif // WKC_ENABLE_CUSTOMJS

}

void
FrameLoaderClientWKC::registerForIconNotification()
{
    m_appClient->registerForIconNotification();
}


void
FrameLoaderClientWKC::setMainFrameDocumentReady(bool flag)
{
    m_appClient->setMainFrameDocumentReady(flag);
}


void
FrameLoaderClientWKC::startDownload(const WebCore::ResourceRequest& request, const WTF::String& suggestedName)
{
    ResourceRequestPrivate req(request);
    m_appClient->startDownload(req.wkc(), suggestedName);
}

void
FrameLoaderClientWKC::convertMainResourceLoadToDownload(WebCore::DocumentLoader* loader, const WebCore::ResourceRequest& request, const WebCore::ResourceResponse& response)
{
    DocumentLoaderPrivate ldr(loader);
    ResourceRequestPrivate req(request);
    ResourceResponsePrivate res(response);
    m_appClient->convertMainResourceLoadToDownload(&ldr.wkc(), req.wkc(), res.wkc());
}

void
FrameLoaderClientWKC::willChangeTitle(WebCore::DocumentLoader* loader)
{
    DocumentLoaderPrivate ldr(loader);
    m_appClient->willChangeTitle(&ldr.wkc());
}

void
FrameLoaderClientWKC::didChangeTitle(WebCore::DocumentLoader* loader)
{
    DocumentLoaderPrivate ldr(loader);
    m_appClient->didChangeTitle(&ldr.wkc());
}

void
FrameLoaderClientWKC::willReplaceMultipartContent()
{
    notImplemented();
}

void
FrameLoaderClientWKC::didReplaceMultipartContent()
{
    notImplemented();
}

void
FrameLoaderClientWKC::committedLoad(WebCore::DocumentLoader* loader, const WebCore::SharedBuffer& buffer)
{
#if ENABLE(NETSCAPE_PLUGIN_API)
    if (!m_pluginView)
#endif
    {
        DocumentLoaderPrivate ldr(loader);
        m_appClient->committedLoad(&ldr.wkc(), buffer.data(), buffer.size());

        WebCore::FrameLoader* frameLoader = &m_frame->core()->loader();
        if (frameLoader && frameLoader->documentLoader()) {
            WebCore::DocumentWriter* writer = &frameLoader->documentLoader()->writer();
            const WTF::String& encoding = loader->overrideEncoding();
            if (encoding.isNull()) {
                writer->setEncoding(loader->response().textEncodingName(), WebCore::DocumentWriter::IsEncodingUserChosen::No);
            } else {
                writer->setEncoding(encoding, WebCore::DocumentWriter::IsEncodingUserChosen::Yes);
            }
        }

        WebCore::Frame* coreFrame = loader->frame();

        loader->commitData(buffer);

        if (coreFrame && coreFrame->document() && coreFrame->document()->isMediaDocument()) {
            loader->cancelMainResourceLoad(frameLoader->client().pluginWillHandleLoadError(loader->response()));
        }
    }

#if ENABLE(NETSCAPE_PLUGIN_API)
    if (m_pluginView) {
        if (!m_hasSentResponseToPlugin) {
            m_pluginView->didReceiveResponse(loader->response());
            m_hasSentResponseToPlugin = true;
        }

        // state may change...
        if (!m_pluginView)
            return;

        m_pluginView->didReceiveData(data, len);
    }
#endif
}

void
FrameLoaderClientWKC::finishedLoading(WebCore::DocumentLoader* loader)
{
#if ENABLE(NETSCAPE_PLUGIN_API)
    if (m_pluginView) {
        m_pluginView->didFinishLoading();
        m_pluginView = 0;
        m_hasSentResponseToPlugin = false;
    } else
#endif
    {
        DocumentLoaderPrivate ldr(loader);
        m_appClient->finishedLoading(&ldr.wkc());
        committedLoad(loader, WebCore::FragmentedSharedBuffer::create()->makeContiguous());
    }
}


void
FrameLoaderClientWKC::updateGlobalHistory()
{
    m_appClient->updateGlobalHistory();
}

void
FrameLoaderClientWKC::updateGlobalHistoryRedirectLinks()
{
    m_appClient->updateGlobalHistoryRedirectLinks();
}

bool
FrameLoaderClientWKC::shouldGoToHistoryItem(WebCore::HistoryItem& item) const
{
    bool ret = false;
    HistoryItemPrivate his(&item);
    ret = m_appClient->shouldGoToHistoryItem(&his.wkc());
    return ret;
}

void
FrameLoaderClientWKC::didDisplayInsecureContent()
{
    m_appClient->didDisplayInsecureContent();
}

void
FrameLoaderClientWKC::didRunInsecureContent(WebCore::SecurityOrigin& origin, const WTF::URL& uri)
{
    SecurityOriginPrivate o(&origin);
    m_appClient->didRunInsecureContent(o.wkc(), uri);
}

void
FrameLoaderClientWKC::didDetectXSS(const WTF::URL& url, bool didBlockEntirePage)
{
    m_appClient->didDetectXSS(url, didBlockEntirePage);
}


WebCore::ResourceError
FrameLoaderClientWKC::cancelledError(const WebCore::ResourceRequest& request) const
{
    ResourceRequestPrivate req(request);
    return m_appClient->cancelledError(req.wkc()).priv().webcore();
}

WebCore::ResourceError
FrameLoaderClientWKC::blockedError(const WebCore::ResourceRequest& request) const
{
    ResourceRequestPrivate req(request);
    return m_appClient->blockedError(req.wkc()).priv().webcore();
}

WebCore::ResourceError
FrameLoaderClientWKC::blockedByContentBlockerError(const WebCore::ResourceRequest& request) const
{
    ResourceRequestPrivate req(request);
    return m_appClient->blockedByContentBlockerError(req.wkc()).priv().webcore();
}

WebCore::ResourceError
FrameLoaderClientWKC::cannotShowURLError(const WebCore::ResourceRequest& request) const
{
    ResourceRequestPrivate req(request);
    return m_appClient->cannotShowURLError(req.wkc()).priv().webcore();
}

WebCore::ResourceError
FrameLoaderClientWKC::interruptedForPolicyChangeError(const WebCore::ResourceRequest& request) const
{
    ResourceRequestPrivate req(request);
    return m_appClient->interruptForPolicyChangeError(req.wkc()).priv().webcore();
}


WebCore::ResourceError
FrameLoaderClientWKC::cannotShowMIMETypeError(const WebCore::ResourceResponse& response) const
{
    ResourceResponsePrivate res(response);
    return m_appClient->cannotShowMIMETypeError(res.wkc()).priv().webcore();
}

WebCore::ResourceError
FrameLoaderClientWKC::fileDoesNotExistError(const WebCore::ResourceResponse& response) const
{
    ResourceResponsePrivate res(response);
    return m_appClient->fileDoesNotExistError(res.wkc()).priv().webcore();
}

WebCore::ResourceError
FrameLoaderClientWKC::pluginWillHandleLoadError(const WebCore::ResourceResponse& response) const
{
    ResourceResponsePrivate res(response);
    return m_appClient->pluginWillHandleLoadError(res.wkc()).priv().webcore();
}


bool
FrameLoaderClientWKC::shouldFallBack(const WebCore::ResourceError& error) const
{
    ResourceErrorPrivate wobj(error);
    return m_appClient->shouldFallBack(wobj.wkc());
}


bool
FrameLoaderClientWKC::canHandleRequest(const WebCore::ResourceRequest& request) const
{
    ResourceRequestPrivate req(request);
    return m_appClient->canHandleRequest(req.wkc());
}

bool
FrameLoaderClientWKC::canShowMIMEType(const WTF::String& type) const
{
    return m_appClient->canShowMIMEType(type);
}

bool
FrameLoaderClientWKC::representationExistsForURLScheme(const WTF::String& string) const
{
    return m_appClient->representationExistsForURLScheme(string);
}

WTF::String
FrameLoaderClientWKC::generatedMIMETypeForURLScheme(const WTF::String& string) const
{
    return m_appClient->generatedMIMETypeForURLScheme(string);
}


void
FrameLoaderClientWKC::frameLoadCompleted()
{
    m_appClient->frameLoadCompleted();
}

void
FrameLoaderClientWKC::saveViewStateToItem(WebCore::HistoryItem& item)
{
    HistoryItemPrivate his(&item);
    m_appClient->saveViewStateToItem(his.wkc());
}

void
FrameLoaderClientWKC::restoreViewState()
{
    m_appClient->restoreViewState();
}

void
FrameLoaderClientWKC::provisionalLoadStarted()
{
    m_appClient->provisionalLoadStarted();
}

void
FrameLoaderClientWKC::didFinishLoad()
{
    m_appClient->didFinishLoad();
}

void
FrameLoaderClientWKC::prepareForDataSourceReplacement()
{
    m_appClient->prepareForDataSourceReplacement();
}


WTF::Ref<WebCore::DocumentLoader>
FrameLoaderClientWKC::createDocumentLoader(const WebCore::ResourceRequest& request, const WebCore::SubstituteData& substituteData)
{
    return WebCore::DocumentLoader::create(request, substituteData);
}

void
FrameLoaderClientWKC::updateCachedDocumentLoader(WebCore::DocumentLoader&)
{
    notImplemented();
}

void
FrameLoaderClientWKC::setTitle(const WebCore::StringWithDirection& title, const WTF::URL& uri)
{
    // This function is for History

    WKC::String str(title.string);
    str.setDirection(title.direction==WebCore::TextDirection::LTR ? WKC::LTR : WKC::RTL);
    m_appClient->setTitle(str, uri);
}


WTF::String
FrameLoaderClientWKC::userAgent(const WTF::URL& uri) const
{
    WTF::String ret;
    ret = m_appClient->userAgent(uri);
    return ret;
}


void
FrameLoaderClientWKC::savePlatformDataToCachedFrame(WebCore::CachedFrame* frame)
{
    CachedFramePrivate c(frame);
    m_appClient->savePlatformDataToCachedFrame(&c.wkc());
}

void
FrameLoaderClientWKC::transitionToCommittedFromCachedFrame(WebCore::CachedFrame* frame)
{
    CachedFramePrivate c(frame);
    m_appClient->transitionToCommittedFromCachedFrame(&c.wkc());
}

void
FrameLoaderClientWKC::transitionToCommittedForNewPage()
{
    WKCWebViewPrivate* containingWindow = m_frame->m_view;
    WebCore::IntSize desktopsize = containingWindow->defaultDesktopSize();
    WebCore::IntSize viewsize = containingWindow->defaultViewSize();
    bool transparent = containingWindow->transparent();
    std::optional<WebCore::Color> backgroundColor;
    if (transparent)
        backgroundColor = WebCore::Color::transparentBlack;
    WebCore::Frame* frame = m_frame->core();
    bool usefixed = false;
    if (frame->view()) {
        usefixed = frame->view()->useFixedLayout();
    }

    auto oldView = frame->view();

    auto overrideSizeForCSSDefaultViewportUnits = oldView ? oldView->overrideSizeForCSSDefaultViewportUnits() : std::nullopt;
    auto overrideSizeForCSSSmallViewportUnits = oldView ? oldView->overrideSizeForCSSSmallViewportUnits() : std::nullopt;
    auto overrideSizeForCSSLargeViewportUnits = oldView ? oldView->overrideSizeForCSSLargeViewportUnits() : std::nullopt;
    auto layoutViewportOverrideRect = oldView ? oldView->layoutViewportOverrideRect() : std::nullopt;

    WebCore::IntRect fixedrect = WebCore::IntRect();
    if (usefixed) {
        fixedrect.setSize(viewsize);
    }
    WebCore::ScrollbarMode scrollbarmode = frame->ownerElement() ? WebCore::ScrollbarMode::Auto : WebCore::ScrollbarMode::AlwaysOff;
    frame->createView(desktopsize, backgroundColor, viewsize, fixedrect, usefixed, scrollbarmode, false, scrollbarmode, false);

    if (frame->view()) {
        if (!frame->ownerElement()) {
            frame->view()->setCanHaveScrollbars(false);
            frame->view()->setClipsRepaints(WKCWebView::clipsRepaints());
        }
    }

    if (overrideSizeForCSSDefaultViewportUnits)
        frame->view()->setOverrideSizeForCSSDefaultViewportUnits(*overrideSizeForCSSDefaultViewportUnits);

    if (overrideSizeForCSSSmallViewportUnits)
        frame->view()->setOverrideSizeForCSSSmallViewportUnits(*overrideSizeForCSSSmallViewportUnits);

    if (overrideSizeForCSSLargeViewportUnits)
        frame->view()->setOverrideSizeForCSSLargeViewportUnits(*overrideSizeForCSSLargeViewportUnits);

    if (layoutViewportOverrideRect)
        frame->view()->setLayoutViewportOverrideRect(*layoutViewportOverrideRect);

    m_appClient->transitionToCommittedForNewPage();
}


bool
FrameLoaderClientWKC::canCachePage() const
{
    return m_appClient->canCachePage();
}

void
FrameLoaderClientWKC::dispatchDecidePolicyForResponse(const WebCore::ResourceResponse& response, const WebCore::ResourceRequest& request, WebCore::PolicyCheckIdentifier id, const WTF::String& downloadAttribute, WebCore::FramePolicyFunction&& function)
{
    WKC::FramePolicyFunctionPrivate* f = new WKC::FramePolicyFunctionPrivate(m_frame->core(), (void *)&function);
    ResourceResponsePrivate res(response);
    ResourceRequestPrivate req(request);
    WKC::PolicyCheckIdentifier wkcIdentifier(id);

    m_appClient->dispatchDecidePolicyForResponse(*f, res.wkc(), req.wkc(), wkcIdentifier);

    delete f;
}

void
FrameLoaderClientWKC::dispatchWillSendSubmitEvent(WTF::Ref<WebCore::FormState>&& state)
{
    FormStatePrivate st(&state.get());
    m_appClient->dispatchWillSendSubmitEvent(&st.wkc());
}

bool
FrameLoaderClientWKC::canShowMIMETypeAsHTML(const WTF::String& MIMEType) const
{
    return m_appClient->canShowMIMETypeAsHTML(MIMEType);
}

void
FrameLoaderClientWKC::didRestoreFromBackForwardCache()
{
    if (m_frame && m_frame->core() && m_frame->core()->page()) {
        // Update style for cached pages to show texts in current defaultFontSize in Settings,
        // or the texts would shown in older font size setting.
        m_frame->core()->page()->setNeedsRecalcStyleInAllFrames();
        // Apply current user style sheet for the same reason above.
        m_frame->core()->page()->userStyleSheetLocationChanged();
    }
    m_appClient->didRestoreFromBackForwardCache();
}

WebCore::ObjectContentType
FrameLoaderClientWKC::objectContentType(const WTF::URL& url, const WTF::String& mime)
{
    WKC::ObjectContentType ret = m_appClient->objectContentType(url, mime);
    return toWebCoreObjectContentType(ret);
}

WTF::Ref<WebCore::FrameNetworkingContext>
FrameLoaderClientWKC::createNetworkingContext()
{
    return WKC::FrameNetworkingContextWKC::create(m_frame ? m_frame->core() : 0);
}

FrameNetworkingContextWKC::FrameNetworkingContextWKC(WebCore::Frame* frame)
    : FrameNetworkingContext(frame)
{
}

WTF::Ref<FrameNetworkingContextWKC>
FrameNetworkingContextWKC::create(WebCore::Frame* frame)
{
    return adoptRef(*new FrameNetworkingContextWKC(frame));
}

WebCore::ResourceLoader*
FrameNetworkingContextWKC::mainResourceLoader() const
{
    if (frame() && frame()->loader().activeDocumentLoader()) {
        return (WebCore::ResourceLoader*)(frame()->loader().activeDocumentLoader()->mainResourceLoader());
    }
    return 0;
}

WebCore::FrameLoaderClient*
FrameNetworkingContextWKC::frameLoaderClient() const
{
    if (frame()) {
        return &frame()->loader().client();
    }
    return 0;
}

WebCore::NetworkStorageSession*
FrameNetworkingContextWKC::storageSession() const
{
    if (frame()) {
        if (auto* storageSession = WebCore::NetworkStorageSession::storageSession(frame()->page()->sessionID()))
            return storageSession;
    }
    return &WebCore::NetworkStorageSession::defaultStorageSession();
}

void
FrameLoaderClientWKC::didChangeScrollOffset()
{
    m_appClient->didChangeScrollOffset();
}

bool
FrameLoaderClientWKC::allowScript(bool enabledPerSettings)
{
    return m_appClient->allowScript(enabledPerSettings);
}

#if ENABLE(WEBGL)
bool
FrameLoaderClientWKC::allowWebGL(bool enabledPerSettings)
{
    return m_appClient->allowWebGL(enabledPerSettings);
}

void
FrameLoaderClientWKC::didLoseWebGLContext(int ctx)
{
    m_appClient->didLoseWebGLContext(ctx);
}
#endif

bool
FrameLoaderClientWKC::shouldForceUniversalAccessFromLocalURL(const WTF::URL& url)
{
    return m_appClient->shouldForceUniversalAccessFromLocalURL(url);
}

#if 0
void
FrameLoaderClientWKC::dispatchWillOpenSocketStream(WebCore::SocketStreamHandle*)
{
    // Ugh!: notify to client!
    // 120808 ACCESS Co.,Ltd.
    notImplemented();
}
#endif

void
FrameLoaderClientWKC::dispatchGlobalObjectAvailable(WebCore::DOMWrapperWorld& world)
{
    DOMWrapperWorldPrivate w(&world);
    m_appClient->dispatchGlobalObjectAvailable(&w.wkc());
}

void
FrameLoaderClientWKC::dispatchWillDisconnectDOMWindowExtensionFromGlobalObject(WebCore::DOMWindowExtension* world)
{
    // Ugh!: notify to client!
    // 120808 ACCESS Co.,Ltd.
    notImplemented();
}

void
FrameLoaderClientWKC::dispatchDidReconnectDOMWindowExtensionToGlobalObject(WebCore::DOMWindowExtension* world)
{
    // Ugh!: notify to client!
    // 120808 ACCESS Co.,Ltd.
    notImplemented();
}

void
FrameLoaderClientWKC::dispatchWillDestroyGlobalObjectForDOMWindowExtension(WebCore::DOMWindowExtension* world)
{
    // Ugh!: notify to client!
    // 120808 ACCESS Co.,Ltd.
    notImplemented();
}

void
FrameLoaderClientWKC::completePageTransitionIfNeeded()
{
    notImplemented();
}

void
FrameLoaderClientWKC::prefetchDNS(const WTF::String&)
{
    // Ugh!: notify to client!
    // 180123 ACCESS Co.,Ltd.
    notImplemented();
}

// framePolicyFunction

FramePolicyFunction::FramePolicyFunction(void* parent, void* func)
    : m_parent(parent)
    , m_func(func)
{
}

FramePolicyFunction::~FramePolicyFunction()
{
}

void
FramePolicyFunction::reply(WKC::PolicyAction action,const WKC::PolicyCheckIdentifier& identifier)
{
    WebCore::FramePolicyFunction* func = (WebCore::FramePolicyFunction *)m_func;

    (*func)(toWebCorePolicyAction(action),identifier.webcore());
}

} // namespace
