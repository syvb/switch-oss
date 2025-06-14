/*
 * Copyright (C) 2006 Apple Computer, Inc.  All rights reserved.
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

#ifndef ResourceResponse_h
#define ResourceResponse_h

#include "ResourceResponseBase.h"

namespace WebCore {

class ResourceHandle;

class ResourceResponse : public ResourceResponseBase {
public:
    ResourceResponse()
        : m_responseFired(false),
          m_resourceHandle(0),
          m_suggestedFilename()
    {
        m_initLevel = 0;
    }

    ResourceResponse(const URL& url, const String& mimeType, long long expectedLength, const String& textEncodingName)
        : ResourceResponseBase(url, mimeType, expectedLength, textEncodingName),
          m_responseFired(false),
          m_resourceHandle(0),
          m_suggestedFilename()
    {
        m_initLevel = 0;
    }

    void setResponseFired(bool fired) { m_responseFired = fired; }
    bool responseFired() const { return m_responseFired; }

    void setResourceHandle(ResourceHandle* resourceHandle) { m_resourceHandle = resourceHandle; }
    ResourceHandle* resourceHandle() const { return m_resourceHandle; }

    const String& platformSuggestedFilename() const { return m_suggestedFilename; }
    void setSuggestedFilename(const String& name) { m_suggestedFilename = name; }

private:
    friend class ResourceResponseBase;

private:
    bool m_responseFired;
    ResourceHandle* m_resourceHandle;
    String m_suggestedFilename;
};

} // namespace WebCore

#endif // ResourceResponse_h
