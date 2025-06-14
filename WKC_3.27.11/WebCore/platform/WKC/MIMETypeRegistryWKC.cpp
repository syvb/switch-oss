/*
 * Copyright (C) 2006 Zack Rusin <zack@kde.org>
 * Copyright (C) 2006 Apple Computer, Inc.  All rights reserved.
 * Copyright (C) 2008 Nokia Corporation and/or its subsidiary(-ies)
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
 * THIS SOFTWARE IS PROVIDED BY APPLE COMPUTER, INC. ``AS IS'' AND ANY
 * EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR
 * PURPOSE ARE DISCLAIMED.  IN NO EVENT SHALL APPLE COMPUTER, INC. OR
 * CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL,
 * EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO,
 * PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR
 * PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY
 * OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
 * (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
 * OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */

#include "config.h"
#include "MIMETypeRegistry.h"
#include "MHTMLArchive.h"

#include "NotImplemented.h"

#if ENABLE(MEDIA_SOURCE)
#include "CString.h"
#include <wkc/wkcmediapeer.h>
#endif

namespace WebCore {

struct ExtensionMap {
    const char* extension;
    const char* mimeType;
};

static const ExtensionMap extensionMap [] = {
    { "bmp", "image/bmp" },
    { "css", "text/css" },
    { "gif", "image/gif" },
    { "html", "text/html" },
    { "htm", "text/html" },
    { "ico", "image/x-icon" },
    { "jpeg", "image/jpeg" },
    { "jpg", "image/jpeg" },
    { "js", "application/x-javascript" },
    { "pdf", "application/pdf" },
    { "png", "image/png" },
    { "rss", "application/rss+xml" },
    { "svg", "image/svg+xml" },
    { "text", "text/plain" },
    { "txt", "text/plain" },
    { "xml", "text/xml" },
    { "xsl", "text/xsl" },
    { "xhtml", "application/xhtml+xml" },
    { "wml", "text/vnd.wap.wml" },
    { "wmlc", "application/vnd.wap.wmlc" },
    { "mht", "multipart/related" },
    { "mhtml", "multipart/related" },
#if USE(WEBP)
    { "webp", "image/webp" },
#endif
#if USE(AVIF)
    { "avif", "image/avif" },
#endif
};

String MIMETypeRegistry::mimeTypeForExtension(const String &extension)
{
    for (auto& entry : extensionMap) {
        if (equalIgnoringASCIICase(extension, entry.extension))
            return entry.mimeType;
    }
    return String();
}

bool
MIMETypeRegistry::isApplicationPluginMIMEType(const String& mimeType)
{
    return false;
}

String MIMETypeRegistry::preferredExtensionForMIMEType(const String& mimeType)
{
    for (auto& entry : extensionMap) {
        if (equalIgnoringASCIICase(mimeType, entry.mimeType))
            return entry.extension;
    }
    return emptyString();
}

Vector<String> MIMETypeRegistry::extensionsForMIMEType(const String& type)
{
    notImplemented();
    return { };
}

}
