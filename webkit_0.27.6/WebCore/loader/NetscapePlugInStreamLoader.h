/*
 * Copyright (C) 2005, 2006, 2008 Apple Inc. All rights reserved.
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

#ifndef NetscapePlugInStreamLoader_h
#define NetscapePlugInStreamLoader_h

#include "ResourceLoader.h"
#include <wtf/Forward.h>

namespace WebCore {

class NetscapePlugInStreamLoader;

class NetscapePlugInStreamLoaderClient {
public:
#if PLATFORM(WKC)
    virtual void willSendRequest(NetscapePlugInStreamLoader*, ResourceRequest&&, const ResourceResponse& redirectResponse, WTF::Function<void (ResourceRequest&&)>&&) = 0;
#else
    virtual void willSendRequest(NetscapePlugInStreamLoader*, ResourceRequest&&, const ResourceResponse& redirectResponse, std::function<void (ResourceRequest&&)>&&) = 0;
#endif
    virtual void didReceiveResponse(NetscapePlugInStreamLoader*, const ResourceResponse&) = 0;
    virtual void didReceiveData(NetscapePlugInStreamLoader*, const char*, int) = 0;
    virtual void didFail(NetscapePlugInStreamLoader*, const ResourceError&) = 0;
    virtual void didFinishLoading(NetscapePlugInStreamLoader*) { }
    virtual bool wantsAllStreams() const { return false; }

protected:
    virtual ~NetscapePlugInStreamLoaderClient() { }
};

class NetscapePlugInStreamLoader final : public ResourceLoader {
public:
    WEBCORE_EXPORT static PassRefPtr<NetscapePlugInStreamLoader> create(Frame*, NetscapePlugInStreamLoaderClient*, const ResourceRequest&);
    virtual ~NetscapePlugInStreamLoader();

    WEBCORE_EXPORT bool isDone() const;

private:
    virtual bool init(const ResourceRequest&) override;

#if PLATFORM(WKC)
    virtual void willSendRequest(ResourceRequest&&, const ResourceResponse& redirectResponse, WTF::Function<void(ResourceRequest&&)>&& callback) override;
#else
    virtual void willSendRequest(ResourceRequest&&, const ResourceResponse& redirectResponse, std::function<void(ResourceRequest&&)>&& callback) override;
#endif
    virtual void didReceiveResponse(const ResourceResponse&) override;
    virtual void didReceiveData(const char*, unsigned, long long encodedDataLength, DataPayloadType) override;
    virtual void didReceiveBuffer(PassRefPtr<SharedBuffer>, long long encodedDataLength, DataPayloadType) override;
    virtual void didFinishLoading(double finishTime) override;
    virtual void didFail(const ResourceError&) override;

    virtual void releaseResources() override;

    NetscapePlugInStreamLoader(Frame*, NetscapePlugInStreamLoaderClient*);

    virtual void willCancel(const ResourceError&) override;
    virtual void didCancel(const ResourceError&) override;

    void didReceiveDataOrBuffer(const char*, int, PassRefPtr<SharedBuffer>, long long encodedDataLength, DataPayloadType);

    void notifyDone();

    NetscapePlugInStreamLoaderClient* m_client;
    bool m_isInitialized { false };
};

}

#endif // NetscapePlugInStreamLoader_h
