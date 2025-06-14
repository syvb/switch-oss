/*
 * Copyright (C) 2017-2021 Apple Inc. All rights reserved.
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

#pragma once

#if ENABLE(MEDIA_STREAM) && PLATFORM(COCOA)

#include "CaptureDevice.h"
#include "RealtimeMediaSource.h"
#include <wtf/Observer.h>
#include <wtf/RefCounted.h>
#include <wtf/RefPtr.h>
#include <wtf/RunLoop.h>
#include <wtf/UniqueRef.h>
#include <wtf/text/WTFString.h>


typedef struct CGImage *CGImageRef;
typedef struct __CVBuffer *CVPixelBufferRef;
typedef struct __IOSurface *IOSurfaceRef;

namespace WTF {
class MediaTime;
}

namespace WebCore {

class CaptureDeviceInfo;
class ImageTransferSessionVT;
class PixelBufferConformerCV;

class CapturerObserver : public CanMakeWeakPtr<CapturerObserver> {
public:
    virtual ~CapturerObserver() = default;

    virtual void capturerIsRunningChanged(bool) { }
    virtual void capturerFailed() { };
    virtual void capturerConfigurationChanged() { };
};

class DisplayCaptureSourceCocoa final
    : public RealtimeMediaSource
    , public CapturerObserver
    , public ThreadSafeRefCountedAndCanMakeThreadSafeWeakPtr<DisplayCaptureSourceCocoa, WTF::DestructionThread::MainRunLoop> {
public:
    using DisplayFrameType = std::variant<RefPtr<NativeImage>, RetainPtr<IOSurfaceRef>, RetainPtr<CMSampleBufferRef>>;

    class Capturer : public LoggerHelper {
        WTF_MAKE_FAST_ALLOCATED;
    public:
        virtual ~Capturer() = default;

        virtual bool start() = 0;
        virtual void stop() = 0;
        virtual DisplayFrameType generateFrame() = 0;
        virtual CaptureDevice::DeviceType deviceType() const = 0;
        virtual RealtimeMediaSourceSettings::DisplaySurfaceType surfaceType() const = 0;
        virtual void commitConfiguration(const RealtimeMediaSourceSettings&) = 0;
        virtual IntSize intrinsicSize() const = 0;

        virtual void setLogger(const Logger&, const void*);
        const Logger* loggerPtr() const { return m_logger.get(); }
        const Logger& logger() const final { ASSERT(m_logger); return *m_logger.get(); }
        const void* logIdentifier() const final { return m_logIdentifier; }
        WTFLogChannel& logChannel() const final;

        using IsRunningObserver = WTF::Observer<void(bool)>;
        void setIsRunningObserver(IsRunningObserver* observer) { m_observer = observer; };

    protected:
        Capturer() = default;
        void capturerIsRunningChanged(bool);

    private:
        WeakPtr<IsRunningObserver> m_observer;
        RefPtr<const Logger> m_logger;
        const void* m_logIdentifier;
    };

    static CaptureSourceOrError create(const CaptureDevice&, String&&, const MediaConstraints*);
    static CaptureSourceOrError create(Expected<UniqueRef<Capturer>, String>&&, const CaptureDevice&, String&&, const MediaConstraints*);

    Seconds elapsedTime();
    void updateFrameSize();

    void ref() const final { ThreadSafeRefCountedAndCanMakeThreadSafeWeakPtr<DisplayCaptureSourceCocoa, WTF::DestructionThread::MainRunLoop>::ref(); }
    void deref() const final { ThreadSafeRefCountedAndCanMakeThreadSafeWeakPtr<DisplayCaptureSourceCocoa, WTF::DestructionThread::MainRunLoop>::deref(); }
    ThreadSafeWeakPtrControlBlock& controlBlock() const final { return ThreadSafeRefCountedAndCanMakeThreadSafeWeakPtr<DisplayCaptureSourceCocoa, WTF::DestructionThread::MainRunLoop>::controlBlock(); }
    virtual ~DisplayCaptureSourceCocoa();

private:
    DisplayCaptureSourceCocoa(UniqueRef<Capturer>&&, const CaptureDevice&, MediaDeviceHashSalts&&, PageIdentifier);

    // RealtimeMediaSource
    void startProducingData() final;
    void stopProducingData() final;
    void settingsDidChange(OptionSet<RealtimeMediaSourceSettings::Flag>) final;
    bool isCaptureSource() const final { return true; }
    const RealtimeMediaSourceCapabilities& capabilities() final;
    const RealtimeMediaSourceSettings& settings() final;
    CaptureDevice::DeviceType deviceType() const { return m_capturer->deviceType(); }
    void commitConfiguration() final { m_capturer->commitConfiguration(settings()); }

    const char* logClassName() const final { return "DisplayCaptureSourceCocoa"; }
    void setLogger(const Logger&, const void*) final;

    void emitFrame();

    UniqueRef<Capturer> m_capturer;
    std::optional<RealtimeMediaSourceCapabilities> m_capabilities;
    std::optional<RealtimeMediaSourceSettings> m_currentSettings;
    RealtimeMediaSourceSupportedConstraints m_supportedConstraints;

    MonotonicTime m_startTime { MonotonicTime::nan() };
    Seconds m_elapsedTime { 0_s };

    RunLoop::Timer<DisplayCaptureSourceCocoa> m_timer;

    std::unique_ptr<ImageTransferSessionVT> m_imageTransferSession;
    WTF::Observer<void(bool)> m_capturerIsRunningObserver;
};

} // namespace WebCore

#endif // ENABLE(MEDIA_STREAM) && PLATFORM(COCOA)
