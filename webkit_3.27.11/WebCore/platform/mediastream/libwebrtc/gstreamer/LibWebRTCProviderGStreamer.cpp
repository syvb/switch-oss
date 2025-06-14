/*
 * Copyright (C) 2017 Igalia S.L. All rights reserved.
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

#include "config.h"
#include "LibWebRTCProviderGStreamer.h"

#if USE(LIBWEBRTC) && USE(GSTREAMER)

#include "RuntimeEnabledFeatures.h"
#include <wtf/UniqueRef.h>

#if PLATFORM(WKC)
#include "GenericTypedArrayViewInlines.h"
#include "JSGenericTypedArrayView.h"
#endif

namespace WebCore {

UniqueRef<LibWebRTCProvider> LibWebRTCProvider::create()
{
    return makeUniqueRef<LibWebRTCProviderGStreamer>();
}

bool LibWebRTCProvider::webRTCAvailable()
{
    return true;
}

std::unique_ptr<webrtc::VideoDecoderFactory> LibWebRTCProviderGStreamer::createDecoderFactory()
{
    return makeUnique<GStreamerVideoDecoderFactory>(isSupportingVP9Profile0(), isSupportingVP9Profile2());
}

std::unique_ptr<webrtc::VideoEncoderFactory> LibWebRTCProviderGStreamer::createEncoderFactory()
{
    return makeUnique<GStreamerVideoEncoderFactory>(isSupportingVP9Profile0(), isSupportingVP9Profile2());
}

} // namespace WebCore

#endif // USE(LIBWEBRTC) && USE(GSTREAMER)
