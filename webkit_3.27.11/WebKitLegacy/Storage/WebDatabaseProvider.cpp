/*
 * Copyright (C) 2015 Apple Inc. All rights reserved.
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

#include "config.h"
#include "WebDatabaseProvider.h"

#include <pal/SessionID.h>

WebDatabaseProvider& WebDatabaseProvider::singleton()
{
#if !PLATFORM(WKC)
    static WebDatabaseProvider& databaseProvider = adoptRef(*new WebDatabaseProvider).leakRef();

    return databaseProvider;
#else
    WKC_DEFINE_STATIC_TYPE(WebDatabaseProvider*, databaseProvider, nullptr);
    if (!databaseProvider)
        databaseProvider = &(adoptRef(*new WebDatabaseProvider).leakRef());
    return *databaseProvider;
#endif
}

WebDatabaseProvider::WebDatabaseProvider()
{
}

WebDatabaseProvider::~WebDatabaseProvider()
{
}

#if ENABLE(WKC_INDEXED_DATABASE)
WebCore::IDBClient::IDBConnectionToServer& WebDatabaseProvider::idbConnectionToServerForSession(PAL::SessionID sessionID)
{
    return m_idbServerMap.ensure(sessionID, [&sessionID] {
        return sessionID.isEphemeral() ? InProcessIDBServer::create(sessionID) : InProcessIDBServer::create(sessionID, indexedDatabaseDirectoryPath());
    }).iterator->value->connectionToServer();
}
#endif

void WebDatabaseProvider::deleteAllDatabases()
{
#if ENABLE(WKC_INDEXED_DATABASE)
    for (auto& server : m_idbServerMap.values())
        server->closeAndDeleteDatabasesModifiedSince(-WallTime::infinity());
#endif
}
