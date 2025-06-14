/*
    Copyright (C) 1998 Lars Knoll (knoll@mpi-hd.mpg.de)
    Copyright (C) 2001 Dirk Mueller (mueller@kde.org)
    Copyright (C) 2002 Waldo Bastian (bastian@kde.org)
    Copyright (C) 2006 Samuel Weinig (sam.weinig@gmail.com)
    Copyright (C) 2004, 2005, 2006, 2007, 2008 Apple Inc. All rights reserved.

    This library is free software; you can redistribute it and/or
    modify it under the terms of the GNU Library General Public
    License as published by the Free Software Foundation; either
    version 2 of the License, or (at your option) any later version.

    This library is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
    Library General Public License for more details.

    You should have received a copy of the GNU Library General Public License
    along with this library; see the file COPYING.LIB.  If not, write to
    the Free Software Foundation, Inc., 51 Franklin Street, Fifth Floor,
    Boston, MA 02110-1301, USA.

    This class provides all functionality needed for loading images, style sheets and html
    pages from the web. It has a memory cache for these objects.
*/

#include "config.h"
#include "CachedScript.h"

#include "CachedResourceClient.h"
#include "CachedResourceRequest.h"
#include "RuntimeApplicationChecks.h"
#include "SharedBuffer.h"
#include "TextResourceDecoder.h"

#if PLATFORM(WKC) && !defined(WKC_USE_ICU)
#include "TextCodecWKC.h"
#include "TextEncodingRegistry.h"
#endif

