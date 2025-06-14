/*
 * Copyright (C) 2011 Apple Inc. All Rights Reserved.
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

#ifndef WebSocketServerClient_h
#define WebSocketServerClient_h

#if ENABLE(REMOTE_INSPECTOR)

#include "HTTPRequest.h"

namespace WTF {
class String;
}

namespace WKC {

class HTTPRequest;
class WebSocketServerConnection;

class WebSocketServerClient {
public:
    virtual ~WebSocketServerClient() { }

    // Received an HTTP request but didn't know what to do with it.
    virtual void didReceiveUnrecognizedHTTPRequest(WebSocketServerConnection*, RefPtr<HTTPRequest>) { }

    // Received a WebSocket Upgrade HTTP request. Ask if we should handle it and upgrade.
    virtual bool didReceiveWebSocketUpgradeHTTPRequest(WebSocketServerConnection*, RefPtr<HTTPRequest>) { return true; }

    // Established a WebSocket Connection.
    virtual void didEstablishWebSocketConnection(WebSocketServerConnection*, RefPtr<HTTPRequest>) { }

    // Received a WebSocket message.
    virtual void didReceiveWebSocketMessage(WebSocketServerConnection*, const WTF::String&) { }

    // WebSocket Connection closed.
    virtual void didCloseWebSocketConnection(WebSocketServerConnection*) { }
};

}

#endif // ENABLE(REMOTE_INSPECTOR)

#endif // WebSocketServerClient_h
