/*
 * Copyright (C) 2011 Apple Inc. All Rights Reserved.
 * Copyright (C) 2012 Nokia Corporation and/or its subsidiary(-ies)
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
 * THIS SOFTWARE IS PROVIDED BY APPLE INC. ``AS IS'' AND ANY
 * EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR
 * PURPOSE ARE DISCLAIMED.  IN NO EVENT SHALL APPLE INC. OR
 * CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL,
 * EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO,
 * PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR
 * PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY
 * OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
 * (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
 * OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */

#ifndef WebSocketServerConnection_h
#define WebSocketServerConnection_h

#if ENABLE(REMOTE_INSPECTOR)

#include "SocketStreamHandleClient.h"
#include <wtf/RefPtr.h>
#include <wtf/Vector.h>
#include <wtf/RefCounted.h>

namespace WebCore {
class HTTPHeaderMap;
class SocketStreamHandle;
class SocketStreamServer;
}

namespace WTF {
class String;
}

namespace WKC {

class HTTPRequest;
class WebSocketServer;
class WebSocketServerClient;

class WebSocketServerConnection : public WebCore::SocketStreamHandleClient, public RefCounted<WebSocketServerConnection> {
WTF_MAKE_FAST_ALLOCATED;
public:
    enum WebSocketServerMode { HTTP, WebSocket };
    WebSocketServerConnection(int socket, WebSocketServerClient*, WebSocketServer*);
    virtual ~WebSocketServerConnection();

    unsigned identifier() const { return m_identifier; }
    void setIdentifier(unsigned id) { m_identifier = id; }

    // Sending data over the connection.
    void sendWebSocketMessage(const WTF::String& message);
    void sendHTTPResponseHeader(int statusCode, const WTF::String& statusText, const WebCore::HTTPHeaderMap& headerFields);
    void sendRawData(const uint8_t* data, size_t length);

    // Terminating the connection.
    void shutdownNow();
    void shutdownAfterSendOrNow();

    // SocketStreamHandleClient implementation.
    void didOpenSocketStream(WebCore::SocketStreamHandle&) final;
    void didCloseSocketStream(WebCore::SocketStreamHandle&) final;
    void didReceiveSocketStreamData(WebCore::SocketStreamHandle&, const uint8_t* data, size_t length) final;
    void didFailToReceiveSocketStreamData(WebCore::SocketStreamHandle&) final;
    void didUpdateBufferedAmount(WebCore::SocketStreamHandle&, size_t bufferedAmount) final;
    void didFailSocketStream(WebCore::SocketStreamHandle&, const WebCore::SocketStreamError&) final;

private:
    // HTTP Mode.
    void readHTTPMessage();

    // WebSocket Mode.
    void upgradeToWebSocketServerConnection(RefPtr<HTTPRequest>);
    void readWebSocketFrames();
    bool readWebSocketFrame();

protected:
    unsigned m_identifier;
    Vector<uint8_t> m_bufferedData;
    WebSocketServerMode m_mode;
    RefPtr<WebCore::SocketStreamServer> m_socket;
    WebSocketServer* m_server;
    WebSocketServerClient* m_client;
};

}

#endif // ENABLE(REMOTE_INSPECTOR)

#endif // WebSocketServerConnection_h
