/*
 * Copyright (C) 2021 Apple Inc. All rights reserved.
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
#include "RTCRtpSFrameTransformErrorEvent.h"

#if ENABLE(WEB_RTC)

#include "EventNames.h"
#include <wtf/IsoMallocInlines.h>

#if PLATFORM(WKC)
using WebCore::HTMLNames::x_apple_editable_imageAttr;
using WebCore::HTMLNames::widthAttr;
using WebCore::HTMLNames::heightAttr;
using WebCore::HTMLNames::styleAttr;
#endif

namespace WebCore {

WTF_MAKE_ISO_ALLOCATED_IMPL(RTCRtpSFrameTransformErrorEvent);

Ref<RTCRtpSFrameTransformErrorEvent> RTCRtpSFrameTransformErrorEvent::create(CanBubble canBubble, IsCancelable isCancelable, Type errorType)
{
    return adoptRef(*new RTCRtpSFrameTransformErrorEvent(eventNames().errorEvent, canBubble, isCancelable, errorType));
}

Ref<RTCRtpSFrameTransformErrorEvent> RTCRtpSFrameTransformErrorEvent::create(const AtomString& type, Init&& init)
{
    return adoptRef(*new RTCRtpSFrameTransformErrorEvent(type, init.bubbles ? CanBubble::Yes : CanBubble::No,
        init.cancelable ? IsCancelable::Yes : IsCancelable::No, init.errorType));
}

RTCRtpSFrameTransformErrorEvent::RTCRtpSFrameTransformErrorEvent(const AtomString& type, CanBubble canBubble, IsCancelable cancelable, Type errorType)
    : Event(type, canBubble, cancelable)
    , m_errorType(errorType)
{
}

RTCRtpSFrameTransformErrorEvent::~RTCRtpSFrameTransformErrorEvent() = default;

EventInterface RTCRtpSFrameTransformErrorEvent::eventInterface() const
{
    return RTCRtpSFrameTransformErrorEventInterfaceType;
}

} // namespace WebCore

#endif // ENABLE(WEB_RTC)
