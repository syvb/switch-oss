/*
 * Copyright (C) 2013-2021 Apple Inc. All rights reserved.
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

#pragma once

#include "GenericTypedArrayView.h"
#include "JSGlobalObjectInlines.h"

namespace JSC {

template<typename Adaptor>
GenericTypedArrayView<Adaptor>::GenericTypedArrayView(
RefPtr<ArrayBuffer>&& buffer, size_t byteOffset, size_t length)
    : ArrayBufferView(WTFMove(buffer), byteOffset, length * sizeof(typename Adaptor::Type))
{
    ASSERT((length / sizeof(typename Adaptor::Type)) < std::numeric_limits<size_t>::max());
}

template<typename Adaptor>
Ref<GenericTypedArrayView<Adaptor>> GenericTypedArrayView<Adaptor>::create(size_t length)
{
    auto result = tryCreate(length);
#if !PLATFORM(WKC)
    RELEASE_ASSERT(result);
#else
    if (!result)
        CRASH_WITH_NO_MEMORY();
#endif
    return result.releaseNonNull();
}

template<typename Adaptor>
Ref<GenericTypedArrayView<Adaptor>> GenericTypedArrayView<Adaptor>::create(
    const typename Adaptor::Type* array, size_t length)
{
    auto result = tryCreate(array, length);
#if !PLATFORM(WKC)
    RELEASE_ASSERT(result);
#else
    if (!result)
        CRASH_WITH_NO_MEMORY();
#endif
    return result.releaseNonNull();
}

template<typename Adaptor>
Ref<GenericTypedArrayView<Adaptor>> GenericTypedArrayView<Adaptor>::create(
    RefPtr<ArrayBuffer>&& buffer, size_t byteOffset, size_t length)
{
    auto result = tryCreate(WTFMove(buffer), byteOffset, length);
#if !PLATFORM(WKC)
    RELEASE_ASSERT(result);
#else
    if (!result)
        CRASH_WITH_NO_MEMORY();
#endif
    return result.releaseNonNull();
}

template<typename Adaptor>
RefPtr<GenericTypedArrayView<Adaptor>> GenericTypedArrayView<Adaptor>::tryCreate(size_t length)
{
    auto buffer = ArrayBuffer::tryCreate(length, sizeof(typename Adaptor::Type));
    if (!buffer)
        return nullptr;
    return tryCreate(WTFMove(buffer), 0, length);
}

template<typename Adaptor>
RefPtr<GenericTypedArrayView<Adaptor>> GenericTypedArrayView<Adaptor>::tryCreate(
    const typename Adaptor::Type* array, size_t length)
{
    RefPtr<GenericTypedArrayView> result = tryCreate(length);
    if (!result)
        return nullptr;
    memcpy(result->data(), array, length * sizeof(typename Adaptor::Type));
    return result;
}

template<typename Adaptor>
RefPtr<GenericTypedArrayView<Adaptor>> GenericTypedArrayView<Adaptor>::tryCreate(
    RefPtr<ArrayBuffer>&& buffer, size_t byteOffset, size_t length)
{
    if (!buffer)
        return nullptr;
    
    if (!ArrayBufferView::verifySubRangeLength(*buffer, byteOffset, length, sizeof(typename Adaptor::Type))
        || !verifyByteOffsetAlignment(byteOffset, sizeof(typename Adaptor::Type))) {
        return nullptr;
    }
    
    return adoptRef(new GenericTypedArrayView(WTFMove(buffer), byteOffset, length));
}

template<typename Adaptor>
Ref<GenericTypedArrayView<Adaptor>>
GenericTypedArrayView<Adaptor>::createUninitialized(size_t length)
{
    auto result = tryCreateUninitialized(length);
#if !PLATFORM(WKC)
    RELEASE_ASSERT(result);
#else
    if (!result)
        CRASH_WITH_NO_MEMORY();
#endif
    return result.releaseNonNull();
}

template<typename Adaptor>
RefPtr<GenericTypedArrayView<Adaptor>>
GenericTypedArrayView<Adaptor>::tryCreateUninitialized(size_t length)
{
    RefPtr<ArrayBuffer> buffer =
        ArrayBuffer::tryCreateUninitialized(length, sizeof(typename Adaptor::Type));
    if (!buffer)
        return nullptr;
    return tryCreate(WTFMove(buffer), 0, length);
}

template<typename Adaptor>
JSArrayBufferView* GenericTypedArrayView<Adaptor>::wrap(JSGlobalObject* lexicalGlobalObject, JSGlobalObject* globalObject)
{
    UNUSED_PARAM(lexicalGlobalObject);
    return Adaptor::JSViewType::create(globalObject->vm(), globalObject->typedArrayStructure(Adaptor::typeValue), this);
}

} // namespace JSC