namespace WebCore {

CachedScript::CachedScript(CachedResourceRequest&& request, PAL::SessionID sessionID, const CookieJar* cookieJar)
    : CachedResource(WTFMove(request), Type::Script, sessionID, cookieJar)
    , m_decoder(TextResourceDecoder::create("text/javascript"_s, request.charset()))
{
}

CachedScript::~CachedScript() = default;

void CachedScript::setEncoding(const String& chs)
{
    m_decoder->setEncoding(chs, TextResourceDecoder::EncodingFromHTTPHeader);
}

String CachedScript::encoding() const
{
    return m_decoder->encoding().name();
}

StringView CachedScript::script()
{
    if (!m_data)
        return emptyString();

    if (!m_data->isContiguous())
        m_data = m_data->makeContiguous();

    auto& contiguousData = downcast<SharedBuffer>(*m_data);
    if (m_decodingState == NeverDecoded
        && PAL::TextEncoding(encoding()).isByteBasedEncoding()
        && m_data->size()
        && charactersAreAllASCII(contiguousData.data(), m_data->size())) {

        m_decodingState = DataAndDecodedStringHaveSameBytes;

        // If the encoded and decoded data are the same, there is no decoded data cost!
        setDecodedSize(0);
        m_decodedDataDeletionTimer.stop();

        m_scriptHash = StringHasher::computeHashAndMaskTop8Bits(contiguousData.data(), m_data->size());
    }

    if (m_decodingState == DataAndDecodedStringHaveSameBytes)
        return { contiguousData.data(), static_cast<unsigned>(m_data->size()) };

    if (!m_script) {
#if PLATFORM(WKC)
#if defined(WKC_USE_ICU)
        const uint8_t* data = contiguousData.data();
        if (!data || !encodedSize()) {
            goto setEmptyScript;
        }
        // Rough size estimate for the script after decoding.
        size_t bytesToBeAllocated = encodedSize() * 2; // Multiply by 2 for 16-bit characters.
        if (0 < bytesToBeAllocated && !wkcMemoryCheckMemoryAllocatablePeer(bytesToBeAllocated, WKC_MEMORYALLOC_TYPE_JAVASCRIPT)) {
            // Failed to allocate continuous memory area for decoded script.
            wkcMemoryNotifyMemoryAllocationErrorPeer(bytesToBeAllocated, WKC_MEMORYALLOC_TYPE_JAVASCRIPT);
            goto setEmptyScript;
        }
#else
        const uint8_t* data;
        unsigned int encodedLength;
        int decodedLength;
        std::unique_ptr<PAL::TextCodec> codecPtr;
        data = contiguousData.data();
        encodedLength = encodedSize();
        if (!data || !encodedLength) {
            goto setEmptyScript;
        }

        decodedLength = 0;
        codecPtr = PAL::newTextCodec(m_decoder->encoding());
        if (codecPtr->isTextCodecWKC()) {
            PAL::TextCodecWKC* codec = static_cast<PAL::TextCodecWKC*>(codecPtr.get());
            decodedLength = codec->getDecodedTextLength(reinterpret_cast<const char*>(data), encodedLength);
        } else {
            // encoding is Latin1 or UserDefined
            decodedLength = encodedLength;
            //How about the case where encoding is UTF-16???
            //decodedLength = encodedLength / 2;
        }

        size_t bytesToBeAllocated = decodedLength * sizeof(UChar) * 2;
        if (0 < bytesToBeAllocated && !wkcMemoryCheckMemoryAllocatablePeer(bytesToBeAllocated, WKC_MEMORYALLOC_TYPE_JAVASCRIPT)) {
            // Failed to allocate continuous memory area for decoded script.
            wkcMemoryNotifyMemoryAllocationErrorPeer(bytesToBeAllocated, WKC_MEMORYALLOC_TYPE_JAVASCRIPT);
            goto setEmptyScript;
        }
#endif
#endif
        m_script = m_decoder->decodeAndFlush(contiguousData.data(), encodedSize());
        ASSERT(!m_scriptHash || m_scriptHash == m_script.impl()->hash());
        if (m_decodingState == NeverDecoded)
            m_scriptHash = m_script.impl()->hash();
        m_decodingState = DataAndDecodedStringHaveDifferentBytes;
        setDecodedSize(m_script.sizeInBytes());
    }

#if PLATFORM(WKC)
    goto junction;

setEmptyScript:
    m_script = emptyString();
    setDecodedSize(0);
    setStatus(DecodeError);

junction:
    ;
#endif

    m_decodedDataDeletionTimer.restart();
    return m_script;
}

unsigned CachedScript::scriptHash()
{
    if (m_decodingState == NeverDecoded)
        script();
    return m_scriptHash;
}

void CachedScript::finishLoading(const FragmentedSharedBuffer* data, const NetworkLoadMetrics& metrics)
{
    if (data) {
        m_data = data->makeContiguous();
        setEncodedSize(data->size());
    } else {
        m_data = nullptr;
        setEncodedSize(0);
    }
    CachedResource::finishLoading(data, metrics);
}

void CachedScript::destroyDecodedData()
{
    m_script = String();
    setDecodedSize(0);
}

void CachedScript::setBodyDataFrom(const CachedResource& resource)
{
    ASSERT(resource.type() == type());
    auto& script = static_cast<const CachedScript&>(resource);

    CachedResource::setBodyDataFrom(resource);

    m_script = script.m_script;
    m_scriptHash = script.m_scriptHash;
    m_decodingState = script.m_decodingState;
    m_decoder = script.m_decoder;
}

bool CachedScript::shouldIgnoreHTTPStatusCodeErrors() const
{
#if PLATFORM(MAC)
    // This is a workaround for <rdar://problem/13916291>
    // REGRESSION (r119759): Adobe Flash Player "smaller" installer relies on the incorrect firing
    // of a load event and needs an app-specific hack for compatibility.
    // The installer in question tries to load .js file that doesn't exist, causing the server to
    // return a 404 response. Normally, this would trigger an error event to be dispatched, but the
    // installer expects a load event instead so we work around it here.
    if (MacApplication::isSolidStateNetworksDownloader())
        return true;
#endif

    return CachedResource::shouldIgnoreHTTPStatusCodeErrors();
}

} // namespace WebCore
