/*
 * Copyright (C) 2006, 2007, 2008 Apple Inc. All rights reserved.
 *           (C) 2008, 2009 Torch Mobile Inc. All rights reserved. (http://www.torchmobile.com/)
 * Copyright (C) 2013 Andrew Bortz. All rights reserved.
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
#include <wtf/RandomNumber.h>

#include <limits>
#include <stdint.h>
#include <wtf/CryptographicallyRandomNumber.h>
#include <wtf/WeakRandom.h>

#if PLATFORM(WKC)
#include "NeverDestroyed.h"
#endif

namespace WTF {

double randomNumber()
{
    uint32_t bits = cryptographicallyRandomNumber();
    return static_cast<double>(bits) / (static_cast<double>(std::numeric_limits<uint32_t>::max()) + 1.0);
}

unsigned weakRandomUint32()
{
    // We don't care about thread safety. WeakRandom just uses POD types,
    // and racy access just increases randomness.
#if !PLATFORM(WKC)
    static WeakRandom s_weakRandom;
    return s_weakRandom.getUint32();
#else
    static LazyNeverDestroyed<WeakRandom> s_weakRandom;
    if (s_weakRandom.isNull())
        s_weakRandom.construct();
    return s_weakRandom.get().getUint32();
#endif
}

}
