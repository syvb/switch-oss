/*
 * WKCPrefs.cpp
 *
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
 */

#include "config.h"

#include "WKCPrefs.h"

#include "Settings.h"
#include "ResourceHandleManagerWKC.h"
#include "FocusController.h"
#include "FontPlatformData.h"
#include "MemoryCache.h"
#include "HTTPCacheWKC.h"
#include "NetworkStateNotifier.h"
#include "SpatialNavigation.h"
#include "DatabaseManager.h"
#include "Options.h"
#include "RenderFullScreen.h"

#include <wkc/wkcgpeer.h>
#include <wkc/wkcglpeer.h>
#include <wkc/wkcmediapeer.h>

namespace WebCore {
extern void setScreenDPI(int horizontaldpi, int verticaldpi);
extern void setScreenSizeWKC(const WebCore::IntSize& size);
extern void setAvailableScreenSize(const WebCore::IntSize& size);
extern void setScreenDepth(int depth, int depth_per_component);
extern void setIsMonochrome(bool monochrome);
typedef void (*ResolveFilenameForDisplayProc)(const unsigned short* path, const int path_len, unsigned short* out_path, int* out_path_len, const int path_maxlen);
extern void RenderTheme_SetResolveFilenameForDisplayProc(WebCore::ResolveFilenameForDisplayProc proc);
extern void SimpleFontData_SetAverageFontGlyph(const unsigned int in_glyph);
extern void FontPlatformData_enableScalingMonosizeFont(bool flag);
}

