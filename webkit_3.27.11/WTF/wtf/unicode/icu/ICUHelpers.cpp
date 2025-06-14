/*
 * Copyright (C) 2020 Apple Inc. All Rights Reserved.
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

#include "config.h"
#include <wtf/unicode/icu/ICUHelpers.h>

#include <mutex>
#include <unicode/uvernum.h>

namespace WTF {
namespace ICU {

#if !PLATFORM(WKC) || defined(WKC_USE_ICU)
static const UVersionInfo& version()
{
    static UVersionInfo versions { };
    static WKCStd::once_flag onceKey;
    WKCStd::call_once(onceKey, [&] {
        u_getVersion(versions);
    });
    return versions;
}
#endif

unsigned majorVersion()
{
#if !PLATFORM(WKC) || defined(WKC_USE_ICU)
    static_assert(0 < U_MAX_VERSION_LENGTH);
    return version()[0];
#else
    return 0;
#endif
}

unsigned minorVersion()
{
#if !PLATFORM(WKC) || defined(WKC_USE_ICU)
    static_assert(1 < U_MAX_VERSION_LENGTH);
    return version()[1];
#else
    return 0;
#endif
}

} } // namespace WTF::ICU
