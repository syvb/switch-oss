/*
 * Copyright (C) 2006-2021 Apple Inc. All rights reserved.
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
#include "DeprecatedGlobalSettings.h"

#include "AudioSession.h"
#include "HTMLMediaElement.h"
#include <wtf/NeverDestroyed.h>

namespace WebCore {

#if PLATFORM(WKC)
WKC_DEFINE_GLOBAL_CLASS_OBJ_ZERO(bool, DeprecatedGlobalSettings, gMockScrollbarsEnabled);
WKC_DEFINE_GLOBAL_CLASS_OBJ_ZERO(bool, DeprecatedGlobalSettings, gUsesOverlayScrollbars);
WKC_DEFINE_GLOBAL_CLASS_OBJ_ZERO(bool, DeprecatedGlobalSettings, gMockScrollAnimatorEnabled);

#if ENABLE(MEDIA_STREAM)
WKC_DEFINE_GLOBAL_CLASS_OBJ_ZERO(bool, DeprecatedGlobalSettings, gMockCaptureDevicesEnabled);
WKC_DEFINE_GLOBAL_CLASS_OBJ(bool, DeprecatedGlobalSettings, gMediaCaptureRequiresSecureConnection, true);
#endif

WKC_DEFINE_GLOBAL_CLASS_OBJ_ZERO(bool, DeprecatedGlobalSettings, gShouldRespectPriorityInCSSAttributeSetters);
WKC_DEFINE_GLOBAL_CLASS_OBJ_ZERO(bool, DeprecatedGlobalSettings, gLowPowerVideoAudioBufferSizeEnabled);
WKC_DEFINE_GLOBAL_CLASS_OBJ_ZERO(bool, DeprecatedGlobalSettings, gResourceLoadStatisticsEnabledEnabled);
WKC_DEFINE_GLOBAL_CLASS_OBJ_ZERO(bool, DeprecatedGlobalSettings, gAllowsAnySSLCertificate);

WKC_DEFINE_GLOBAL_CLASS_OBJ_ZERO(bool, DeprecatedGlobalSettings, gManageAudioSession);
#else
#if USE(AVFOUNDATION)
bool DeprecatedGlobalSettings::gAVFoundationEnabled = true;
bool DeprecatedGlobalSettings::gAVFoundationNSURLSessionEnabled = true;
#endif

#if USE(GSTREAMER)
bool DeprecatedGlobalSettings::gGStreamerEnabled = true;
#endif

bool DeprecatedGlobalSettings::gMockScrollbarsEnabled = false;
bool DeprecatedGlobalSettings::gUsesOverlayScrollbars = false;

#if PLATFORM(WIN)
bool DeprecatedGlobalSettings::gShouldUseHighResolutionTimers = true;
#endif
    
bool DeprecatedGlobalSettings::gLowPowerVideoAudioBufferSizeEnabled = false;
bool DeprecatedGlobalSettings::gResourceLoadStatisticsEnabledEnabled = false;
bool DeprecatedGlobalSettings::gAllowsAnySSLCertificate = false;

#if PLATFORM(IOS_FAMILY)
bool DeprecatedGlobalSettings::gNetworkDataUsageTrackingEnabled = false;
bool DeprecatedGlobalSettings::gShouldOptOutOfNetworkStateObservation = false;
bool DeprecatedGlobalSettings::gDisableScreenSizeOverride = false;
#endif

#if PLATFORM(WIN)
void DeprecatedGlobalSettings::setShouldUseHighResolutionTimers(bool shouldUseHighResolutionTimers)
{
    gShouldUseHighResolutionTimers = shouldUseHighResolutionTimers;
}
#endif

#if USE(AVFOUNDATION)
void DeprecatedGlobalSettings::setAVFoundationEnabled(bool enabled)
{
    if (gAVFoundationEnabled == enabled)
        return;

    gAVFoundationEnabled = enabled;
    HTMLMediaElement::resetMediaEngines();
}

void DeprecatedGlobalSettings::setAVFoundationNSURLSessionEnabled(bool enabled)
{
    if (gAVFoundationNSURLSessionEnabled == enabled)
        return;

    gAVFoundationNSURLSessionEnabled = enabled;
}
#endif

#if USE(GSTREAMER)
void DeprecatedGlobalSettings::setGStreamerEnabled(bool enabled)
{
    if (gGStreamerEnabled == enabled)
        return;

    gGStreamerEnabled = enabled;

#if ENABLE(VIDEO)
    HTMLMediaElement::resetMediaEngines();
#endif
}
#endif
#endif

// It's very important that this setting doesn't change in the middle of a document's lifetime.
// The Mac port uses this flag when registering and deregistering platform-dependent scrollbar
// objects. Therefore, if this changes at an unexpected time, deregistration may not happen
// correctly, which may cause the platform to follow dangling pointers.
void DeprecatedGlobalSettings::setMockScrollbarsEnabled(bool flag)
{
    gMockScrollbarsEnabled = flag;
    // FIXME: This should update scroll bars in existing pages.
}

bool DeprecatedGlobalSettings::mockScrollbarsEnabled()
{
    return gMockScrollbarsEnabled;
}

void DeprecatedGlobalSettings::setUsesOverlayScrollbars(bool flag)
{
    gUsesOverlayScrollbars = flag;
    // FIXME: This should update scroll bars in existing pages.
}

bool DeprecatedGlobalSettings::usesOverlayScrollbars()
{
    return gUsesOverlayScrollbars;
}

void DeprecatedGlobalSettings::setLowPowerVideoAudioBufferSizeEnabled(bool flag)
{
    gLowPowerVideoAudioBufferSizeEnabled = flag;
}

void DeprecatedGlobalSettings::setResourceLoadStatisticsEnabled(bool flag)
{
    gResourceLoadStatisticsEnabledEnabled = flag;
}

#if PLATFORM(IOS_FAMILY)
void DeprecatedGlobalSettings::setAudioSessionCategoryOverride(unsigned sessionCategory)
{
    AudioSession::sharedSession().setCategoryOverride(static_cast<AudioSession::CategoryType>(sessionCategory));
}

unsigned DeprecatedGlobalSettings::audioSessionCategoryOverride()
{
    return static_cast<unsigned>(AudioSession::sharedSession().categoryOverride());
}

void DeprecatedGlobalSettings::setNetworkDataUsageTrackingEnabled(bool trackingEnabled)
{
    gNetworkDataUsageTrackingEnabled = trackingEnabled;
}

bool DeprecatedGlobalSettings::networkDataUsageTrackingEnabled()
{
    return gNetworkDataUsageTrackingEnabled;
}

static String& sharedNetworkInterfaceNameGlobal()
{
    static NeverDestroyed<String> networkInterfaceName;
#if PLATFORM(WKC)
    if (networkInterfaceName.isNull())
        networkInterfaceName.construct();
#endif
    return networkInterfaceName;
}

void DeprecatedGlobalSettings::setNetworkInterfaceName(const String& networkInterfaceName)
{
    sharedNetworkInterfaceNameGlobal() = networkInterfaceName;
}

const String& DeprecatedGlobalSettings::networkInterfaceName()
{
    return sharedNetworkInterfaceNameGlobal();
}
#endif

#if USE(AUDIO_SESSION)
void DeprecatedGlobalSettings::setShouldManageAudioSessionCategory(bool flag)
{
    AudioSession::setShouldManageAudioSessionCategory(flag);
}

bool DeprecatedGlobalSettings::shouldManageAudioSessionCategory()
{
    return AudioSession::shouldManageAudioSessionCategory();
}
#endif

void DeprecatedGlobalSettings::setAllowsAnySSLCertificate(bool allowAnySSLCertificate)
{
    gAllowsAnySSLCertificate = allowAnySSLCertificate;
}

bool DeprecatedGlobalSettings::allowsAnySSLCertificate()
{
    return gAllowsAnySSLCertificate;
}

} // namespace WebCore
