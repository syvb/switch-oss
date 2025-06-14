/*
 * Copyright (C) 2006-2007, 2010-2011, 2016 Apple Inc. All rights reserved.
 *           (C) 2007 Graham Dennis (graham.dennis@gmail.com)
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

#include "config.h"
#include "ResourceLoader.h"

#include "ApplicationCacheHost.h"
#include "AuthenticationChallenge.h"
#include "DiagnosticLoggingClient.h"
#include "DiagnosticLoggingKeys.h"
#include "DocumentLoader.h"
#include "Frame.h"
#include "FrameLoader.h"
#include "FrameLoaderClient.h"
#include "InspectorInstrumentation.h"
#include "LoaderStrategy.h"
#include "MainFrame.h"
#include "MixedContentChecker.h"
#include "Page.h"
#include "PlatformStrategies.h"
#include "ProgressTracker.h"
#include "ResourceError.h"
#include "ResourceHandle.h"
#include "ResourceLoadScheduler.h"
#include "SecurityOrigin.h"
#include "Settings.h"
#include "SharedBuffer.h"
#include <wtf/Ref.h>

#if ENABLE(CONTENT_EXTENSIONS)
#include "ResourceLoadInfo.h"
#include "UserContentController.h"
#endif

namespace WebCore {

ResourceLoader::ResourceLoader(Frame* frame, ResourceLoaderOptions options)
    : m_frame(frame)
    , m_documentLoader(frame->loader().activeDocumentLoader())
    , m_identifier(0)
    , m_reachedTerminalState(false)
    , m_notifiedLoadComplete(false)
    , m_cancellationStatus(NotCancelled)
    , m_defersLoading(frame->page()->defersLoading())
    , m_canAskClientForCredentials { options.clientCredentialPolicy == ClientCredentialPolicy::MayAskClientForCredentials }
    , m_options(options)
    , m_isQuickLookResource(false)
#if ENABLE(CONTENT_EXTENSIONS)
    , m_resourceType(ResourceType::Invalid)
#endif
{
}

ResourceLoader::~ResourceLoader()
{
    ASSERT(m_reachedTerminalState);
}

void ResourceLoader::releaseResources()
{
    ASSERT(!m_reachedTerminalState);
    
    // It's possible that when we release the handle, it will be
    // deallocated and release the last reference to this object.
    // We need to retain to avoid accessing the object after it
    // has been deallocated and also to avoid reentering this method.
    Ref<ResourceLoader> protect(*this);

    m_frame = nullptr;
    m_documentLoader = nullptr;
    
    // We need to set reachedTerminalState to true before we release
    // the resources to prevent a double dealloc of WebView <rdar://problem/4372628>
    m_reachedTerminalState = true;

    platformStrategies()->loaderStrategy()->resourceLoadScheduler()->remove(this);
    m_identifier = 0;

    if (m_handle) {
        // Clear out the ResourceHandle's client so that it doesn't try to call
        // us back after we release it, unless it has been replaced by someone else.
        if (m_handle->client() == this)
            m_handle->setClient(nullptr);
        m_handle = nullptr;
    }

    m_resourceData = nullptr;
    m_deferredRequest = ResourceRequest();
}

bool ResourceLoader::init(const ResourceRequest& r)
{
    ASSERT(!m_handle);
    ASSERT(m_request.isNull());
    ASSERT(m_deferredRequest.isNull());
    ASSERT(!m_documentLoader->isSubstituteLoadPending(this));
    
    ResourceRequest clientRequest(r);

#if PLATFORM(IOS)
    // If the documentLoader was detached while this ResourceLoader was waiting its turn
    // in ResourceLoadScheduler queue, don't continue.
    if (!m_documentLoader->frame()) {
        cancel();
        return false;
    }
#endif
    
    m_defersLoading = m_frame->page()->defersLoading();
    m_canAskClientForCredentials = m_options.clientCredentialPolicy == ClientCredentialPolicy::MayAskClientForCredentials && !isMixedContent(r.url());
    if (m_options.securityCheck() == DoSecurityCheck && !m_frame->document()->securityOrigin().canDisplay(clientRequest.url())) {
        FrameLoader::reportLocalLoadFailed(m_frame.get(), clientRequest.url().string());
        releaseResources();
        return false;
    }
    
    // https://bugs.webkit.org/show_bug.cgi?id=26391
    // The various plug-in implementations call directly to ResourceLoader::load() instead of piping requests
    // through FrameLoader. As a result, they miss the FrameLoader::addExtraFieldsToRequest() step which sets
    // up the 1st party for cookies URL. Until plug-in implementations can be reigned in to pipe through that
    // method, we need to make sure there is always a 1st party for cookies set.
    if (clientRequest.firstPartyForCookies().isNull()) {
        if (Document* document = m_frame->document())
            clientRequest.setFirstPartyForCookies(document->firstPartyForCookies());
    }

    willSendRequestInternal(clientRequest, ResourceResponse());

#if PLATFORM(IOS)
    // If this ResourceLoader was stopped as a result of willSendRequest, bail out.
    if (m_reachedTerminalState)
        return false;
#endif

    if (clientRequest.isNull()) {
        cancel();
        return false;
    }

    m_originalRequest = m_request = clientRequest;
    return true;
}

void ResourceLoader::deliverResponseAndData(const ResourceResponse& response, RefPtr<SharedBuffer>&& buffer)
{
    Ref<ResourceLoader> protect(*this);

    didReceiveResponse(response);
    if (reachedTerminalState())
        return;

    if (buffer) {
        unsigned size = buffer->size();
        didReceiveBuffer(buffer.release(), size, DataPayloadWholeResource);
        if (reachedTerminalState())
            return;
    }

    didFinishLoading(0);
}

void ResourceLoader::start()
{
    ASSERT(!m_handle);
    ASSERT(!m_request.isNull());
    ASSERT(m_deferredRequest.isNull());
    ASSERT(frameLoader());

#if ENABLE(WEB_ARCHIVE) || ENABLE(MHTML)
    if (m_documentLoader->scheduleArchiveLoad(this, m_request))
        return;
#endif

    if (m_documentLoader->applicationCacheHost()->maybeLoadResource(this, m_request, m_request.url()))
        return;

    if (m_defersLoading) {
        m_deferredRequest = m_request;
        return;
    }

    if (!m_reachedTerminalState) {
        FrameLoader& loader = m_request.url().protocolIsData() ? dataProtocolFrameLoader() : *frameLoader();
        m_handle = ResourceHandle::create(loader.networkingContext(), m_request, this, m_defersLoading, m_options.sniffContent() == SniffContent);
    }
}

void ResourceLoader::setDefersLoading(bool defers)
{
    m_defersLoading = defers;
    if (m_handle)
        m_handle->setDefersLoading(defers);
    if (!defers && !m_deferredRequest.isNull()) {
        m_request = m_deferredRequest;
        m_deferredRequest = ResourceRequest();
        start();
    }

    platformStrategies()->loaderStrategy()->resourceLoadScheduler()->setDefersLoading(this, defers);
}

FrameLoader* ResourceLoader::frameLoader() const
{
    if (!m_frame)
        return nullptr;
    return &m_frame->loader();
}

// This function should only be called when frameLoader() is non-null.
FrameLoader& ResourceLoader::dataProtocolFrameLoader() const
{
    FrameLoader* loader = frameLoader();
    ASSERT(loader);
    FrameLoader* dataProtocolLoader = loader->client().dataProtocolLoader();
    return *(dataProtocolLoader ? dataProtocolLoader : loader);
}

void ResourceLoader::setDataBufferingPolicy(DataBufferingPolicy dataBufferingPolicy)
{ 
    m_options.setDataBufferingPolicy(dataBufferingPolicy); 

    // Reset any already buffered data
    if (dataBufferingPolicy == DoNotBufferData)
        m_resourceData = nullptr;
}
    
void ResourceLoader::willSwitchToSubstituteResource()
{
    ASSERT(!m_documentLoader->isSubstituteLoadPending(this));
    platformStrategies()->loaderStrategy()->resourceLoadScheduler()->remove(this);
    if (m_handle)
        m_handle->cancel();
}

void ResourceLoader::addDataOrBuffer(const char* data, unsigned length, SharedBuffer* buffer, DataPayloadType dataPayloadType)
{
    if (m_options.dataBufferingPolicy() == DoNotBufferData)
        return;

    if (dataPayloadType == DataPayloadWholeResource) {
        m_resourceData = buffer ? buffer : SharedBuffer::create(data, length);
        return;
    }
        
    if (!m_resourceData)
        m_resourceData = buffer ? buffer : SharedBuffer::create(data, length);
    else {
        if (buffer)
            m_resourceData->append(buffer);
        else
            m_resourceData->append(data, length);
    }
}

void ResourceLoader::clearResourceData()
{
    if (m_resourceData)
        m_resourceData->clear();
}

bool ResourceLoader::isSubresourceLoader()
{
    return false;
}

bool ResourceLoader::isMixedContent(const URL& url) const
{
    if (MixedContentChecker::isMixedContent(m_frame->document()->securityOrigin(), url))
        return true;
    Frame& topFrame = m_frame->tree().top();
    if (&topFrame != m_frame && MixedContentChecker::isMixedContent(topFrame.document()->securityOrigin(), url))
        return true;
    return false;
}

void ResourceLoader::willSendRequestInternal(ResourceRequest& request, const ResourceResponse& redirectResponse)
{
    // Protect this in this delegate method since the additional processing can do
    // anything including possibly derefing this; one example of this is Radar 3266216.
    Ref<ResourceLoader> protect(*this);

    ASSERT(!m_reachedTerminalState);
#if ENABLE(CONTENT_EXTENSIONS)
    ASSERT(m_resourceType != ResourceType::Invalid);
#endif

    // We need a resource identifier for all requests, even if FrameLoader is never going to see it (such as with CORS preflight requests).
    bool createdResourceIdentifier = false;
    if (!m_identifier) {
        m_identifier = m_frame->page()->progress().createUniqueIdentifier();
        createdResourceIdentifier = true;
    }

#if ENABLE(CONTENT_EXTENSIONS)
    if (frameLoader()) {
        Page* page = frameLoader()->frame().page();
        if (page && m_documentLoader) {
            auto* userContentController = page->userContentController();
            if (userContentController)
                userContentController->processContentExtensionRulesForLoad(*page, request, m_resourceType, *m_documentLoader);
        }
    }
#endif

    if (request.isNull()) {
        didFail(cannotShowURLError());
        return;
    }

    if (m_options.sendLoadCallbacks() == SendCallbacks) {
        if (createdResourceIdentifier)
            frameLoader()->notifier().assignIdentifierToInitialRequest(m_identifier, documentLoader(), request);

#if PLATFORM(IOS)
        // If this ResourceLoader was stopped as a result of assignIdentifierToInitialRequest, bail out
        if (m_reachedTerminalState)
            return;
#endif

        frameLoader()->notifier().willSendRequest(this, request, redirectResponse);
    }
    else
        InspectorInstrumentation::willSendRequest(m_frame.get(), m_identifier, m_frame->loader().documentLoader(), request, redirectResponse);

    bool isRedirect = !redirectResponse.isNull();

    if (isMixedContent(m_request.url()) || (isRedirect && isMixedContent(request.url())))
        m_canAskClientForCredentials = false;

    if (isRedirect)
        platformStrategies()->loaderStrategy()->resourceLoadScheduler()->crossOriginRedirectReceived(this, request.url());

    m_request = request;

    if (isRedirect && !m_documentLoader->isCommitted())
        frameLoader()->client().dispatchDidReceiveServerRedirectForProvisionalLoad();
}

#if PLATFORM(WKC)
void ResourceLoader::willSendRequest(ResourceRequest&& request, const ResourceResponse& redirectResponse, WTF::Function<void(ResourceRequest&&)>&& callback)
#else
void ResourceLoader::willSendRequest(ResourceRequest&& request, const ResourceResponse& redirectResponse, std::function<void(ResourceRequest&&)>&& callback)
#endif
{
    willSendRequestInternal(request, redirectResponse);
    callback(WTF::move(request));
}

void ResourceLoader::didSendData(unsigned long long, unsigned long long)
{
}

static void logResourceResponseSource(Frame* frame, ResourceResponse::Source source)
{
    if (!frame)
        return;

    String sourceKey;
    switch (source) {
    case ResourceResponse::Source::Network:
        sourceKey = DiagnosticLoggingKeys::networkKey();
        break;
    case ResourceResponse::Source::DiskCache:
        sourceKey = DiagnosticLoggingKeys::diskCacheKey();
        break;
    case ResourceResponse::Source::DiskCacheAfterValidation:
        sourceKey = DiagnosticLoggingKeys::diskCacheAfterValidationKey();
        break;
    case ResourceResponse::Source::Unknown:
        return;
    }

    frame->mainFrame().diagnosticLoggingClient().logDiagnosticMessageWithValue(DiagnosticLoggingKeys::resourceResponseKey(), DiagnosticLoggingKeys::sourceKey(), sourceKey, ShouldSample::Yes);
}

void ResourceLoader::didReceiveResponse(const ResourceResponse& r)
{
    ASSERT(!m_reachedTerminalState);

    // Protect this in this delegate method since the additional processing can do
    // anything including possibly derefing this; one example of this is Radar 3266216.
    Ref<ResourceLoader> protect(*this);

    logResourceResponseSource(m_frame.get(), r.source());

    m_response = r;

    if (m_response.isHttpVersion0_9()) {
        auto url = m_response.url();
        // Non-HTTP responses are interpreted as HTTP/0.9 which may allow exfiltration of data
        // from non-HTTP services. Therefore cancel if the document was loaded with different
        // HTTP version or if the resource request was to a non-default port.
        if (!m_documentLoader->response().isHttpVersion0_9()) {
            String message = "Cancelled resource load from '" + url.string() + "' because it is using HTTP/0.9 and the document was loaded with a different HTTP version.";
            m_frame->document()->addConsoleMessage(MessageSource::Security, MessageLevel::Error, message, identifier());
            ResourceError error("", 0, url, message);
            didFail(error);
            return;
        }
        if (!isDefaultPortForProtocol(url.port(), url.protocol())) {
            String message = "Cancelled resource load from '" + url.string() + "' because it is using HTTP/0.9 on a non-default port.";
            m_frame->document()->addConsoleMessage(MessageSource::Security, MessageLevel::Error, message, identifier());
            ResourceError error("", 0, url, message);
            didFail(error);
            return;
        }
            
        String message = "Sandboxing '" + m_response.url().string() + "' because it is using HTTP/0.9.";
        m_frame->document()->addConsoleMessage(MessageSource::Security, MessageLevel::Error, message, m_identifier);
        frameLoader()->forceSandboxFlags(SandboxScripts | SandboxPlugins);
    }

    if (FormData* data = m_request.httpBody())
        data->removeGeneratedFilesIfNeeded();
        
    if (m_options.sendLoadCallbacks() == SendCallbacks)
        frameLoader()->notifier().didReceiveResponse(this, m_response);
}

void ResourceLoader::didReceiveData(const char* data, unsigned length, long long encodedDataLength, DataPayloadType dataPayloadType)
{
    // The following assertions are not quite valid here, since a subclass
    // might override didReceiveData in a way that invalidates them. This
    // happens with the steps listed in 3266216
    // ASSERT(con == connection);
    // ASSERT(!m_reachedTerminalState);

    didReceiveDataOrBuffer(data, length, 0, encodedDataLength, dataPayloadType);
}

void ResourceLoader::didReceiveBuffer(PassRefPtr<SharedBuffer> buffer, long long encodedDataLength, DataPayloadType dataPayloadType)
{
    didReceiveDataOrBuffer(0, 0, buffer, encodedDataLength, dataPayloadType);
}

void ResourceLoader::didReceiveDataOrBuffer(const char* data, unsigned length, PassRefPtr<SharedBuffer> prpBuffer, long long encodedDataLength, DataPayloadType dataPayloadType)
{
    // This method should only get data+length *OR* a SharedBuffer.
    ASSERT(!prpBuffer || (!data && !length));

    // Protect this in this delegate method since the additional processing can do
    // anything including possibly derefing this; one example of this is Radar 3266216.
    Ref<ResourceLoader> protect(*this);
    RefPtr<SharedBuffer> buffer = prpBuffer;

    addDataOrBuffer(data, length, buffer.get(), dataPayloadType);
    
    // FIXME: If we get a resource with more than 2B bytes, this code won't do the right thing.
    // However, with today's computers and networking speeds, this won't happen in practice.
    // Could be an issue with a giant local file.
    if (m_options.sendLoadCallbacks() == SendCallbacks && m_frame)
        frameLoader()->notifier().didReceiveData(this, buffer ? buffer->data() : data, buffer ? buffer->size() : length, static_cast<int>(encodedDataLength));
}

void ResourceLoader::didFinishLoading(double finishTime)
{
    didFinishLoadingOnePart(finishTime);

    // If the load has been cancelled by a delegate in response to didFinishLoad(), do not release
    // the resources a second time, they have been released by cancel.
    if (wasCancelled())
        return;
    releaseResources();
}

void ResourceLoader::didFinishLoadingOnePart(double finishTime)
{
    // If load has been cancelled after finishing (which could happen with a
    // JavaScript that changes the window location), do nothing.
    if (wasCancelled())
        return;
    ASSERT(!m_reachedTerminalState);

    if (m_notifiedLoadComplete)
        return;
    m_notifiedLoadComplete = true;
    if (m_options.sendLoadCallbacks() == SendCallbacks)
        frameLoader()->notifier().didFinishLoad(this, finishTime);
}

void ResourceLoader::didFail(const ResourceError& error)
{
    if (wasCancelled())
        return;
    ASSERT(!m_reachedTerminalState);

    // Protect this in this delegate method since the additional processing can do
    // anything including possibly derefing this; one example of this is Radar 3266216.
    Ref<ResourceLoader> protect(*this);

    cleanupForError(error);
    releaseResources();
}

void ResourceLoader::cleanupForError(const ResourceError& error)
{
    if (FormData* data = m_request.httpBody())
        data->removeGeneratedFilesIfNeeded();

    if (m_notifiedLoadComplete)
        return;
    m_notifiedLoadComplete = true;
    if (m_options.sendLoadCallbacks() == SendCallbacks && m_identifier)
        frameLoader()->notifier().didFailToLoad(this, error);
}

void ResourceLoader::cancel()
{
    cancel(ResourceError());
}

void ResourceLoader::cancel(const ResourceError& error)
{
    // If the load has already completed - succeeded, failed, or previously cancelled - do nothing.
    if (m_reachedTerminalState)
        return;
       
    ResourceError nonNullError = error.isNull() ? cancelledError() : error;
    
    // willCancel() and didFailToLoad() both call out to clients that might do 
    // something causing the last reference to this object to go away.
    Ref<ResourceLoader> protect(*this);
    
    // If we re-enter cancel() from inside willCancel(), we want to pick up from where we left 
    // off without re-running willCancel()
    if (m_cancellationStatus == NotCancelled) {
        m_cancellationStatus = CalledWillCancel;
        
        willCancel(nonNullError);
    }

    // If we re-enter cancel() from inside didFailToLoad(), we want to pick up from where we 
    // left off without redoing any of this work.
    if (m_cancellationStatus == CalledWillCancel) {
        m_cancellationStatus = Cancelled;

        if (m_handle)
            m_handle->clearAuthentication();

        m_documentLoader->cancelPendingSubstituteLoad(this);
        if (m_handle) {
            m_handle->cancel();
            m_handle = nullptr;
        }
        cleanupForError(nonNullError);
    }

    // If cancel() completed from within the call to willCancel() or didFailToLoad(),
    // we don't want to redo didCancel() or releasesResources().
    if (m_reachedTerminalState)
        return;

    didCancel(nonNullError);

    if (m_cancellationStatus == FinishedCancel)
        return;
    m_cancellationStatus = FinishedCancel;

    releaseResources();
}

ResourceError ResourceLoader::cancelledError()
{
    return frameLoader()->cancelledError(m_request);
}

ResourceError ResourceLoader::blockedError()
{
    return frameLoader()->client().blockedError(m_request);
}

ResourceError ResourceLoader::cannotShowURLError()
{
    return frameLoader()->client().cannotShowURLError(m_request);
}

void ResourceLoader::willSendRequest(ResourceHandle*, ResourceRequest& request, const ResourceResponse& redirectResponse)
{
    if (documentLoader()->applicationCacheHost()->maybeLoadFallbackForRedirect(this, request, redirectResponse))
        return;
    willSendRequestInternal(request, redirectResponse);
}

void ResourceLoader::didSendData(ResourceHandle*, unsigned long long bytesSent, unsigned long long totalBytesToBeSent)
{
    didSendData(bytesSent, totalBytesToBeSent);
}

void ResourceLoader::didReceiveResponse(ResourceHandle*, const ResourceResponse& response)
{
    if (documentLoader()->applicationCacheHost()->maybeLoadFallbackForResponse(this, response))
        return;
    didReceiveResponse(response);
}

void ResourceLoader::didReceiveData(ResourceHandle*, const char* data, unsigned length, int encodedDataLength)
{
    didReceiveData(data, length, encodedDataLength, DataPayloadBytes);
}

void ResourceLoader::didReceiveBuffer(ResourceHandle*, PassRefPtr<SharedBuffer> buffer, int encodedDataLength)
{
    didReceiveBuffer(buffer, encodedDataLength, DataPayloadBytes);
}

void ResourceLoader::didFinishLoading(ResourceHandle*, double finishTime)
{
    didFinishLoading(finishTime);
}

void ResourceLoader::didFail(ResourceHandle*, const ResourceError& error)
{
    if (documentLoader()->applicationCacheHost()->maybeLoadFallbackForError(this, error))
        return;
    didFail(error);
}

void ResourceLoader::wasBlocked(ResourceHandle*)
{
    didFail(blockedError());
}

void ResourceLoader::cannotShowURL(ResourceHandle*)
{
    didFail(cannotShowURLError());
}

bool ResourceLoader::shouldUseCredentialStorage()
{
    if (m_options.allowCredentials() == DoNotAllowStoredCredentials)
        return false;
    
    Ref<ResourceLoader> protect(*this);
    return frameLoader()->client().shouldUseCredentialStorage(documentLoader(), identifier());
}

bool ResourceLoader::isAllowedToAskUserForCredentials() const
{
    if (!m_canAskClientForCredentials)
        return false;
    return m_options.credentials == FetchOptions::Credentials::Include || (m_options.credentials == FetchOptions::Credentials::SameOrigin && m_frame->document()->securityOrigin().canRequest(originalRequest().url()));
}

void ResourceLoader::didReceiveAuthenticationChallenge(const AuthenticationChallenge& challenge)
{
    ASSERT(m_handle->hasAuthenticationChallenge());

    // Protect this in this delegate method since the additional processing can do
    // anything including possibly derefing this; one example of this is Radar 3266216.
    Ref<ResourceLoader> protect(*this);

    if (m_options.allowCredentials() == AllowStoredCredentials) {
        if (isAllowedToAskUserForCredentials()) {
            frameLoader()->notifier().didReceiveAuthenticationChallenge(this, challenge);
            return;
        }
    }
    // Only these platforms provide a way to continue without credentials.
    // If we can't continue with credentials, we need to cancel the load altogether.
#if PLATFORM(COCOA) || USE(CFNETWORK) || USE(CURL) || PLATFORM(GTK) || PLATFORM(EFL)
    challenge.authenticationClient()->receivedRequestToContinueWithoutCredential(challenge);
    ASSERT(!m_handle || !m_handle->hasAuthenticationChallenge());
#else
    didFail(blockedError());
#endif
}

void ResourceLoader::didCancelAuthenticationChallenge(const AuthenticationChallenge& challenge)
{
    // Protect this in this delegate method since the additional processing can do
    // anything including possibly derefing this; one example of this is Radar 3266216.
    Ref<ResourceLoader> protect(*this);
    frameLoader()->notifier().didCancelAuthenticationChallenge(this, challenge);
}

#if USE(PROTECTION_SPACE_AUTH_CALLBACK)

bool ResourceLoader::canAuthenticateAgainstProtectionSpace(const ProtectionSpace& protectionSpace)
{
    Ref<ResourceLoader> protect(*this);
    return frameLoader()->client().canAuthenticateAgainstProtectionSpace(documentLoader(), identifier(), protectionSpace);
}

#endif
    
#if PLATFORM(IOS)

RetainPtr<CFDictionaryRef> ResourceLoader::connectionProperties(ResourceHandle*)
{
    return frameLoader()->connectionProperties(this);
}

#endif

void ResourceLoader::receivedCancellation(const AuthenticationChallenge&)
{
    cancel();
}

#if PLATFORM(MAC)

void ResourceLoader::schedule(SchedulePair& pair)
{
    if (m_handle)
        m_handle->schedule(pair);
}

void ResourceLoader::unschedule(SchedulePair& pair)
{
    if (m_handle)
        m_handle->unschedule(pair);
}

#endif

#if USE(QUICK_LOOK)
void ResourceLoader::didCreateQuickLookHandle(QuickLookHandle& handle)
{
    m_isQuickLookResource = true;
    frameLoader()->client().didCreateQuickLookHandle(handle);
}
#endif

}