namespace WKC {
namespace WKCPrefs {

static wkcSkin*
gSkin()
{
    WKC_DEFINE_STATIC_TYPE(wkcSkin*, skin, 0);
    if (!skin) {
        skin = (wkcSkin *)WTF::fastMalloc(sizeof(wkcSkin));
        memset(skin, 0, sizeof(wkcSkin));
    }
    return skin;
}

void
setCacheCapacities(unsigned int min_dead_resource, unsigned int max_dead_resource, unsigned int total)
{
    unsigned int min_dead = min_dead_resource;
    unsigned int max_dead = max_dead_resource;

    if (min_dead > total) {
        min_dead = total;
    }
    if (max_dead > total) {
        max_dead = total;
    }

    WebCore::MemoryCache::singleton().setCapacities(min_dead, max_dead, total);
}

void
setDeadDecodedDataDeletionInterval(double interval)
{
    WTF::Seconds v(interval);
    WebCore::MemoryCache::singleton().setDeadDecodedDataDeletionInterval(v);
}

void 
setMinDelayBeforeLiveDecodedPruneCaches(double delay)
{
    WebCore::MemoryCache::setMinDelayBeforeLiveDecodedPrune(WTF::Seconds(delay));
}

void
setEnableMemoryCache(bool enable)
{
    if (!enable)
        WebCore::MemoryCache::singleton().setDisabled(TRUE);
}

void
setProxy(bool enable, const char* host, int port, bool isHTTP10, const char* proxyuser, const char* proxypass, const char* filters)
{
    WebCore::ResourceHandleManager::ProxyType proxy_type = WebCore::ResourceHandleManager::HTTP;

    using WebCore::ResourceHandleManager;
    if (WebCore::ResourceHandleManager* mgr = WebCore::ResourceHandleManager::sharedInstance()) {
        if (enable) {
            const char* user = "";
            const char* pass = "";
            if (proxyuser) user = proxyuser;
            if (proxypass) pass = proxypass;
            if (isHTTP10) proxy_type = WebCore::ResourceHandleManager::HTTP10;
            mgr->setProxyInfo(host, port, proxy_type, user, pass, WTF::String::fromUTF8(filters));
        } else {
            mgr->setProxyInfo("", 0, proxy_type, "", "", "");
        }
    }
}

void setSSLProtocols(bool isEnableTLS12, bool isEnableTLS13)
{
    if (WebCore::ResourceHandleManager::sharedInstance()) {
        WebCore::ResourceHandleManager* mgr = WebCore::ResourceHandleManager::sharedInstance();
        unsigned int ver = 0;
        if (isEnableTLS12)  ver |= (1 << 4);
        if (isEnableTLS13)  ver |= (1 << 5);
        mgr->SSLEnableProtocols(ver);
    }
}

void setSSLEnableOnlineCertChecks(bool isEnableOCSP, bool isEnableCRLDP)
{
    if (WebCore::ResourceHandleManager::sharedInstance()) {
        WebCore::ResourceHandleManager* mgr = WebCore::ResourceHandleManager::sharedInstance();
        mgr->SSLEnableOnlineCertChecks(isEnableOCSP, isEnableCRLDP);
    }
}

void
setMaxHTTPConnections(long num)
{
    using WebCore::ResourceHandleManager;
    if (WebCore::ResourceHandleManager* mgr = WebCore::ResourceHandleManager::sharedInstance()) {
        mgr->setMaxHTTPConnections(num);
    }
}

void
setMaxHTTPPipelineRequests(long num)
{
    using WebCore::ResourceHandleManager;
    if (WebCore::ResourceHandleManager* mgr = WebCore::ResourceHandleManager::sharedInstance()) {
        mgr->setMaxHTTPPipelineRequests(num);
    }
}

void
setMaxWebSocketConnections(long number)
{
    using WebCore::ResourceHandleManager;
    if (WebCore::ResourceHandleManager* mgr = WebCore::ResourceHandleManager::sharedInstance()) {
        mgr->setMaxWebSocketConnections(number);
    }
}

void
setMaxCookieEntries(long number)
{
    using WebCore::ResourceHandleManager;
    if (WebCore::ResourceHandleManager* mgr = WebCore::ResourceHandleManager::sharedInstance()) {
        mgr->setMaxCookieEntries(number);
    }
}

void
setDNSCacheTimeout(int sec)
{
    using WebCore::ResourceHandleManager;
    if (WebCore::ResourceHandleManager* mgr = WebCore::ResourceHandleManager::sharedInstance()) {
        mgr->setDNSCacheTimeout(sec);
    }
}

void
setServerResponseTimeout(int sec)
{
    using WebCore::ResourceHandleManager;
    if (WebCore::ResourceHandleManager* mgr = WebCore::ResourceHandleManager::sharedInstance()) {
        mgr->setServerResponseTimeout(sec);
    }
}

void
setConnectTimeout(int sec)
{
    using WebCore::ResourceHandleManager;
    if (WebCore::ResourceHandleManager* mgr = WebCore::ResourceHandleManager::sharedInstance()) {
        mgr->setConnectTimeout(sec);
    }
}

void
setAcceptEncoding(const char* encodings)
{
    using WebCore::ResourceHandleManager;
    if (WebCore::ResourceHandleManager* mgr = WebCore::ResourceHandleManager::sharedInstance()) {
        mgr->setAcceptEncoding(encodings);
    }
}

void
setHarmfulSiteFilter(bool enable)
{
    using WebCore::ResourceHandleManager;
    if (WebCore::ResourceHandleManager* mgr = WebCore::ResourceHandleManager::sharedInstance()) {
        mgr->setHarmfulSiteFilter(enable);
    }
}

void
setDoNotTrack(bool enable)
{
    using WebCore::ResourceHandleManager;
    if (WebCore::ResourceHandleManager* mgr = WebCore::ResourceHandleManager::sharedInstance()) {
        mgr->setDoNotTrack(enable);
    }
}

void
setRedirectInWebKit(bool enable)
{
    using WebCore::ResourceHandleManager;
    if (WebCore::ResourceHandleManager* mgr = WebCore::ResourceHandleManager::sharedInstance()) {
        mgr->setRedirectInWKC(enable);
    }
}

void
setUseHTTP2(bool enable)
{
    using WebCore::ResourceHandleManager;
    if (WebCore::ResourceHandleManager* mgr = WebCore::ResourceHandleManager::sharedInstance()) {
        mgr->setUseHTTP2(enable);
    }
}

void
setStackSize(unsigned int in_stack_size)
{
    void* thread;

    // JavaScriptCore requires stack size is more than 256KB.
    // See: Interpreter::StackPolicy::StackPolicy()
    ASSERT(in_stack_size > 256*1024);

    thread = wkcThreadCurrentThreadPeer();
#ifdef __clang__
    ASSERT(thread);
#endif
    wkcThreadSetStackSizePeer(thread, in_stack_size);
}

void
setScreenDeviceParams(const ScreenDeviceParams& params)
{
    WebCore::setScreenSizeWKC(WebCore::IntSize(params.fScreenWidth, params.fScreenHeight));
    WebCore::setAvailableScreenSize(WebCore::IntSize(params.fAvailableScreenWidth, params.fAvailableScreenHeight));
    WebCore::setScreenDepth(params.fScreenDepth, params.fScreenDepthPerComponent);
    WebCore::setIsMonochrome(params.fIsMonochrome);
}

void
setSystemStrings(const WKC::SystemStrings* strings)
{
    wkcSystemSetNavigatorPlatformPeer(reinterpret_cast<const UChar*>(strings->fNavigatorPlatform));
    wkcSystemSetNavigatorProductPeer(reinterpret_cast<const UChar*>(strings->fNavigatorProduct));
    wkcSystemSetNavigatorProductSubPeer(reinterpret_cast<const UChar*>(strings->fNavigatorProductSub));
    wkcSystemSetNavigatorVendorPeer(reinterpret_cast<const UChar*>(strings->fNavigatorVendor));
    wkcSystemSetNavigatorVendorSubPeer(reinterpret_cast<const UChar*>(strings->fNavigatorVendorSub));
    wkcSystemSetLanguagePeer(reinterpret_cast<const UChar*>(strings->fLanguage));
    wkcSystemSetButtonLabelSubmitPeer(reinterpret_cast<const UChar*>(strings->fButtonLabelSubmit));
    wkcSystemSetButtonLabelResetPeer(reinterpret_cast<const UChar*>(strings->fButtonLabelReset));
    wkcSystemSetButtonLabelFilePeer(reinterpret_cast<const UChar*>(strings->fButtonLabelFile));
}

bool
setSystemString(const unsigned char* key, const unsigned char* text)
{
    if (!key || !text)
        return false;
    return wkcSystemSetSystemStringPeer(key, text);
}

void 
setThreadInfo(void* threadID, void* stackBase)
{
    wkcThreadSetMainThreadInfoPeer(threadID, stackBase);
}

bool
setPrimaryFont(int fontID)
{
    return wkcFontEngineSetPrimaryFontPeer(fontID);
}

void
setEnableScalingMonosizeFont(bool flag)
{
    WebCore::FontPlatformData_enableScalingMonosizeFont(flag);
}

void 
setResolveFilenameForDisplayProc(WKC::ResolveFilenameForDisplayProc proc)
{
    WebCore::ResolveFilenameForDisplayProc iproc = (WebCore::ResolveFilenameForDisplayProc)proc;
    WebCore::RenderTheme_SetResolveFilenameForDisplayProc(iproc);
}

void
setGlyphForAverageWidth(const unsigned int glyph)
{
    WebCore::SimpleFontData_SetAverageFontGlyph(glyph);
}

void
setEmojiGlyphsFolderPath(const char* path)
{
    wkcDrawContextSetEmojiGlyphsFolderPeer(path);
}

void
registerSkin(const WKC::WKCSkin* skin)
{
    int i=0, j=0;

    if (gSkin()->fDefaultStyleSheet) {
        fastFree((void *)gSkin()->fDefaultStyleSheet);
        gSkin()->fDefaultStyleSheet = 0;
    }
    if (gSkin()->fQuirksStyleSheet) {
        fastFree((void *)gSkin()->fQuirksStyleSheet);
        gSkin()->fQuirksStyleSheet = 0;
    }
    for (i=0; i<WKC::ESystemFontTypes; i++) {
        if (gSkin()->fSystemFontFamilyName[i]) {
            fastFree((void *)gSkin()->fSystemFontFamilyName[i]);
            gSkin()->fSystemFontFamilyName[i] = 0;
        }
    }
    if (gSkin()->fResourceImages) {
        i=0;
        while (gSkin()->fResourceImages[i]) {
            fastFree((void *)gSkin()->fResourceImages[i]);
            i++;
        }
        fastFree((void *)gSkin()->fResourceImages);
        gSkin()->fResourceImages = 0;
    }

    if (!skin) {
        wkcStockImageRegisterSkinPeer((const wkcSkin *)0);
        return;
    }

    // for safety: WKC::WKCSkin and wkcSkin would be same structure,
    // but it should be safe transfer each members one by one...
    for (i=0; i<WKC::ESkinImages; i++) {
        gSkin()->fImages[i].fBitmap = skin->fImages[i].fBitmap;
        gSkin()->fImages[i].fSize.fWidth = skin->fImages[i].fSize.fWidth;
        gSkin()->fImages[i].fSize.fHeight = skin->fImages[i].fSize.fHeight;
        gSkin()->fDarkImages[i].fBitmap = skin->fDarkImages[i].fBitmap;
        gSkin()->fDarkImages[i].fSize.fWidth = skin->fDarkImages[i].fSize.fWidth;
        gSkin()->fDarkImages[i].fSize.fHeight = skin->fDarkImages[i].fSize.fHeight;
        for (j=0; j<4; j++) {
            gSkin()->fImages[i].fPoints[j] = skin->fImages[i].fPoints[j];
            gSkin()->fDarkImages[i].fPoints[j] = skin->fDarkImages[i].fPoints[j];
        }
    }
    gSkin()->fImageScale = skin->fImageScale;
    for (i=0; i<WKC::ESkinColors; i++) {
        gSkin()->fColors[i] = skin->fColors[i];
        gSkin()->fDarkColors[i] = skin->fDarkColors[i];
    }
    for (i=0; i<WKC::ESystemFontTypes; i++) {
        gSkin()->fSystemFontSize[i] = skin->fSystemFontSize[i];
        if (skin->fSystemFontFamilyName[i] && skin->fSystemFontFamilyName[i][0]) {
            gSkin()->fSystemFontFamilyName[i] = fastStrDup(skin->fSystemFontFamilyName[i]);
        }
    }
    if (skin->fDefaultStyleSheet && skin->fDefaultStyleSheet[0]) {
        gSkin()->fDefaultStyleSheet = fastStrDup(skin->fDefaultStyleSheet);
    }
    if (skin->fQuirksStyleSheet && skin->fQuirksStyleSheet[0]) {
        gSkin()->fQuirksStyleSheet = fastStrDup(skin->fQuirksStyleSheet);
    }
    if (skin->fResourceImages) {
        i=0;
        while (skin->fResourceImages[i++]);
        gSkin()->fResourceImages = (wkcSkinResourceImage **)fastMalloc(sizeof(wkcSkinResourceImage*)*(i+1));
        i=0;
        while (skin->fResourceImages[i]) {
            gSkin()->fResourceImages[i] = (wkcSkinResourceImage *)fastMalloc(sizeof(wkcSkinResourceImage));
            gSkin()->fResourceImages[i]->fName = skin->fResourceImages[i]->fName;
            gSkin()->fResourceImages[i]->fSize.fWidth = skin->fResourceImages[i]->fSize.fWidth;
            gSkin()->fResourceImages[i]->fSize.fHeight = skin->fResourceImages[i]->fSize.fHeight;
            gSkin()->fResourceImages[i]->fBitmap = skin->fResourceImages[i]->fBitmap;
            i++;
        }
        gSkin()->fResourceImages[i] = 0;
    }
    wkcStockImageRegisterSkinPeer(gSkin());
}

void
registerMediaControlsStyleSheet(const char* css)
{
    wkcMediaPlayerControlsRegisterStyleSheetPeer(css);
}

void
registerMediaControlsJavaScript(const char* js)
{
    wkcMediaPlayerControlsRegisterScriptPeer(js);
}

void
setHTTPCache(bool enable, long long limitTotalContentsSize, long maxContentSize, long minContentSize, int limitEntries, int limitEntryInfoSize, const char *filePath)
{
#if ENABLE(WKC_HTTPCACHE)
    using WebCore::ResourceHandleManager;
    if (WebCore::ResourceHandleManager* mgr = WebCore::ResourceHandleManager::sharedInstance()) {
        mgr->httpCache()->setDisabled(!enable);
        mgr->httpCache()->setMaxContentSize(maxContentSize);
        mgr->httpCache()->setMinContentSize(minContentSize);
        mgr->httpCache()->setMaxContentEntries(limitEntries);
        mgr->httpCache()->setMaxContentEntryInfoSize(limitEntryInfoSize);
        // The setting for limiting size of cache must be done before the following.
        mgr->httpCache()->setFilePath(filePath); // This causes the FAT file to be read.
        mgr->httpCache()->setMaxTotalCacheSize(limitTotalContentsSize); // This may cause the FAT file to be written.
    }
#else
    (void)enable;
    (void)limitSize;
    (void)filePath;
#endif
}

void
setOnLine(bool online)
{
    WebCore::NetworkStateNotifier::singleton().setOnLine(online);
}

void
setEffectiveAngleForHorizontalFocusNavigation(unsigned int angle)
{
    WebCore::SpatialNavigation_SetEffectiveAngleForHorizontalNavigation(angle);
}

void
setEffectiveAngleForVerticalFocusNavigation(unsigned int angle)
{
    WebCore::SpatialNavigation_SetEffectiveAngleForVerticalNavigation(angle);
}

void
setEffectiveAngleForDiagonalFocusNavigation(unsigned int minAngle, unsigned int maxAngle)
{
    WebCore::SpatialNavigation_SetEffectiveAngleForDiagonalNavigation(minAngle, maxAngle);
}

void
setAllowsMovingFocusToCoveredElement(bool allow)
{
    WebCore::FocusController::setAllowsMovingFocusToCoveredElement(allow);
}

void
setDatabasesDirectory(const char* path)
{
    WebCore::DatabaseManager::singleton().initialize(path);
}

void
setUseNearestFilter(bool flag)
{
    wkcDrawContexCairoSetUseFilterNearestPeer(flag);
}

void
activateWebGL()
{
#if ENABLE(WEBGL)
    bool ret = wkcGLActivatePeer();
#endif
}

void
setProhibitsScrollingEnabled(bool enabled)
{
    WebCore::Element::setProhibitsScrollingEnabled(enabled);
}

void
setUseDollarVM(bool enabled)
{
    JSC::Options::useDollarVM() = enabled;
}

void
setFullscreenBackgroundColor(wkc_uint32 rgba)
{
    WebCore::RenderFullScreen::setBackgroundColor(WebCore::asSRGBA(WebCore::PackedColor::RGBA{ rgba }));
}

void
forceTerminate()
{
    wkcStockImageRegisterSkinPeer(0);
    wkcMediaPlayerControlsRegisterStyleSheetPeer(0);
    wkcMediaPlayerControlsRegisterScriptPeer(0);
}

void
initialize()
{
    gSkin()->fDefaultStyleSheet = 0;
    gSkin()->fQuirksStyleSheet = 0;
    for (int i=0; i<WKC::ESystemFontTypes; i++) {
        gSkin()->fSystemFontFamilyName[i] = 0;
    }
}

void
finalize()
{
    if (gSkin()->fDefaultStyleSheet) {
        fastFree((void *)gSkin()->fDefaultStyleSheet);
    }
    if (gSkin()->fQuirksStyleSheet) {
        fastFree((void *)gSkin()->fQuirksStyleSheet);
    }
    for (int i=0; i<WKC::ESystemFontTypes; i++) {
        if (gSkin()->fSystemFontFamilyName[i]) {
            fastFree((void *)gSkin()->fSystemFontFamilyName[i]);
        }
    }

    if (gSkin()->fResourceImages) {
        int i=0;
        while (gSkin()->fResourceImages[i]) {
            fastFree((void *)gSkin()->fResourceImages[i]);
            i++;
        }
        fastFree((void *)gSkin()->fResourceImages);
        gSkin()->fResourceImages = 0;
    }
}

} // namespace
} // namespace
