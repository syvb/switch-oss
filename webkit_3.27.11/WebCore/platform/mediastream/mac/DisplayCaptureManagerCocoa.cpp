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

#include "config.h"
#include "DisplayCaptureManagerCocoa.h"

#if ENABLE(MEDIA_STREAM)

#include "Logging.h"
#include <wtf/Algorithms.h>
#include <wtf/NeverDestroyed.h>

#if PLATFORM(MAC)
#include "CGDisplayStreamScreenCaptureSource.h"
#endif

#if PLATFORM(IOS)
#include "ReplayKitCaptureSource.h"
#endif

namespace WebCore {

DisplayCaptureManagerCocoa& DisplayCaptureManagerCocoa::singleton()
{
    static NeverDestroyed<DisplayCaptureManagerCocoa> manager;
#if PLATFORM(WKC)
    if (manager.isNull())
        manager.construct();
#endif
    return manager.get();
}

const Vector<CaptureDevice>& DisplayCaptureManagerCocoa::captureDevices()
{
    m_devices.clear();

    updateDisplayCaptureDevices();
    updateWindowCaptureDevices();

    return m_devices;
}

void DisplayCaptureManagerCocoa::updateDisplayCaptureDevices()
{
#if PLATFORM(MAC)
    CGDisplayStreamScreenCaptureSource::screenCaptureDevices(m_devices);
#elif PLATFORM(IOS)
    ReplayKitCaptureSource::screenCaptureDevices(m_devices);
#endif
}

void DisplayCaptureManagerCocoa::updateWindowCaptureDevices()
{
}

std::optional<CaptureDevice> DisplayCaptureManagerCocoa::screenCaptureDeviceWithPersistentID(const String& deviceID)
{
#if PLATFORM(MAC)
    return CGDisplayStreamScreenCaptureSource::screenCaptureDeviceWithPersistentID(deviceID);
#else
    UNUSED_PARAM(deviceID);
    return std::nullopt;
#endif
}

std::optional<CaptureDevice> DisplayCaptureManagerCocoa::windowCaptureDeviceWithPersistentID(const String& deviceID)
{
    UNUSED_PARAM(deviceID);
    return std::nullopt;
}

std::optional<CaptureDevice> DisplayCaptureManagerCocoa::captureDeviceWithPersistentID(CaptureDevice::DeviceType type, const String& id)
{
    switch (type) {
    case CaptureDevice::DeviceType::Screen:
        return screenCaptureDeviceWithPersistentID(id);
        break;

    case CaptureDevice::DeviceType::Window:
        return windowCaptureDeviceWithPersistentID(id);
        break;

    case CaptureDevice::DeviceType::Camera:
    case CaptureDevice::DeviceType::Microphone:
    case CaptureDevice::DeviceType::Speaker:
    case CaptureDevice::DeviceType::Unknown:
        ASSERT_NOT_REACHED();
        break;
    }

    return std::nullopt;
}

} // namespace WebCore

#endif // ENABLE(MEDIA_STREAM)
