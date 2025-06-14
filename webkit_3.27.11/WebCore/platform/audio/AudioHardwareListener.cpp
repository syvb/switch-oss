/*
 * Copyright (C) 2014 Apple Inc. All rights reserved.
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
#include "AudioHardwareListener.h"

#include <wtf/Function.h>
#include <wtf/NeverDestroyed.h>

#if PLATFORM(MAC)
#include "AudioHardwareListenerMac.h"
#endif

namespace WebCore {

static AudioHardwareListener::CreationFunction& audioHardwareListenerCreationFunction()
{
    static NeverDestroyed<AudioHardwareListener::CreationFunction> creationFunction;
#if PLATFORM(WKC)
    if (creationFunction.isNull())
        creationFunction.construct();
#endif
    return creationFunction;
}

void AudioHardwareListener::setCreationFunction(CreationFunction&& function)
{
    audioHardwareListenerCreationFunction() = WTFMove(function);
}

void AudioHardwareListener::resetCreationFunction()
{
    audioHardwareListenerCreationFunction() = [] (AudioHardwareListener::Client& client) {
#if PLATFORM(MAC)
        return AudioHardwareListenerMac::create(client);
#else
        class RefCountedAudioHardwareListener : public AudioHardwareListener, public RefCounted<RefCountedAudioHardwareListener> {
        public:
            RefCountedAudioHardwareListener(AudioHardwareListener::Client& client)
                : AudioHardwareListener(client) { }
            void ref() const final { RefCounted<RefCountedAudioHardwareListener>::ref(); }
            void deref() const final { RefCounted<RefCountedAudioHardwareListener>::deref(); }
        };
        return adoptRef(*new RefCountedAudioHardwareListener(client));
#endif
    };
}

Ref<AudioHardwareListener> AudioHardwareListener::create(Client& client)
{
    if (!audioHardwareListenerCreationFunction())
        resetCreationFunction();

    return audioHardwareListenerCreationFunction()(client);
}

AudioHardwareListener::AudioHardwareListener(Client& client)
    : m_client(client)
{
#if PLATFORM(IOS_FAMILY)
    m_supportedBufferSizes = { 32, 4096 };
#endif
}

}
