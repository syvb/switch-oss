/*
 * Copyright (C) 2008 Apple Inc. All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 *
 * 1.  Redistributions of source code must retain the above copyright
 *     notice, this list of conditions and the following disclaimer.
 * 2.  Redistributions in binary form must reproduce the above copyright
 *     notice, this list of conditions and the following disclaimer in the
 *     documentation and/or other materials provided with the distribution.
 * 3.  Neither the name of Apple Computer, Inc. ("Apple") nor the names of
 *     its contributors may be used to endorse or promote products derived
 *     from this software without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY APPLE AND ITS CONTRIBUTORS "AS IS" AND ANY
 * EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED
 * WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE
 * DISCLAIMED. IN NO EVENT SHALL APPLE OR ITS CONTRIBUTORS BE LIABLE FOR ANY
 * DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES
 * (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES;
 * LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND
 * ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
 * (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF
 * THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */

#include "config.h"

#if !defined(WKC_USE_ICU)

#include "Collator.h"
#include "StringView.h"

#include <wkc/wkcpeer.h>

namespace WTF {

Collator::Collator(const char*, bool shouldSortLowercaseFirst)
    : m_locale(0)
    , m_shouldSortLowercaseFirst(shouldSortLowercaseFirst)
    , m_collator(nullptr)
{
}

Collator::~Collator()
{
}

int Collator::collateUTF8(const char* lhs, const char* rhs) const
{
    size_t lhsLength = strlen(lhs);
    size_t rhsLength = strlen(rhs);

    int lmin = lhsLength < rhsLength ? lhsLength : rhsLength;
    int l = 0;
    while (l < lmin && *lhs == *rhs) {
        lhs++;
        rhs++;
        l++;
    }

    if (l < lmin) {
        int lv = *lhs;
        int rv = *rhs;
        int lf = wkcUnicodeFoldCasePeer(lv);
        int rf = wkcUnicodeFoldCasePeer(rv);
        if (lf==rf) {
            if (m_shouldSortLowercaseFirst)
                return (rv > lv) ? 1 : -1;
            else
                return (lv > rv) ? 1 : -1;
        }
        return (lf > rf) ? 1 : -1;
    }

    if (lhsLength == rhsLength)
        return 0;

    return (lhsLength > rhsLength) ? 1 : -1;
}

int Collator::collate(StringView lhs, StringView rhs) const
{
    // only act as us-ascii collator...

    size_t lhsLength = lhs.length();
    size_t rhsLength = rhs.length();

    int lmin = lhsLength < rhsLength ? lhsLength : rhsLength;
    int l = 0;
    while (l < lmin && lhs[l] == rhs[l]) {
        l++;
    }

    if (l < lmin) {
        int lv = lhs[l];
        int rv = rhs[l];
        int lf = wkcUnicodeFoldCasePeer(lv);
        int rf = wkcUnicodeFoldCasePeer(rv);
        if (lf==rf) {
            if (m_shouldSortLowercaseFirst)
                return (rv > lv) ? 1 : -1;
            else
                return (lv > rv) ? 1 : -1;
        }
        return (lf > rf) ? 1 : -1;
    }

    if (lhsLength == rhsLength)
        return 0;

    return (lhsLength > rhsLength) ? 1 : -1;
}

}

#endif
