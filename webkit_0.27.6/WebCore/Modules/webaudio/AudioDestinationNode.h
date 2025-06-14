/*
 * Copyright (C) 2010, Google Inc. All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 * 1.  Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 * 2.  Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
 *
 * THIS SOFTWARE IS PROVIDED BY APPLE INC. AND ITS CONTRIBUTORS ``AS IS'' AND ANY
 * EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED
 * WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE
 * DISCLAIMED. IN NO EVENT SHALL APPLE INC. OR ITS CONTRIBUTORS BE LIABLE FOR ANY
 * DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES
 * (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES;
 * LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON
 * ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
 * (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS
 * SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */

#ifndef AudioDestinationNode_h
#define AudioDestinationNode_h

#include "AudioBuffer.h"
#include "AudioBus.h"
#include "AudioIOCallback.h"
#include "AudioNode.h"
#include "AudioSourceProvider.h"

namespace WebCore {

class AudioBus;
class AudioContext;

class AudioDestinationNode : public AudioNode, public AudioIOCallback {
public:
    AudioDestinationNode(AudioContext*, float sampleRate);
    virtual ~AudioDestinationNode();
    
    // AudioNode   
    virtual void process(size_t) override { }; // we're pulled by hardware so this is never called
    virtual void reset() override { m_currentSampleFrame = 0; }
    
    // The audio hardware calls render() to get the next render quantum of audio into destinationBus.
    // It will optionally give us local/live audio input in sourceBus (if it's not 0).
    virtual void render(AudioBus* sourceBus, AudioBus* destinationBus, size_t numberOfFrames) override;

    size_t currentSampleFrame() const { return m_currentSampleFrame; }
    double currentTime() const { return currentSampleFrame() / static_cast<double>(sampleRate()); }

    virtual unsigned long maxChannelCount() const { return 0; }

    // Enable local/live input for the specified device.
    virtual void enableInput(const String& inputDeviceId) = 0;

    virtual void startRendering() = 0;
#if PLATFORM(WKC)
    virtual void resume(WTF::Function<void()>) { }
    virtual void suspend(WTF::Function<void()>) { }
    virtual void close(WTF::Function<void()>) { }
#else
    virtual void resume(std::function<void()>) { }
    virtual void suspend(std::function<void()>) { }
    virtual void close(std::function<void()>) { }
#endif

    AudioSourceProvider* localAudioInputProvider() { return &m_localAudioInputProvider; }

    virtual bool isPlaying() { return false; }
    virtual void isPlayingDidChange() override;
    bool isPlayingAudio() const { return m_isEffectivelyPlayingAudio; }
    void setMuted(bool muted) { m_muted = muted; }

protected:
    // LocalAudioInputProvider allows us to expose an AudioSourceProvider for local/live audio input.
    // If there is local/live audio input, we call set() with the audio input data every render quantum.
    class LocalAudioInputProvider : public AudioSourceProvider {
    public:
        LocalAudioInputProvider()
            : m_sourceBus(AudioBus::create(2, AudioNode::ProcessingSizeInFrames)) // FIXME: handle non-stereo local input.
        {
        }

        void set(AudioBus* bus)
        {
            if (bus)
                m_sourceBus->copyFrom(*bus);
        }

        // AudioSourceProvider.
        virtual void provideInput(AudioBus* destinationBus, size_t numberOfFrames) override
        {
            bool isGood = destinationBus && destinationBus->length() == numberOfFrames && m_sourceBus->length() == numberOfFrames;
            ASSERT(isGood);
            if (isGood)
                destinationBus->copyFrom(*m_sourceBus);
        }

    private:
        RefPtr<AudioBus> m_sourceBus;
    };

    virtual double tailTime() const override { return 0; }
    virtual double latencyTime() const override { return 0; }

    void setIsSilent(bool);
    void updateIsEffectivelyPlayingAudio();

    // Counts the number of sample-frames processed by the destination.
    size_t m_currentSampleFrame;

    LocalAudioInputProvider m_localAudioInputProvider;
    bool m_isSilent;
    bool m_isEffectivelyPlayingAudio;
    bool m_muted;
};

} // namespace WebCore

#endif // AudioDestinationNode_h
