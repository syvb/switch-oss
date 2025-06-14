/*
 * Copyright (C) 2017-2020 Apple Inc. All rights reserved.
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
#include "TrackPrivateBase.h"

#if ENABLE(VIDEO)

#include "Logging.h"

namespace WebCore {

std::optional<uint64_t> TrackPrivateBase::trackUID() const
{
    return std::nullopt;
}

std::optional<bool> TrackPrivateBase::defaultEnabled() const
{
    return std::nullopt;
}

#if !RELEASE_LOG_DISABLED

#if !PLATFORM(WKC)
static uint64_t s_uniqueId = 0;
#else
static WKC_DEFINE_GLOBAL_TYPE_ZERO(uint64_t, s_uniqueId);
#endif

void TrackPrivateBase::setLogger(const Logger& logger, const void* logIdentifier)
{
    m_logger = &logger;
    m_logIdentifier = childLogIdentifier(logIdentifier, ++s_uniqueId);
}

WTFLogChannel& TrackPrivateBase::logChannel() const
{
    return LogMedia;
}
#endif

} // namespace WebCore

#endif
