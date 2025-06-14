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

#ifndef WebSocketServer_h
#define WebSocketServer_h

#if ENABLE(REMOTE_INSPECTOR)

#include <wtf/Deque.h>
#include <wtf/text/WTFString.h>

namespace WebCore {
class SocketStreamServer;
}

namespace WTF {
class String;
}

namespace WKC {
    
class WKCTcpServerHandler;
class WebSocketServerClient;
class WebSocketServerConnection;

class WebSocketServer {
WTF_MAKE_FAST_ALLOCATED;
public:
    enum ServerState { Closed, Listening };
    WebSocketServer(WebSocketServerClient*);
    virtual ~WebSocketServer();

    // Server operations.
    bool listen(const WTF::String& bindAddress, unsigned short port);
    void close();

    void didAcceptConnection(int socket);

private:
    void didCloseWebSocketServerConnection(WebSocketServerConnection*);

    void platformInitialize();
    bool platformListen(const WTF::String& bindAddress, unsigned short port);
    void platformClose();

    ServerState m_state;
    Deque<RefPtr<WebSocketServerConnection> > m_connections;
    WebSocketServerClient* m_client;
#if PLATFORM(WKC)
    RefPtr<WKCTcpServerHandler> m_tcpServerHandler;
#endif
    friend class WebSocketServerConnection;
};

}

#endif // ENABLE(REMOTE_INSPECTOR)

#endif // WebSocketServer_h
