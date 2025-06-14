/*
 * Copyright (C) 2019 Apple Inc. All rights reserved.
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

#pragma once

#if ENABLE(LAYOUT_FORMATTING_CONTEXT)

#include "InlineItem.h"

namespace WebCore {
namespace Layout {

class InlineSoftLineBreakItem : public InlineItem {
public:
#if !PLATFORM(WKC)
    static InlineSoftLineBreakItem createSoftLineBreakItem(const InlineTextBox&, unsigned position, UBiDiLevel = UBIDI_DEFAULT_LTR);
#else
    inline static InlineSoftLineBreakItem createSoftLineBreakItem(const InlineTextBox&, unsigned position, UBiDiLevel = UBIDI_DEFAULT_LTR);
#endif

    unsigned position() const { return m_startOrPosition; }
    const InlineTextBox& inlineTextBox() const { return downcast<InlineTextBox>(layoutBox()); }

private:
#if !PLATFORM(WKC)
    InlineSoftLineBreakItem(const InlineTextBox&, unsigned position, UBiDiLevel);
#else
    InlineSoftLineBreakItem(const Box& inlineBox, unsigned position, UBiDiLevel bidiLevel)
        : InlineItem(inlineBox, Type::SoftLineBreak, bidiLevel)
    {
        m_startOrPosition = position;
    }
#endif
};

inline InlineSoftLineBreakItem InlineSoftLineBreakItem::createSoftLineBreakItem(const InlineTextBox& inlineTextBox, unsigned position, UBiDiLevel bidiLevel)
{
    return { inlineTextBox, position, bidiLevel };
}

#if !PLATFORM(WKC)
inline InlineSoftLineBreakItem::InlineSoftLineBreakItem(const InlineTextBox& inlineTextBox, unsigned position, UBiDiLevel bidiLevel)
    : InlineItem(inlineTextBox, Type::SoftLineBreak, bidiLevel)
{
    m_startOrPosition = position;
}
#endif

}
}

SPECIALIZE_TYPE_TRAITS_INLINE_ITEM(InlineSoftLineBreakItem, isSoftLineBreak())

#endif
