/*
 * Copyright (C) 2006 Lars Knoll <lars@trolltech.com>
 * Copyright (C) 2004, 2005, 2006, 2007, 2008 Apple Inc. All rights reserved.
 * Copyright (C) 2008 Jurg Billeter <j@bitron.ch>
 * Copyright (C) 2008 Dominik Rottsches <dominik.roettsches@access-company.com>
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Library General Public
 * License as published by the Free Software Foundation; either
 * version 2 of the License, or (at your option) any later version.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Library General Public License for more details.
 *
 * You should have received a copy of the GNU Library General Public License
 * along with this library; see the file COPYING.LIB.  If not, write to
 * the Free Software Foundation, Inc., 51 Franklin Street, Fifth Floor,
 * Boston, MA 02110-1301, USA.
 *
 */

#include "config.h"
#include "TextBreakIterator.h"
#include "CString.h"

#if !defined(WKC_USE_ICU)

#include <wkc/wkcpeer.h>

#include "NotImplemented.h"

namespace WTF {

void
ensureUChar(StringView str, const UChar*& s, int& len)
{
    if (!str.is8Bit()) {
        s = str.characters16();
        len = str.length();
        return;
    }

    const LChar* c = str.characters8();
    int l = str.length();
    UChar* d = (UChar *)WTF::fastMalloc((l+1)*sizeof(UChar));
    memset((void *)d, 0, (l+1) * sizeof(UChar));
    for (int i=0; i<l; i++) {
        d[i] = c[i];
    }
    s = d;
    len = l;
}

void
freeUChar(StringView str, const UChar* s, int len)
{
    if (!str.is8Bit())
        return;
    WTF::fastFree((void *)s);
}

UBreakIterator*
characterBreakIterator(StringView str)
{
    WKC_DEFINE_STATIC_TYPE(void*, gIteratorCharacter, wkcTextBreakIteratorNewPeer(WKC_TEXTBREAKITERATOR_TYPE_CHARACTER));

    if (str.isEmpty()) return 0;
    const UChar* s = 0;
    int len = 0;

    ensureUChar(str, s, len);
    bool ret = wkcTextBreakIteratorSetStringPeer(gIteratorCharacter, s, len);
    freeUChar(str, s, len);
    if (!ret)
        return 0;
    return (UBreakIterator *)gIteratorCharacter;
}

UBreakIterator*
cursorMovementIterator(StringView str)
{
    WKC_DEFINE_STATIC_TYPE(void*, gIteratorCursorMovement, wkcTextBreakIteratorNewPeer(WKC_TEXTBREAKITERATOR_TYPE_CURSORMOVEMENT));

    if (str.isEmpty()) return 0;
    const UChar* s = 0;
    int len = 0;

    ensureUChar(str, s, len);
    bool ret = wkcTextBreakIteratorSetStringPeer(gIteratorCursorMovement, s, len);
    freeUChar(str, s, len);
    if (!ret)
        return 0;
    return (UBreakIterator *)gIteratorCursorMovement;
}

UBreakIterator*
wordBreakIterator(StringView str)
{
    WKC_DEFINE_STATIC_TYPE(void*, gIteratorWord, wkcTextBreakIteratorNewPeer(WKC_TEXTBREAKITERATOR_TYPE_WORD));

    if (str.isEmpty()) return 0;
    const UChar* s = 0;
    int len = 0;

    ensureUChar(str, s, len);
    bool ret = wkcTextBreakIteratorSetStringPeer(gIteratorWord, s, len);
    freeUChar(str, s, len);
    if (!ret)
        return 0;
    return (UBreakIterator *)gIteratorWord;
}

UBreakIterator*
acquireLineBreakIterator(StringView str, const AtomString& /*locale*/, const UChar* priorContext, unsigned int priorContextLength, LineBreakIteratorMode mode)
{
    void* iterator = 0;
    iterator = wkcTextBreakIteratorNewPeer(WKC_TEXTBREAKITERATOR_TYPE_LINE);
    if (!iterator)
        return 0;

    if (str.isEmpty()) {
        wkcTextBreakIteratorDeletePeer(iterator);
        return 0;
    }
    const UChar* s = 0;
    int len = 0;

    ensureUChar(str, s, len);
    bool ret = wkcTextBreakIteratorSetStringPeer(iterator, s, len);
    freeUChar(str, s, len);
    if (!ret) {
        wkcTextBreakIteratorDeletePeer(iterator);
        return 0;
    }
    if (priorContextLength){
        ret = wkcTextBreakIteratorSetPriorContextPeer(iterator, priorContext, priorContextLength);
        // ignore return value.
    }
    return (UBreakIterator *)iterator;
}

void
releaseLineBreakIterator(UBreakIterator* self)
{
    if (self)
        wkcTextBreakIteratorReleasePeer((void *)self);
}

NonSharedCharacterBreakIterator::NonSharedCharacterBreakIterator(StringView sv)
    : m_iterator(nullptr)
{
    m_iterator = (UBreakIterator *)wkcTextBreakIteratorNewPeer(WKC_TEXTBREAKITERATOR_TYPE_CHARACTER_NONSHARED);
    if (!m_iterator)
        CRASH();
    if (sv.is8Bit()) {
        UChar* p = (UChar *)WTF::fastZeroedMalloc((sv.length()+1) * sizeof(UChar));
        for (size_t i=0; i<sv.length(); i++) {
            p[i] = sv.characters8()[i];
        }
        wkcTextBreakIteratorSetStringPeer((void *)m_iterator, p, sv.length());
        WTF::fastFree(p);
    } else {
        wkcTextBreakIteratorSetStringPeer((void *)m_iterator, sv.characters16(), sv.length());
    }
}

NonSharedCharacterBreakIterator::~NonSharedCharacterBreakIterator()
{
    if (m_iterator)
        wkcTextBreakIteratorDeletePeer((void* )m_iterator);
}

NonSharedCharacterBreakIterator::NonSharedCharacterBreakIterator(NonSharedCharacterBreakIterator&& other)
    : m_iterator(nullptr)
{
    std::swap(m_iterator, other.m_iterator);
}

UBreakIterator*
sentenceBreakIterator(StringView str)
{
    WKC_DEFINE_STATIC_TYPE(void*, gIteratorSentence, wkcTextBreakIteratorNewPeer(WKC_TEXTBREAKITERATOR_TYPE_SENTENCE));

    if (str.isEmpty()) return 0;
    const UChar* s = 0;
    int len = 0;

    ensureUChar(str, s, len);
    bool ret = wkcTextBreakIteratorSetStringPeer(gIteratorSentence, s, len);
    freeUChar(str, s, len);
    if (!ret)
        return 0;
    return (UBreakIterator *)gIteratorSentence;
}

bool
isWordTextBreak(UBreakIterator* self)
{
    if (!self) return true;

    notImplemented();
    return true;
}

const char*
currentSearchLocaleID()
{
    return "ja-JP";
}

const char*
currentTextBreakLocaleID()
{
    return "ja-JP";
}

unsigned numGraphemeClusters(StringView s)
{
    unsigned stringLength = s.length();
    
    if (!stringLength)
        return 0;

    // The only Latin-1 Extended Grapheme Cluster is CR LF
    if (s.is8Bit() && !s.contains('\r'))
        return stringLength;

    NonSharedCharacterBreakIterator it(s);
    if (!it)
        return stringLength;

    unsigned num = 0;
    while (ubrk_next(it) != UBRK_DONE)
        ++num;
    return num;
}

unsigned numCodeUnitsInGraphemeClusters(StringView s, unsigned numGraphemeClusters)
{
    unsigned stringLength = s.length();

    if (!stringLength)
        return 0;

    // The only Latin-1 Extended Grapheme Cluster is CR LF
    if (s.is8Bit() && !s.contains('\r'))
        return std::min(stringLength, numGraphemeClusters);

    NonSharedCharacterBreakIterator it(s);
    if (!it)
        return std::min(stringLength, numGraphemeClusters);

    for (unsigned i = 0; i < numGraphemeClusters; ++i) {
        if (ubrk_next(it) == UBRK_DONE)
            return stringLength;
    }
    return ubrk_current(it);
}

bool
isCJKLocale(const AtomString& locale)
{
    size_t length = locale.length();
    if (length < 2)
        return false;
    auto c1 = locale[0];
    auto c2 = locale[1];
    auto c3 = length == 2 ? 0 : locale[2];
    if (!c3 || c3 == '-' || c3 == '_' || c3 == '@') {
        if (c1 == 'z' || c1 == 'Z')
            return c2 == 'h' || c2 == 'H';
        if (c1 == 'j' || c1 == 'J')
            return c2 == 'a' || c2 == 'A';
        if (c1 == 'k' || c1 == 'K')
            return c2 == 'o' || c2 == 'O';
    }
    return false;
}

}

#endif
