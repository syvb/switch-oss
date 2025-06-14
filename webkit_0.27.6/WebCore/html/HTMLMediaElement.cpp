/*
 * Copyright (C) 2007-2018 Apple Inc. All rights reserved.
 * Copyright (c) 2015-2017 ACCESS CO., LTD. All rights reserved.
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
#if ENABLE(VIDEO)
#include "HTMLMediaElement.h"

#include "ApplicationCacheHost.h"
#include "ApplicationCacheResource.h"
#include "Attribute.h"
#include "CSSPropertyNames.h"
#include "CSSValueKeywords.h"
#include "ChromeClient.h"
#include "ClientRect.h"
#include "ClientRectList.h"
#include "ContentSecurityPolicy.h"
#include "ContentType.h"
#include "CookieJar.h"
#include "DiagnosticLoggingClient.h"
#include "DiagnosticLoggingKeys.h"
#include "DisplaySleepDisabler.h"
#include "DocumentLoader.h"
#include "ElementIterator.h"
#include "EventNames.h"
#include "ExceptionCodePlaceholder.h"
#include "FrameLoader.h"
#include "FrameLoaderClient.h"
#include "FrameView.h"
#include "HTMLSourceElement.h"
#include "HTMLVideoElement.h"
#include "JSHTMLMediaElement.h"
#include "Language.h"
#include "Logging.h"
#include "MIMETypeRegistry.h"
#include "MainFrame.h"
#include "MediaController.h"
#include "MediaControls.h"
#include "MediaDocument.h"
#include "MediaError.h"
#include "MediaFragmentURIParser.h"
#include "MediaKeyEvent.h"
#include "MediaList.h"
#include "MediaPlayer.h"
#include "MediaQueryEvaluator.h"
#include "MediaResourceLoader.h"
#include "MemoryPressureHandler.h"
#include "NetworkingContext.h"
#include "NoEventDispatchAssertion.h"
#include "PageGroup.h"
#include "PageThrottler.h"
#include "PlatformMediaSessionManager.h"
#include "ProgressTracker.h"
#include "RenderLayerCompositor.h"
#include "RenderVideo.h"
#include "RenderView.h"
#include "ResourceLoadInfo.h"
#include "ScriptController.h"
#include "ScriptSourceCode.h"
#include "SecurityPolicy.h"
#include "SessionID.h"
#include "Settings.h"
#include "ShadowRoot.h"
#include "TimeRanges.h"
#include "UserContentController.h"
#include <limits>
#include <runtime/Uint8Array.h>
#include <wtf/CurrentTime.h>
#include <wtf/MathExtras.h>
#include <wtf/Ref.h>
#include <wtf/text/CString.h>

#if ENABLE(VIDEO_TRACK)
#include "AudioTrackList.h"
#include "HTMLTrackElement.h"
#include "InbandGenericTextTrack.h"
#include "InbandTextTrackPrivate.h"
#include "InbandWebVTTTextTrack.h"
#include "RuntimeEnabledFeatures.h"
#include "TextTrackCueList.h"
#include "TextTrackList.h"
#include "VideoTrackList.h"
#endif

#if ENABLE(WEB_AUDIO)
#include "AudioSourceProvider.h"
#include "MediaElementAudioSourceNode.h"
#endif

#if PLATFORM(IOS)
#include "RuntimeApplicationChecksIOS.h"
#endif

#if ENABLE(WIRELESS_PLAYBACK_TARGET)
#include "WebKitPlaybackTargetAvailabilityEvent.h"
#endif

#if ENABLE(MEDIA_SESSION)
#include "MediaSession.h"
#endif

#if ENABLE(MEDIA_SOURCE)
#include "DOMWindow.h"
#include "MediaSource.h"
#include "Performance.h"
#include "VideoPlaybackQuality.h"
#endif

#if ENABLE(MEDIA_STREAM)
#include "MediaStream.h"
#include "MediaStreamRegistry.h"
#endif

#if ENABLE(ENCRYPTED_MEDIA_V2)
#include "MediaKeyNeededEvent.h"
#include "MediaKeys.h"
#endif

#if USE(PLATFORM_TEXT_TRACK_MENU)
#include "PlatformTextTrack.h"
#endif

#if ENABLE(MEDIA_CONTROLS_SCRIPT)
#include "JSMediaControlsHost.h"
#include "MediaControlsHost.h"
#include "ScriptGlobalObject.h"
#include <bindings/ScriptObject.h>
#endif

namespace WebCore {

static const double SeekRepeatDelay = 0.1;
static const double SeekTime = 0.2;
static const double ScanRepeatDelay = 1.5;
static const double ScanMaximumRate = 8;

static void setFlags(unsigned& value, unsigned flags)
{
    value |= flags;
}

static void clearFlags(unsigned& value, unsigned flags)
{
    value &= ~flags;
}
    
#if !LOG_DISABLED
static String urlForLoggingMedia(const URL& url)
{
    static const unsigned maximumURLLengthForLogging = 128;

    if (url.string().length() < maximumURLLengthForLogging)
        return url.string();
    return url.string().substring(0, maximumURLLengthForLogging) + "...";
}

static const char* boolString(bool val)
{
    return val ? "true" : "false";
}

static String actionName(HTMLMediaElementEnums::DelayedActionType action)
{
    StringBuilder actionBuilder;

#define CASE(_actionType) \
    if (action & (HTMLMediaElementEnums::_actionType)) { \
        if (!actionBuilder.isEmpty()) \
        actionBuilder.append(", "); \
        actionBuilder.append(#_actionType); \
    } \

    CASE(LoadMediaResource);
    CASE(ConfigureTextTracks);
    CASE(TextTrackChangesNotification);
    CASE(ConfigureTextTrackDisplay);
    CASE(CheckPlaybackTargetCompatablity);
    CASE(CheckMediaState);

    return actionBuilder.toString();

#undef CASE
}

#endif

#ifndef LOG_MEDIA_EVENTS
// Default to not logging events because so many are generated they can overwhelm the rest of 
// the logging.
#define LOG_MEDIA_EVENTS 0
#endif

#ifndef LOG_CACHED_TIME_WARNINGS
// Default to not logging warnings about excessive drift in the cached media time because it adds a
// fair amount of overhead and logging.
#define LOG_CACHED_TIME_WARNINGS 0
#endif

#if ENABLE(MEDIA_SOURCE)
// URL protocol used to signal that the media source API is being used.
static const char* mediaSourceBlobProtocol = "blob";
#endif

#if ENABLE(MEDIA_STREAM)
// URL protocol used to signal that the media stream API is being used.
static const char* mediaStreamBlobProtocol = "blob";
#endif

using namespace HTMLNames;

typedef HashMap<Document*, HashSet<HTMLMediaElement*>> DocumentElementSetMap;
static DocumentElementSetMap& documentToElementSetMap()
{
    DEPRECATED_DEFINE_STATIC_LOCAL(DocumentElementSetMap, map, ());
    return map;
}

static void addElementToDocumentMap(HTMLMediaElement& element, Document& document)
{
    DocumentElementSetMap& map = documentToElementSetMap();
    HashSet<HTMLMediaElement*> set = map.take(&document);
    set.add(&element);
    map.add(&document, set);
}

static void removeElementFromDocumentMap(HTMLMediaElement& element, Document& document)
{
    DocumentElementSetMap& map = documentToElementSetMap();
    HashSet<HTMLMediaElement*> set = map.take(&document);
    set.remove(&element);
    if (!set.isEmpty())
        map.add(&document, set);
}

#if ENABLE(ENCRYPTED_MEDIA)
static ExceptionCode exceptionCodeForMediaKeyException(MediaPlayer::MediaKeyException exception)
{
    switch (exception) {
    case MediaPlayer::NoError:
        return 0;
    case MediaPlayer::InvalidPlayerState:
        return INVALID_STATE_ERR;
    case MediaPlayer::KeySystemNotSupported:
        return NOT_SUPPORTED_ERR;
#if PLATFORM(WKC)
    case MediaPlayer::InvalidAccess:
        return INVALID_ACCESS_ERR;
#endif
    }

    ASSERT_NOT_REACHED();
    return INVALID_STATE_ERR;
}
#endif

#if ENABLE(VIDEO_TRACK)
class TrackDisplayUpdateScope {
#if PLATFORM(WKC)
    WTF_MAKE_FAST_ALLOCATED;
#endif
public:
    TrackDisplayUpdateScope(HTMLMediaElement* mediaElement)
    {
        m_mediaElement = mediaElement;
        m_mediaElement->beginIgnoringTrackDisplayUpdateRequests();
    }
    ~TrackDisplayUpdateScope()
    {
        ASSERT(m_mediaElement);
        m_mediaElement->endIgnoringTrackDisplayUpdateRequests();
    }
    
private:
    HTMLMediaElement* m_mediaElement;
};
#endif

struct HTMLMediaElement::TrackGroup {
    enum GroupKind { CaptionsAndSubtitles, Description, Chapter, Metadata, Other };

    TrackGroup(GroupKind kind)
        : visibleTrack(0)
        , defaultTrack(0)
        , kind(kind)
        , hasSrcLang(false)
    {
    }

    Vector<RefPtr<TextTrack>> tracks;
    RefPtr<TextTrack> visibleTrack;
    RefPtr<TextTrack> defaultTrack;
    GroupKind kind;
    bool hasSrcLang;
};

HashSet<HTMLMediaElement*>& HTMLMediaElement::allMediaElements()
{
#if !PLATFORM(WKC)
    static NeverDestroyed<HashSet<HTMLMediaElement*>> elements;
    return elements;
#else
    WKC_DEFINE_STATIC_PTR(HashSet<HTMLMediaElement*>*, elements, 0);
    if (!elements)
        elements = new HashSet<HTMLMediaElement*>();
    return *elements;
#endif
}

HTMLMediaElement::HTMLMediaElement(const QualifiedName& tagName, Document& document, bool createdByParser)
    : HTMLElement(tagName, document)
    , ActiveDOMObject(&document)
    , m_pendingActionTimer(*this, &HTMLMediaElement::pendingActionTimerFired)
    , m_progressEventTimer(*this, &HTMLMediaElement::progressEventTimerFired)
    , m_playbackProgressTimer(*this, &HTMLMediaElement::playbackProgressTimerFired)
    , m_scanTimer(*this, &HTMLMediaElement::scanTimerFired)
    , m_seekToPlaybackPositionEndedTimer(*this, &HTMLMediaElement::seekToPlaybackPositionEndedTimerFired)
    , m_seekTaskQueue(document)
    , m_resizeTaskQueue(document)
    , m_shadowDOMTaskQueue(document)
    , m_playedTimeRanges()
    , m_asyncEventQueue(*this)
    , m_requestedPlaybackRate(1)
    , m_reportedPlaybackRate(1)
    , m_defaultPlaybackRate(1)
    , m_webkitPreservesPitch(true)
    , m_networkState(NETWORK_EMPTY)
    , m_readyState(HAVE_NOTHING)
    , m_readyStateMaximum(HAVE_NOTHING)
    , m_volume(1.0f)
    , m_volumeInitialized(false)
    , m_previousProgressTime(std::numeric_limits<double>::max())
    , m_clockTimeAtLastUpdateEvent(0)
    , m_lastTimeUpdateEventMovieTime(MediaTime::positiveInfiniteTime())
    , m_loadState(WaitingForSource)
    , m_videoFullscreenMode(VideoFullscreenModeNone)
#if PLATFORM(IOS)
    , m_videoFullscreenGravity(MediaPlayer::VideoGravityResizeAspect)
#endif
    , m_preload(MediaPlayer::Auto)
    , m_displayMode(Unknown)
    , m_processingMediaPlayerCallback(0)
#if ENABLE(MEDIA_SOURCE)
    , m_droppedVideoFrames(0)
#endif
    , m_clockTimeAtLastCachedTimeUpdate(0)
    , m_minimumClockTimeToUpdateCachedTime(0)
    , m_pendingActionFlags(0)
    , m_actionAfterScan(Nothing)
    , m_scanType(Scan)
    , m_scanDirection(Forward)
    , m_firstTimePlaying(true)
    , m_playing(false)
    , m_isWaitingUntilMediaCanStart(false)
    , m_shouldDelayLoadEvent(false)
    , m_haveFiredLoadedData(false)
    , m_inActiveDocument(true)
    , m_autoplaying(true)
    , m_muted(false)
    , m_explicitlyMuted(false)
    , m_initiallyMuted(false)
    , m_paused(true)
    , m_seeking(false)
    , m_sentStalledEvent(false)
    , m_sentEndEvent(false)
    , m_pausedInternal(false)
    , m_sendProgressEvents(true)
    , m_closedCaptionsVisible(false)
    , m_webkitLegacyClosedCaptionOverride(false)
    , m_completelyLoaded(false)
    , m_havePreparedToPlay(false)
#if PLATFORM(WKC)
    , m_playRequested(false)
#endif
    , m_parsingInProgress(createdByParser)
    , m_elementIsHidden(document.hidden())
    , m_creatingControls(false)
#if ENABLE(MEDIA_CONTROLS_SCRIPT)
    , m_mediaControlsDependOnPageScaleFactor(false)
    , m_haveSetUpCaptionContainer(false)
#endif
    , m_isScrubbingRemotely(false)
#if ENABLE(VIDEO_TRACK)
    , m_tracksAreReady(true)
    , m_haveVisibleTextTrack(false)
    , m_processingPreferenceChange(false)
    , m_lastTextTrackUpdateTime(MediaTime(-1, 1))
    , m_captionDisplayMode(CaptionUserPreferences::Automatic)
    , m_audioTracks(0)
    , m_textTracks(0)
    , m_videoTracks(0)
    , m_ignoreTrackDisplayUpdate(0)
#endif
#if ENABLE(WEB_AUDIO)
    , m_audioSourceNode(0)
#endif
    , m_mediaSession(std::make_unique<MediaElementSession>(*this))
    , m_reportedExtraMemoryCost(0)
#if ENABLE(MEDIA_STREAM)
    , m_mediaStreamSrcObject(nullptr)
#endif
{
    allMediaElements().add(this);

    LOG(Media, "HTMLMediaElement::HTMLMediaElement(%p)", this);
    setHasCustomStyleResolveCallbacks();

    m_mediaSession->addBehaviorRestriction(MediaElementSession::RequireUserGestureForFullscreen);
    m_mediaSession->addBehaviorRestriction(MediaElementSession::RequirePageConsentToLoadMedia);
#if ENABLE(WIRELESS_PLAYBACK_TARGET)
    m_mediaSession->addBehaviorRestriction(MediaElementSession::RequireUserGestureToAutoplayToExternalDevice);
#endif

    // FIXME: We should clean up and look to better merge the iOS and non-iOS code below.
    Settings* settings = document.settings();
#if !PLATFORM(IOS)
    if (settings && settings->requiresUserGestureForMediaPlayback()) {
        m_mediaSession->addBehaviorRestriction(MediaElementSession::RequireUserGestureForRateChange);
        m_mediaSession->addBehaviorRestriction(MediaElementSession::RequireUserGestureForLoad);
    }
#if PLATFORM(WKC)
    m_mediaSession->addBehaviorRestriction(MediaElementSession::RequireUserGestureForRateChange);
#endif
#else
    m_sendProgressEvents = false;
    if (!settings || settings->requiresUserGestureForMediaPlayback()) {
        // Allow autoplay in a MediaDocument that is not in an iframe.
        if (document.ownerElement() || !document.isMediaDocument())
            m_mediaSession->addBehaviorRestriction(MediaElementSession::RequireUserGestureForRateChange);
#if ENABLE(WIRELESS_PLAYBACK_TARGET)
        m_mediaSession->addBehaviorRestriction(MediaElementSession::RequireUserGestureToShowPlaybackTargetPicker);
#endif
    } else {
        // Relax RequireUserGestureForFullscreen when requiresUserGestureForMediaPlayback is not set:
        m_mediaSession->removeBehaviorRestriction(MediaElementSession::RequireUserGestureForFullscreen);
    }
    if (settings && settings->invisibleAutoplayNotPermitted())
        m_mediaSession->addBehaviorRestriction(MediaElementSession::InvisibleAutoplayNotPermitted);
#endif // !PLATFORM(IOS)

    if (settings && settings->audioPlaybackRequiresUserGesture() && settings->requiresUserGestureForMediaPlayback())
        m_mediaSession->addBehaviorRestriction(MediaElementSession::RequireUserGestureForAudioRateChange);

#if ENABLE(VIDEO_TRACK)
    if (document.page())
        m_captionDisplayMode = document.page()->group().captionPreferences()->captionDisplayMode();
#endif

#if ENABLE(MEDIA_SESSION)
    setSessionInternal(document.defaultMediaSession());
#endif

    registerWithDocument(document);

    mediaSession().clientWillBeginAutoplaying();
}

HTMLMediaElement::~HTMLMediaElement()
{
    LOG(Media, "HTMLMediaElement::~HTMLMediaElement(%p)", this);

    allMediaElements().remove(this);

    m_asyncEventQueue.close();

    setShouldDelayLoadEvent(false);
    unregisterWithDocument(document());

#if ENABLE(VIDEO_TRACK)
    if (m_audioTracks) {
        m_audioTracks->clearElement();
        for (unsigned i = 0; i < m_audioTracks->length(); ++i)
            m_audioTracks->item(i)->clearClient();
    }
    if (m_textTracks)
        m_textTracks->clearElement();
    if (m_textTracks) {
        for (unsigned i = 0; i < m_textTracks->length(); ++i)
            m_textTracks->item(i)->clearClient();
    }
    if (m_videoTracks) {
        m_videoTracks->clearElement();
        for (unsigned i = 0; i < m_videoTracks->length(); ++i)
            m_videoTracks->item(i)->clearClient();
    }
#endif

#if ENABLE(WIRELESS_PLAYBACK_TARGET)
    if (hasEventListeners(eventNames().webkitplaybacktargetavailabilitychangedEvent)) {
        m_hasPlaybackTargetAvailabilityListeners = false;
        m_mediaSession->setHasPlaybackTargetAvailabilityListeners(*this, false);
        updateMediaState();
    }
#endif

    if (m_mediaController) {
        m_mediaController->removeMediaElement(this);
        m_mediaController = nullptr;
    }

#if ENABLE(MEDIA_SOURCE)
    closeMediaSource();
#endif

#if ENABLE(ENCRYPTED_MEDIA_V2)
    setMediaKeys(0);
#endif

#if ENABLE(MEDIA_CONTROLS_SCRIPT)
    if (m_isolatedWorld)
        m_isolatedWorld->clearWrappers();
#endif

#if ENABLE(MEDIA_SESSION)
    if (m_session) {
        m_session->removeMediaElement(*this);
        m_session = nullptr;
    }
#endif

    m_seekTaskQueue.close();
    m_visibilityChangeTaskQueue.close();

    m_completelyLoaded = true;

    m_mediaSession = nullptr;
}

void HTMLMediaElement::registerWithDocument(Document& document)
{
    m_mediaSession->registerWithDocument(document);

    if (m_isWaitingUntilMediaCanStart)
        document.addMediaCanStartListener(this);

#if !PLATFORM(IOS)
    document.registerForMediaVolumeCallbacks(this);
    document.registerForPrivateBrowsingStateChangedCallbacks(this);
#endif

    document.registerForVisibilityStateChangedCallbacks(this);

#if ENABLE(VIDEO_TRACK)
    if (m_requireCaptionPreferencesChangedCallbacks)
        document.registerForCaptionPreferencesChangedCallbacks(this);
#endif

#if ENABLE(MEDIA_CONTROLS_SCRIPT)
    if (m_mediaControlsDependOnPageScaleFactor)
        document.registerForPageScaleFactorChangedCallbacks(this);
#endif

#if ENABLE(WIRELESS_PLAYBACK_TARGET)
    document.registerForDocumentSuspensionCallbacks(this);
#endif

    document.registerForAllowsMediaDocumentInlinePlaybackChangedCallbacks(*this);

    document.addAudioProducer(this);
    addElementToDocumentMap(*this, document);
}

void HTMLMediaElement::unregisterWithDocument(Document& document)
{
    m_mediaSession->unregisterWithDocument(document);

    if (m_isWaitingUntilMediaCanStart)
        document.removeMediaCanStartListener(this);

#if !PLATFORM(IOS)
    document.unregisterForMediaVolumeCallbacks(this);
    document.unregisterForPrivateBrowsingStateChangedCallbacks(this);
#endif

    document.unregisterForVisibilityStateChangedCallbacks(this);

#if ENABLE(VIDEO_TRACK)
    if (m_requireCaptionPreferencesChangedCallbacks)
        document.unregisterForCaptionPreferencesChangedCallbacks(this);
#endif

#if ENABLE(MEDIA_CONTROLS_SCRIPT)
    if (m_mediaControlsDependOnPageScaleFactor)
        document.unregisterForPageScaleFactorChangedCallbacks(this);
#endif

#if ENABLE(WIRELESS_PLAYBACK_TARGET)
    document.unregisterForDocumentSuspensionCallbacks(this);
#endif

    document.unregisterForAllowsMediaDocumentInlinePlaybackChangedCallbacks(*this);

    document.removeAudioProducer(this);
    removeElementFromDocumentMap(*this, document);
}

void HTMLMediaElement::didMoveToNewDocument(Document* oldDocument)
{
    if (m_shouldDelayLoadEvent) {
        if (oldDocument)
            oldDocument->decrementLoadEventDelayCount();
        document().incrementLoadEventDelayCount();
    }

    if (oldDocument)
        unregisterWithDocument(*oldDocument);

    registerWithDocument(document());

    HTMLElement::didMoveToNewDocument(oldDocument);
    updateShouldAutoplay();
}

#if ENABLE(WIRELESS_PLAYBACK_TARGET)
void HTMLMediaElement::prepareForDocumentSuspension()
{
    m_mediaSession->unregisterWithDocument(document());
}

void HTMLMediaElement::resumeFromDocumentSuspension()
{
    m_mediaSession->registerWithDocument(document());
    updateShouldAutoplay();
}
#endif

bool HTMLMediaElement::hasCustomFocusLogic() const
{
    return true;
}

bool HTMLMediaElement::supportsFocus() const
{
    if (document().isMediaDocument())
        return false;

    // If no controls specified, we should still be able to focus the element if it has tabIndex.
    return controls() ||  HTMLElement::supportsFocus();
}

bool HTMLMediaElement::isMouseFocusable() const
{
    return false;
}

void HTMLMediaElement::parseAttribute(const QualifiedName& name, const AtomicString& value)
{
    if (name == srcAttr) {
#if PLATFORM(IOS)
        // Note, unless the restriction on requiring user action has been removed,
        // do not begin downloading data on iOS.
        if (!value.isNull() && m_mediaSession->dataLoadingPermitted(*this))
#else
        // Trigger a reload, as long as the 'src' attribute is present.
        if (!value.isNull())
#endif
        {
            clearMediaPlayer(LoadMediaResource);
            scheduleDelayedAction(LoadMediaResource);
        }
    } else if (name == controlsAttr)
        configureMediaControls();
    else if (name == loopAttr)
        updateSleepDisabling();
    else if (name == preloadAttr) {
        if (equalIgnoringCase(value, "none"))
            m_preload = MediaPlayer::None;
        else if (equalIgnoringCase(value, "metadata"))
            m_preload = MediaPlayer::MetaData;
        else {
            // The spec does not define an "invalid value default" but "auto" is suggested as the
            // "missing value default", so use it for everything except "none" and "metadata"
            m_preload = MediaPlayer::Auto;
        }

        // The attribute must be ignored if the autoplay attribute is present
        if (!autoplay() && m_player)
            m_player->setPreload(m_mediaSession->effectivePreloadForElement(*this));

    } else if (name == mediagroupAttr)
        setMediaGroup(value);
    else
        HTMLElement::parseAttribute(name, value);
}

void HTMLMediaElement::finishParsingChildren()
{
    HTMLElement::finishParsingChildren();
    m_parsingInProgress = false;
    
#if ENABLE(VIDEO_TRACK)
    if (!RuntimeEnabledFeatures::sharedFeatures().webkitVideoTrackEnabled())
        return;

    if (descendantsOfType<HTMLTrackElement>(*this).first())
        scheduleDelayedAction(ConfigureTextTracks);
#endif
}

bool HTMLMediaElement::rendererIsNeeded(const RenderStyle& style)
{
    return controls() && HTMLElement::rendererIsNeeded(style);
}

RenderPtr<RenderElement> HTMLMediaElement::createElementRenderer(Ref<RenderStyle>&& style, const RenderTreePosition&)
{
    return createRenderer<RenderMedia>(*this, WTF::move(style));
}

bool HTMLMediaElement::childShouldCreateRenderer(const Node& child) const
{
#if ENABLE(MEDIA_CONTROLS_SCRIPT)
    return hasShadowRootParent(child) && HTMLElement::childShouldCreateRenderer(child);
#else
    if (!hasMediaControls())
        return false;
    // <media> doesn't allow its content, including shadow subtree, to
    // be rendered. So this should return false for most of the children.
    // One exception is a shadow tree built for rendering controls which should be visible.
    // So we let them go here by comparing its subtree root with one of the controls.
    return &mediaControls()->treeScope() == &child.treeScope()
        && hasShadowRootParent(child)
        && HTMLElement::childShouldCreateRenderer(child);
#endif
}

Node::InsertionNotificationRequest HTMLMediaElement::insertedInto(ContainerNode& insertionPoint)
{
    LOG(Media, "HTMLMediaElement::insertedInto(%p)", this);

    HTMLElement::insertedInto(insertionPoint);
    if (insertionPoint.inDocument()) {
        m_inActiveDocument = true;

#if PLATFORM(IOS)
        if (m_networkState == NETWORK_EMPTY && !fastGetAttribute(srcAttr).isEmpty() && m_mediaSession->dataLoadingPermitted(*this))
#else
        if (m_networkState == NETWORK_EMPTY && !fastGetAttribute(srcAttr).isEmpty())
#endif
            scheduleDelayedAction(LoadMediaResource);
    }

    if (!m_explicitlyMuted) {
        m_explicitlyMuted = true;
        m_muted = fastHasAttribute(mutedAttr);
    }

    configureMediaControls();
    return InsertionDone;
}

void HTMLMediaElement::removedFrom(ContainerNode& insertionPoint)
{
    LOG(Media, "HTMLMediaElement::removedFrom(%p)", this);

    m_inActiveDocument = false;
    if (insertionPoint.inDocument()) {
        if (hasMediaControls())
            mediaControls()->hide();
#if PLATFORM(WKC)
        if (m_player)
            m_player->removedFromDocument();
#else
        if (m_networkState > NETWORK_EMPTY)
            pause();
#endif
        if (m_videoFullscreenMode != VideoFullscreenModeNone)
            exitFullscreen();

        if (m_player) {
            size_t extraMemoryCost = m_player->extraMemoryCost();
            if (extraMemoryCost > m_reportedExtraMemoryCost) {
                JSC::VM& vm = JSDOMWindowBase::commonVM();
                JSC::JSLockHolder lock(vm);

                size_t extraMemoryCostDelta = extraMemoryCost - m_reportedExtraMemoryCost;
                m_reportedExtraMemoryCost = extraMemoryCost;
                // FIXME: Adopt reportExtraMemoryVisited, and switch to reportExtraMemoryAllocated.
                // https://bugs.webkit.org/show_bug.cgi?id=142595
                vm.heap.deprecatedReportExtraMemory(extraMemoryCostDelta);
            }
        }
    }

    HTMLElement::removedFrom(insertionPoint);
}

void HTMLMediaElement::willAttachRenderers()
{
    ASSERT(!renderer());
}

void HTMLMediaElement::didAttachRenderers()
{
    if (RenderElement* renderer = this->renderer()) {
        renderer->updateFromElement();
        if (m_mediaSession->hasBehaviorRestriction(MediaElementSession::InvisibleAutoplayNotPermitted))
            renderer->registerForVisibleInViewportCallback();
    }
    updateShouldAutoplay();
}

void HTMLMediaElement::willDetachRenderers()
{
    if (renderer() && m_mediaSession->hasBehaviorRestriction(MediaElementSession::InvisibleAutoplayNotPermitted))
        renderer()->unregisterForVisibleInViewportCallback();
}

void HTMLMediaElement::didDetachRenderers()
{
    updateShouldAutoplay();
}

void HTMLMediaElement::didRecalcStyle(Style::Change)
{
    if (renderer())
        renderer()->updateFromElement();
}

void HTMLMediaElement::scheduleDelayedAction(DelayedActionType actionType)
{
    LOG(Media, "HTMLMediaElement::scheduleDelayedAction(%p) - setting %s flag", this, actionName(actionType).utf8().data());

    if ((actionType & LoadMediaResource) && !(m_pendingActionFlags & LoadMediaResource)) {
#if PLATFORM(WKC)
        if (!canPrepareForLoad())
            return;
#endif
        prepareForLoad();
        setFlags(m_pendingActionFlags, LoadMediaResource);
    }

#if ENABLE(VIDEO_TRACK)
    if (RuntimeEnabledFeatures::sharedFeatures().webkitVideoTrackEnabled() && (actionType & ConfigureTextTracks))
        setFlags(m_pendingActionFlags, ConfigureTextTracks);
#endif

#if USE(PLATFORM_TEXT_TRACK_MENU)
    if (actionType & TextTrackChangesNotification)
        setFlags(m_pendingActionFlags, TextTrackChangesNotification);
#endif

#if ENABLE(WIRELESS_PLAYBACK_TARGET)
    if (actionType & CheckPlaybackTargetCompatablity)
        setFlags(m_pendingActionFlags, CheckPlaybackTargetCompatablity);
#endif

    if (actionType & CheckMediaState)
        setFlags(m_pendingActionFlags, CheckMediaState);

    m_pendingActionTimer.startOneShot(0);
}

void HTMLMediaElement::scheduleNextSourceChild()
{
    // Schedule the timer to try the next <source> element WITHOUT resetting state ala prepareForLoad.
    LOG(Media, "HTMLMediaElement::scheduleNextSourceChild(%p) - setting %s flag", this, actionName(LoadMediaResource).utf8().data());
    setFlags(m_pendingActionFlags, LoadMediaResource);
    m_pendingActionTimer.startOneShot(0);
}

void HTMLMediaElement::scheduleEvent(const AtomicString& eventName)
{
#if LOG_MEDIA_EVENTS
    LOG(Media, "HTMLMediaElement::scheduleEvent(%p) - scheduling '%s'", this, eventName.string().ascii().data());
#endif
    RefPtr<Event> event = Event::create(eventName, false, true);
    
    // Don't set the event target, the event queue will set it in GenericEventQueue::timerFired and setting it here
    // will trigger an ASSERT if this element has been marked for deletion.

    m_asyncEventQueue.enqueueEvent(event.release());
}

void HTMLMediaElement::pendingActionTimerFired()
{
    Ref<HTMLMediaElement> protect(*this); // loadNextSourceChild may fire 'beforeload', which can make arbitrary DOM mutations.

#if ENABLE(VIDEO_TRACK)
    if (RuntimeEnabledFeatures::sharedFeatures().webkitVideoTrackEnabled() && (m_pendingActionFlags & ConfigureTextTracks))
        configureTextTracks();
#endif

    if (m_pendingActionFlags & LoadMediaResource) {
        if (m_loadState == LoadingFromSourceElement)
            loadNextSourceChild();
        else
            loadInternal();
    }

#if USE(PLATFORM_TEXT_TRACK_MENU)
    if (RuntimeEnabledFeatures::sharedFeatures().webkitVideoTrackEnabled() && (m_pendingActionFlags & TextTrackChangesNotification))
        notifyMediaPlayerOfTextTrackChanges();
#endif

#if ENABLE(WIRELESS_PLAYBACK_TARGET)
    if (m_pendingActionFlags & CheckPlaybackTargetCompatablity && m_player && m_player->isCurrentPlaybackTargetWireless() && !m_player->canPlayToWirelessPlaybackTarget()) {
        LOG(Media, "HTMLMediaElement::pendingActionTimerFired(%p) - calling setShouldPlayToPlaybackTarget(false)", this);
        m_failedToPlayToWirelessTarget = true;
        m_player->setShouldPlayToPlaybackTarget(false);
    }

    if (m_pendingActionFlags & CheckMediaState)
        updateMediaState();
#endif

    m_pendingActionFlags = 0;
}

PassRefPtr<MediaError> HTMLMediaElement::error() const 
{
    return m_error;
}

void HTMLMediaElement::setSrc(const String& url)
{
    setAttribute(srcAttr, url);
}

#if ENABLE(MEDIA_STREAM)
void HTMLMediaElement::setSrcObject(MediaStream* mediaStream)
{
    // FIXME: Setting the srcObject attribute may cause other changes to the media element's internal state:
    // Specifically, if srcObject is specified, the UA must use it as the source of media, even if the src
    // attribute is also set or children are present. If the value of srcObject is replaced or set to null
    // the UA must re-run the media element load algorithm.
    //
    // https://bugs.webkit.org/show_bug.cgi?id=124896

    m_mediaStreamSrcObject = mediaStream;
}
#endif

HTMLMediaElement::NetworkState HTMLMediaElement::networkState() const
{
    return m_networkState;
}

String HTMLMediaElement::canPlayType(const String& mimeType, const String& keySystem, const URL& url) const
{
#if PLATFORM(WKC)
    // disable audio tag (a media player is never created if MediaPlayer::load is not called)
    if (!isVideo())
        return emptyString();
#endif

    MediaEngineSupportParameters parameters;
    ContentType contentType(mimeType);
    parameters.type = contentType.type().lower();
    parameters.codecs = contentType.parameter(ASCIILiteral("codecs"));
    parameters.url = url;
#if ENABLE(ENCRYPTED_MEDIA)
    parameters.keySystem = keySystem;
#else
    UNUSED_PARAM(keySystem);
#endif
    MediaPlayer::SupportsType support = MediaPlayer::supportsType(parameters, this);
    String canPlay;

    // 4.8.10.3
    switch (support)
    {
        case MediaPlayer::IsNotSupported:
            canPlay = emptyString();
            break;
        case MediaPlayer::MayBeSupported:
            canPlay = ASCIILiteral("maybe");
            break;
        case MediaPlayer::IsSupported:
            canPlay = ASCIILiteral("probably");
            break;
    }
    
    LOG(Media, "HTMLMediaElement::canPlayType(%p) - [%s, %s, %s] -> %s", this, mimeType.utf8().data(), keySystem.utf8().data(), url.stringCenterEllipsizedToLength().utf8().data(), canPlay.utf8().data());

    return canPlay;
}

double HTMLMediaElement::getStartDate() const
{
    if (!m_player)
        return std::numeric_limits<double>::quiet_NaN();
    return m_player->getStartDate().toDouble();
}

void HTMLMediaElement::load()
{
    Ref<HTMLMediaElement> protect(*this); // loadInternal may result in a 'beforeload' event, which can make arbitrary DOM mutations.
    
    LOG(Media, "HTMLMediaElement::load(%p)", this);
    
    if (!m_mediaSession->dataLoadingPermitted(*this))
        return;
    if (ScriptController::processingUserGestureForMedia())
        removeBehaviorsRestrictionsAfterFirstUserGesture();

#if PLATFORM(WKC)
    if (!canPrepareForLoad())
        return;
#endif
    prepareForLoad();
    loadInternal();
#if PLATFORM(WKC)
    if (ScriptController::processingUserGestureForMedia() || m_mediaSession->playbackPermitted(*this))
        prepareToPlay();
#else
    prepareToPlay();
#endif
}

void HTMLMediaElement::prepareForLoad()
{
    LOG(Media, "HTMLMediaElement::prepareForLoad(%p)", this);

    // Perform the cleanup required for the resource load algorithm to run.
    stopPeriodicTimers();
    m_pendingActionTimer.stop();
    // FIXME: Figure out appropriate place to reset LoadTextTrackResource if necessary and set m_pendingActionFlags to 0 here.
    m_pendingActionFlags &= ~LoadMediaResource;
    m_sentEndEvent = false;
    m_sentStalledEvent = false;
    m_haveFiredLoadedData = false;
    m_completelyLoaded = false;
    m_havePreparedToPlay = false;
    m_displayMode = Unknown;
    m_currentSrc = URL();

#if ENABLE(WIRELESS_PLAYBACK_TARGET)
    m_failedToPlayToWirelessTarget = false;
#endif

    // 1 - Abort any already-running instance of the resource selection algorithm for this element.
    m_loadState = WaitingForSource;
    m_currentSourceNode = nullptr;

    // 2 - If there are any tasks from the media element's media element event task source in 
    // one of the task queues, then remove those tasks.
    cancelPendingEventsAndCallbacks();

    // 3 - If the media element's networkState is set to NETWORK_LOADING or NETWORK_IDLE, queue
    // a task to fire a simple event named abort at the media element.
    if (m_networkState == NETWORK_LOADING || m_networkState == NETWORK_IDLE)
        scheduleEvent(eventNames().abortEvent);

#if ENABLE(MEDIA_SOURCE)
    closeMediaSource();
#endif

    createMediaPlayer();

    // 4 - If the media element's networkState is not set to NETWORK_EMPTY, then run these substeps
    if (m_networkState != NETWORK_EMPTY) {
        // 4.1 - Queue a task to fire a simple event named emptied at the media element.
        scheduleEvent(eventNames().emptiedEvent);

        // 4.2 - If a fetching process is in progress for the media element, the user agent should stop it.
        m_networkState = NETWORK_EMPTY;

        // 4.3 - Forget the media element's media-resource-specific tracks.
        forgetResourceSpecificTracks();

        // 4.4 - If readyState is not set to HAVE_NOTHING, then set it to that state.
        m_readyState = HAVE_NOTHING;
        m_readyStateMaximum = HAVE_NOTHING;

        // 4.5 - If the paused attribute is false, then set it to true.
        m_paused = true;

        // 4.6 - If seeking is true, set it to false.
        clearSeeking();

        // 4.7 - Set the current playback position to 0.
        //       Set the official playback position to 0.
        //       If this changed the official playback position, then queue a task to fire a simple event named timeupdate at the media element.
        // FIXME: Add support for firing this event. e.g., scheduleEvent(eventNames().timeUpdateEvent);

        // 4.8 - Set the initial playback position to 0.
        // FIXME: Make this less subtle. The position only becomes 0 because of the createMediaPlayer() call
        // above.
        refreshCachedTime();

        invalidateCachedTime();

        // 4.9 - Set the timeline offset to Not-a-Number (NaN).
        // 4.10 - Update the duration attribute to Not-a-Number (NaN).

        updateMediaController();
#if ENABLE(VIDEO_TRACK)
        if (RuntimeEnabledFeatures::sharedFeatures().webkitVideoTrackEnabled())
            updateActiveTextTrackCues(MediaTime::zeroTime());
#endif
    }

    // 5 - Set the playbackRate attribute to the value of the defaultPlaybackRate attribute.
    setPlaybackRate(defaultPlaybackRate());

    // 6 - Set the error attribute to null and the autoplaying flag to true.
    m_error = nullptr;
    m_autoplaying = true;

    // 7 - Invoke the media element's resource selection algorithm.

    // 8 - Note: Playback of any previously playing media resource for this element stops.

    // The resource selection algorithm
    // 1 - Set the networkState to NETWORK_NO_SOURCE
    m_networkState = NETWORK_NO_SOURCE;

    // 2 - Asynchronously await a stable state.

    m_playedTimeRanges = TimeRanges::create();

    // FIXME: Investigate whether these can be moved into m_networkState != NETWORK_EMPTY block above
    // so they are closer to the relevant spec steps.
    m_lastSeekTime = MediaTime::zeroTime();

    // The spec doesn't say to block the load event until we actually run the asynchronous section
    // algorithm, but do it now because we won't start that until after the timer fires and the 
    // event may have already fired by then.
    MediaPlayer::Preload effectivePreload = m_mediaSession->effectivePreloadForElement(*this);
    if (effectivePreload != MediaPlayer::None)
        setShouldDelayLoadEvent(true);

#if PLATFORM(IOS)
    if (effectivePreload != MediaPlayer::None && m_mediaSession->allowsAutomaticMediaDataLoading(*this))
        prepareToPlay();
#endif

    configureMediaControls();
}

void HTMLMediaElement::loadInternal()
{
#if PLATFORM(WKC)
    // disable audio tag (a media player is never created if MediaPlayer::load is not called)
    if (!isVideo()) {
        noneSupported();
        return;
    }
    if (!m_player)
        createMediaPlayer();
#endif

    // Some of the code paths below this function dispatch the BeforeLoad event. This ASSERT helps
    // us catch those bugs more quickly without needing all the branches to align to actually
    // trigger the event.
    ASSERT(NoEventDispatchAssertion::isEventAllowedInMainThread());

    // If we can't start a load right away, start it later.
    if (!m_mediaSession->pageAllowsDataLoading(*this)) {
        setShouldDelayLoadEvent(false);
        if (m_isWaitingUntilMediaCanStart)
            return;
        document().addMediaCanStartListener(this);
        m_isWaitingUntilMediaCanStart = true;
        return;
    }

    clearFlags(m_pendingActionFlags, LoadMediaResource);

    // Once the page has allowed an element to load media, it is free to load at will. This allows a 
    // playlist that starts in a foreground tab to continue automatically if the tab is subsequently 
    // put into the background.
    m_mediaSession->removeBehaviorRestriction(MediaElementSession::RequirePageConsentToLoadMedia);

#if ENABLE(VIDEO_TRACK)
    if (hasMediaControls())
        mediaControls()->changedClosedCaptionsVisibility();

    // HTMLMediaElement::textTracksAreReady will need "... the text tracks whose mode was not in the
    // disabled state when the element's resource selection algorithm last started".
    if (RuntimeEnabledFeatures::sharedFeatures().webkitVideoTrackEnabled()) {
        m_textTracksWhenResourceSelectionBegan.clear();
        if (m_textTracks) {
            for (unsigned i = 0; i < m_textTracks->length(); ++i) {
                TextTrack* track = m_textTracks->item(i);
                if (track->mode() != TextTrack::disabledKeyword())
                    m_textTracksWhenResourceSelectionBegan.append(track);
            }
        }
    }
#endif

    selectMediaResource();
}

void HTMLMediaElement::selectMediaResource()
{
    LOG(Media, "HTMLMediaElement::selectMediaResource(%p)", this);

    enum Mode { attribute, children };

    // 3 - If the media element has a src attribute, then let mode be attribute.
    Mode mode = attribute;
    if (!fastHasAttribute(srcAttr)) {
        // Otherwise, if the media element does not have a src attribute but has a source 
        // element child, then let mode be children and let candidate be the first such 
        // source element child in tree order.
        if (auto firstSource = childrenOfType<HTMLSourceElement>(*this).first()) {
            mode = children;
            m_nextChildNodeToConsider = firstSource;
            m_currentSourceNode = nullptr;
        } else {
            // Otherwise the media element has neither a src attribute nor a source element 
            // child: set the networkState to NETWORK_EMPTY, and abort these steps; the 
            // synchronous section ends.
            m_loadState = WaitingForSource;
            setShouldDelayLoadEvent(false);
            m_networkState = NETWORK_EMPTY;

            LOG(Media, "HTMLMediaElement::selectMediaResource(%p) - nothing to load", this);
            return;
        }
    }

    // 4 - Set the media element's delaying-the-load-event flag to true (this delays the load event), 
    // and set its networkState to NETWORK_LOADING.
    setShouldDelayLoadEvent(true);
    m_networkState = NETWORK_LOADING;

    // 5 - Queue a task to fire a simple event named loadstart at the media element.
    scheduleEvent(eventNames().loadstartEvent);

    // 6 - If mode is attribute, then run these substeps
    if (mode == attribute) {
        m_loadState = LoadingFromSrcAttr;

        // If the src attribute's value is the empty string ... jump down to the failed step below
        URL mediaURL = getNonEmptyURLAttribute(srcAttr);
        if (mediaURL.isEmpty()) {
            mediaLoadingFailed(MediaPlayer::FormatError);
            LOG(Media, "HTMLMediaElement::selectMediaResource(%p) -  empty 'src'", this);
            return;
        }

        if (!isSafeToLoadURL(mediaURL, Complain) || !dispatchBeforeLoadEvent(mediaURL.string())) {
            mediaLoadingFailed(MediaPlayer::FormatError);
            return;
        }

        // No type or key system information is available when the url comes
        // from the 'src' attribute so MediaPlayer
        // will have to pick a media engine based on the file extension.
        ContentType contentType((String()));
        loadResource(mediaURL, contentType, String());
        LOG(Media, "HTMLMediaElement::selectMediaResource(%p) - using 'src' attribute url", this);
        return;
    }

    // Otherwise, the source elements will be used
    loadNextSourceChild();
}

void HTMLMediaElement::loadNextSourceChild()
{
    ContentType contentType((String()));
    String keySystem;
    URL mediaURL = selectNextSourceChild(&contentType, &keySystem, Complain);
    if (!mediaURL.isValid()) {
        waitForSourceChange();
        return;
    }

    // Recreate the media player for the new url
    createMediaPlayer();

    m_loadState = LoadingFromSourceElement;
    loadResource(mediaURL, contentType, keySystem);
}

void HTMLMediaElement::loadResource(const URL& initialURL, ContentType& contentType, const String& keySystem)
{
    ASSERT(isSafeToLoadURL(initialURL, Complain));

    LOG(Media, "HTMLMediaElement::loadResource(%p) - %s, %s, %s", this, urlForLoggingMedia(initialURL).utf8().data(), contentType.raw().utf8().data(), keySystem.utf8().data());

#if PLATFORM(WKC)
    if (!m_player)
        return;
#endif
    Frame* frame = document().frame();
    if (!frame) {
        mediaLoadingFailed(MediaPlayer::FormatError);
        return;
    }

    Page* page = frame->page();
    if (!page) {
        mediaLoadingFailed(MediaPlayer::FormatError);
        return;
    }

    URL url = initialURL;
    if (!frame->loader().willLoadMediaElementURL(url)) {
        mediaLoadingFailed(MediaPlayer::FormatError);
        return;
    }

#if ENABLE(CONTENT_EXTENSIONS)
    ResourceRequest request(url);
    DocumentLoader* documentLoader = frame->loader().documentLoader();

    if (page->userContentController() && documentLoader)
        page->userContentController()->processContentExtensionRulesForLoad(*page, request, ResourceType::Media, *documentLoader);

    if (request.isNull()) {
        mediaLoadingFailed(MediaPlayer::FormatError);
        return;
    }
#endif

    // The resource fetch algorithm 
    m_networkState = NETWORK_LOADING;

    // If the url should be loaded from the application cache, pass the url of the cached file
    // to the media engine.
    ApplicationCacheHost* cacheHost = frame->loader().documentLoader()->applicationCacheHost();
    ApplicationCacheResource* resource = 0;
    if (cacheHost && cacheHost->shouldLoadResourceFromApplicationCache(ResourceRequest(url), resource)) {
        // Resources that are not present in the manifest will always fail to load (at least, after the
        // cache has been primed the first time), making the testing of offline applications simpler.
        if (!resource || resource->path().isEmpty()) {
            mediaLoadingFailed(MediaPlayer::NetworkError);
            return;
        }
    }

    // Log that we started loading a media element.
    if (Frame* frame = document().frame())
        frame->mainFrame().diagnosticLoggingClient().logDiagnosticMessageWithValue(DiagnosticLoggingKeys::mediaKey(), isVideo() ? DiagnosticLoggingKeys::videoKey() : DiagnosticLoggingKeys::audioKey(), DiagnosticLoggingKeys::loadingKey(), ShouldSample::No);

    m_firstTimePlaying = true;

    // Set m_currentSrc *before* changing to the cache url, the fact that we are loading from the app
    // cache is an internal detail not exposed through the media element API.
    m_currentSrc = url;

    if (resource) {
        url = ApplicationCacheHost::createFileURL(resource->path());
        LOG(Media, "HTMLMediaElement::loadResource(%p) - will load from app cache -> %s", this, urlForLoggingMedia(url).utf8().data());
    }

    LOG(Media, "HTMLMediaElement::loadResource(%p) - m_currentSrc -> %s", this, urlForLoggingMedia(m_currentSrc).utf8().data());

#if ENABLE(MEDIA_STREAM)
    if (MediaStreamRegistry::registry().lookup(url.string()))
        m_mediaSession->removeBehaviorRestriction(MediaElementSession::RequireUserGestureForRateChange);
#endif

    if (m_sendProgressEvents) 
        startProgressEventTimer();

    bool privateMode = document().page() && document().page()->usesEphemeralSession();
    m_player->setPrivateBrowsingMode(privateMode);

    // Reset display mode to force a recalculation of what to show because we are resetting the player.
    setDisplayMode(Unknown);

    if (!autoplay())
        m_player->setPreload(m_mediaSession->effectivePreloadForElement(*this));
    m_player->setPreservesPitch(m_webkitPreservesPitch);

    if (!m_explicitlyMuted) {
        m_explicitlyMuted = true;
        m_muted = fastHasAttribute(mutedAttr);
    }

    updateVolume();

#if ENABLE(MEDIA_SOURCE)
    ASSERT(!m_mediaSource);

    if (url.protocolIs(mediaSourceBlobProtocol))
        m_mediaSource = MediaSource::lookup(url.string());

    if (m_mediaSource) {
        if (m_mediaSource->attachToElement(this))
            m_player->load(url, contentType, m_mediaSource.get());
        else {
            // Forget our reference to the MediaSource, so we leave it alone
            // while processing remainder of load failure.
            m_mediaSource = nullptr;
            mediaLoadingFailed(MediaPlayer::FormatError);
        }
    } else
#endif
#if ENABLE(MEDIA_STREAM)
        if (m_mediaStreamSrcObject)
            m_player->load(m_mediaStreamSrcObject->privateStream());
        else
#endif
    if (!m_player->load(url, contentType, keySystem))
        mediaLoadingFailed(MediaPlayer::FormatError);

#if PLATFORM(WKC)
    if (autoplay() || m_playRequested) {
        m_playRequested = false;
        prepareToPlay();
    }
#endif

    // If there is no poster to display, allow the media engine to render video frames as soon as
    // they are available.
    updateDisplayState();

    if (renderer())
        renderer()->updateFromElement();
}

#if ENABLE(VIDEO_TRACK)
static bool trackIndexCompare(TextTrack* a,
                              TextTrack* b)
{
    return a->trackIndex() - b->trackIndex() < 0;
}

static bool eventTimeCueCompare(const std::pair<MediaTime, TextTrackCue*>& a, const std::pair<MediaTime, TextTrackCue*>& b)
{
    // 12 - Sort the tasks in events in ascending time order (tasks with earlier
    // times first).
    if (a.first != b.first)
        return a.first - b.first < MediaTime::zeroTime();

    // If the cues belong to different text tracks, it doesn't make sense to
    // compare the two tracks by the relative cue order, so return the relative
    // track order.
    if (a.second->track() != b.second->track())
        return trackIndexCompare(a.second->track(), b.second->track());

    // 12 - Further sort tasks in events that have the same time by the
    // relative text track cue order of the text track cues associated
    // with these tasks.
    return a.second->cueIndex() - b.second->cueIndex() < 0;
}

static bool compareCueInterval(const CueInterval& one, const CueInterval& two)
{
    return one.data()->isOrderedBefore(two.data());
};


void HTMLMediaElement::updateActiveTextTrackCues(const MediaTime& movieTime)
{
#if PLATFORM(WKC)
    return;
#endif

    // 4.8.10.8 Playing the media resource

    //  If the current playback position changes while the steps are running,
    //  then the user agent must wait for the steps to complete, and then must
    //  immediately rerun the steps.
    if (ignoreTrackDisplayUpdateRequests())
        return;

    LOG(Media, "HTMLMediaElement::updateActiveTextTrackCues(%p)", this);

    // 1 - Let current cues be a list of cues, initialized to contain all the
    // cues of all the hidden, showing, or showing by default text tracks of the
    // media element (not the disabled ones) whose start times are less than or
    // equal to the current playback position and whose end times are greater
    // than the current playback position.
    CueList currentCues;

    // The user agent must synchronously unset [the text track cue active] flag
    // whenever ... the media element's readyState is changed back to HAVE_NOTHING.
    if (m_readyState != HAVE_NOTHING && m_player) {
        currentCues = m_cueTree.allOverlaps(m_cueTree.createInterval(movieTime, movieTime));
        if (currentCues.size() > 1)
            std::sort(currentCues.begin(), currentCues.end(), &compareCueInterval);
    }

    CueList previousCues;
    CueList missedCues;

    // 2 - Let other cues be a list of cues, initialized to contain all the cues
    // of hidden, showing, and showing by default text tracks of the media
    // element that are not present in current cues.
    previousCues = m_currentlyActiveCues;

    // 3 - Let last time be the current playback position at the time this
    // algorithm was last run for this media element, if this is not the first
    // time it has run.
    MediaTime lastTime = m_lastTextTrackUpdateTime;

    // 4 - If the current playback position has, since the last time this
    // algorithm was run, only changed through its usual monotonic increase
    // during normal playback, then let missed cues be the list of cues in other
    // cues whose start times are greater than or equal to last time and whose
    // end times are less than or equal to the current playback position.
    // Otherwise, let missed cues be an empty list.
    if (lastTime >= MediaTime::zeroTime() && m_lastSeekTime < movieTime) {
        CueList potentiallySkippedCues =
            m_cueTree.allOverlaps(m_cueTree.createInterval(lastTime, movieTime));

        for (size_t i = 0; i < potentiallySkippedCues.size(); ++i) {
            MediaTime cueStartTime = potentiallySkippedCues[i].low();
            MediaTime cueEndTime = potentiallySkippedCues[i].high();

            // Consider cues that may have been missed since the last seek time.
            if (cueStartTime > std::max(m_lastSeekTime, lastTime) && cueEndTime < movieTime)
                missedCues.append(potentiallySkippedCues[i]);
        }
    }

    m_lastTextTrackUpdateTime = movieTime;

    // 5 - If the time was reached through the usual monotonic increase of the
    // current playback position during normal playback, and if the user agent
    // has not fired a timeupdate event at the element in the past 15 to 250ms
    // and is not still running event handlers for such an event, then the user
    // agent must queue a task to fire a simple event named timeupdate at the
    // element. (In the other cases, such as explicit seeks, relevant events get
    // fired as part of the overall process of changing the current playback
    // position.)
    if (!m_paused && m_lastSeekTime <= lastTime)
        scheduleTimeupdateEvent(false);

    // Explicitly cache vector sizes, as their content is constant from here.
    size_t currentCuesSize = currentCues.size();
    size_t missedCuesSize = missedCues.size();
    size_t previousCuesSize = previousCues.size();

    // 6 - If all of the cues in current cues have their text track cue active
    // flag set, none of the cues in other cues have their text track cue active
    // flag set, and missed cues is empty, then abort these steps.
    bool activeSetChanged = missedCuesSize;

    for (size_t i = 0; !activeSetChanged && i < previousCuesSize; ++i)
        if (!currentCues.contains(previousCues[i]) && previousCues[i].data()->isActive())
            activeSetChanged = true;

    for (size_t i = 0; i < currentCuesSize; ++i) {
        TextTrackCue* cue = currentCues[i].data();

        if (cue->isRenderable())
            toVTTCue(cue)->updateDisplayTree(movieTime);

        if (!cue->isActive())
            activeSetChanged = true;
    }

    if (!activeSetChanged)
        return;

    // 7 - If the time was reached through the usual monotonic increase of the
    // current playback position during normal playback, and there are cues in
    // other cues that have their text track cue pause-on-exi flag set and that
    // either have their text track cue active flag set or are also in missed
    // cues, then immediately pause the media element.
    for (size_t i = 0; !m_paused && i < previousCuesSize; ++i) {
        if (previousCues[i].data()->pauseOnExit()
            && previousCues[i].data()->isActive()
            && !currentCues.contains(previousCues[i]))
            pause();
    }

    for (size_t i = 0; !m_paused && i < missedCuesSize; ++i) {
        if (missedCues[i].data()->pauseOnExit())
            pause();
    }

    // 8 - Let events be a list of tasks, initially empty. Each task in this
    // list will be associated with a text track, a text track cue, and a time,
    // which are used to sort the list before the tasks are queued.
    Vector<std::pair<MediaTime, TextTrackCue*>> eventTasks;

    // 8 - Let affected tracks be a list of text tracks, initially empty.
    Vector<TextTrack*> affectedTracks;

    for (size_t i = 0; i < missedCuesSize; ++i) {
        // 9 - For each text track cue in missed cues, prepare an event named enter
        // for the TextTrackCue object with the text track cue start time.
        eventTasks.append(std::make_pair(missedCues[i].data()->startMediaTime(),
                                         missedCues[i].data()));

        // 10 - For each text track [...] in missed cues, prepare an event
        // named exit for the TextTrackCue object with the  with the later of
        // the text track cue end time and the text track cue start time.

        // Note: An explicit task is added only if the cue is NOT a zero or
        // negative length cue. Otherwise, the need for an exit event is
        // checked when these tasks are actually queued below. This doesn't
        // affect sorting events before dispatch either, because the exit
        // event has the same time as the enter event.
        if (missedCues[i].data()->startMediaTime() < missedCues[i].data()->endMediaTime())
            eventTasks.append(std::make_pair(missedCues[i].data()->endMediaTime(), missedCues[i].data()));
    }

    for (size_t i = 0; i < previousCuesSize; ++i) {
        // 10 - For each text track cue in other cues that has its text
        // track cue active flag set prepare an event named exit for the
        // TextTrackCue object with the text track cue end time.
        if (!currentCues.contains(previousCues[i]))
            eventTasks.append(std::make_pair(previousCues[i].data()->endMediaTime(),
                                             previousCues[i].data()));
    }

    for (size_t i = 0; i < currentCuesSize; ++i) {
        // 11 - For each text track cue in current cues that does not have its
        // text track cue active flag set, prepare an event named enter for the
        // TextTrackCue object with the text track cue start time.
        if (!previousCues.contains(currentCues[i]))
            eventTasks.append(std::make_pair(currentCues[i].data()->startMediaTime(),
                                             currentCues[i].data()));
    }

    // 12 - Sort the tasks in events in ascending time order (tasks with earlier
    // times first).
    std::sort(eventTasks.begin(), eventTasks.end(), eventTimeCueCompare);

    for (size_t i = 0; i < eventTasks.size(); ++i) {
        if (!affectedTracks.contains(eventTasks[i].second->track()))
            affectedTracks.append(eventTasks[i].second->track());

        // 13 - Queue each task in events, in list order.
        RefPtr<Event> event;

        // Each event in eventTasks may be either an enterEvent or an exitEvent,
        // depending on the time that is associated with the event. This
        // correctly identifies the type of the event, if the startTime is
        // less than the endTime in the cue.
        if (eventTasks[i].second->startTime() >= eventTasks[i].second->endTime()) {
            event = Event::create(eventNames().enterEvent, false, false);
            event->setTarget(eventTasks[i].second);
            m_asyncEventQueue.enqueueEvent(event.release());

            event = Event::create(eventNames().exitEvent, false, false);
            event->setTarget(eventTasks[i].second);
            m_asyncEventQueue.enqueueEvent(event.release());
        } else {
            if (eventTasks[i].first == eventTasks[i].second->startMediaTime())
                event = Event::create(eventNames().enterEvent, false, false);
            else
                event = Event::create(eventNames().exitEvent, false, false);

            event->setTarget(eventTasks[i].second);
            m_asyncEventQueue.enqueueEvent(event.release());
        }
    }

    // 14 - Sort affected tracks in the same order as the text tracks appear in
    // the media element's list of text tracks, and remove duplicates.
    std::sort(affectedTracks.begin(), affectedTracks.end(), trackIndexCompare);

    // 15 - For each text track in affected tracks, in the list order, queue a
    // task to fire a simple event named cuechange at the TextTrack object, and, ...
    for (size_t i = 0; i < affectedTracks.size(); ++i) {
        RefPtr<Event> event = Event::create(eventNames().cuechangeEvent, false, false);
        event->setTarget(affectedTracks[i]);

        m_asyncEventQueue.enqueueEvent(event.release());

        // ... if the text track has a corresponding track element, to then fire a
        // simple event named cuechange at the track element as well.
        if (is<LoadableTextTrack>(*affectedTracks[i])) {
            RefPtr<Event> event = Event::create(eventNames().cuechangeEvent, false, false);
            HTMLTrackElement* trackElement = downcast<LoadableTextTrack>(*affectedTracks[i]).trackElement();
            ASSERT(trackElement);
            event->setTarget(trackElement);
            
            m_asyncEventQueue.enqueueEvent(event.release());
        }
    }

    // 16 - Set the text track cue active flag of all the cues in the current
    // cues, and unset the text track cue active flag of all the cues in the
    // other cues.
    for (size_t i = 0; i < currentCuesSize; ++i)
        currentCues[i].data()->setIsActive(true);

    for (size_t i = 0; i < previousCuesSize; ++i)
        if (!currentCues.contains(previousCues[i]))
            previousCues[i].data()->setIsActive(false);

    // Update the current active cues.
    m_currentlyActiveCues = currentCues;

    if (activeSetChanged)
        updateTextTrackDisplay();
}

bool HTMLMediaElement::textTracksAreReady() const
{
    // 4.8.10.12.1 Text track model
    // ...
    // The text tracks of a media element are ready if all the text tracks whose mode was not 
    // in the disabled state when the element's resource selection algorithm last started now
    // have a text track readiness state of loaded or failed to load.
    for (unsigned i = 0; i < m_textTracksWhenResourceSelectionBegan.size(); ++i) {
        if (m_textTracksWhenResourceSelectionBegan[i]->readinessState() == TextTrack::Loading
            || m_textTracksWhenResourceSelectionBegan[i]->readinessState() == TextTrack::NotLoaded)
            return false;
    }

    return true;
}

void HTMLMediaElement::textTrackReadyStateChanged(TextTrack* track)
{
    if (m_player && m_textTracksWhenResourceSelectionBegan.contains(track)) {
        if (track->readinessState() != TextTrack::Loading)
            setReadyState(m_player->readyState());
    } else {
        // The track readiness state might have changed as a result of the user
        // clicking the captions button. In this case, a check whether all the
        // resources have failed loading should be done in order to hide the CC button.
        if (hasMediaControls() && track->readinessState() == TextTrack::FailedToLoad)
            mediaControls()->refreshClosedCaptionsButtonVisibility();
    }
}

void HTMLMediaElement::audioTrackEnabledChanged(AudioTrack* track)
{
    if (!RuntimeEnabledFeatures::sharedFeatures().webkitVideoTrackEnabled())
        return;
    if (m_audioTracks && m_audioTracks->contains(track))
        m_audioTracks->scheduleChangeEvent();
}

void HTMLMediaElement::textTrackModeChanged(TextTrack* track)
{
    bool trackIsLoaded = true;
    if (track->trackType() == TextTrack::TrackElement) {
        trackIsLoaded = false;
        for (auto& trackElement : childrenOfType<HTMLTrackElement>(*this)) {
            if (trackElement.track() == track) {
                if (trackElement.readyState() == HTMLTrackElement::LOADING || trackElement.readyState() == HTMLTrackElement::LOADED)
                    trackIsLoaded = true;
                break;
            }
        }
    }

    // If this is the first added track, create the list of text tracks.
    if (!m_textTracks)
        m_textTracks = TextTrackList::create(this, ActiveDOMObject::scriptExecutionContext());
    
    // Mark this track as "configured" so configureTextTracks won't change the mode again.
    track->setHasBeenConfigured(true);
    
    if (track->mode() != TextTrack::disabledKeyword() && trackIsLoaded)
        textTrackAddCues(track, track->cues());

#if USE(PLATFORM_TEXT_TRACK_MENU)
    if (platformTextTrackMenu())
        platformTextTrackMenu()->trackWasSelected(track->platformTextTrack());
#endif
    
    configureTextTrackDisplay(AssumeTextTrackVisibilityChanged);

    if (m_textTracks && m_textTracks->contains(track))
        m_textTracks->scheduleChangeEvent();

#if ENABLE(AVF_CAPTIONS)
    if (track->trackType() == TextTrack::TrackElement && m_player)
        m_player->notifyTrackModeChanged();
#endif
}

void HTMLMediaElement::videoTrackSelectedChanged(VideoTrack* track)
{
    if (!RuntimeEnabledFeatures::sharedFeatures().webkitVideoTrackEnabled())
        return;
    if (m_videoTracks && m_videoTracks->contains(track))
        m_videoTracks->scheduleChangeEvent();
}

void HTMLMediaElement::textTrackKindChanged(TextTrack* track)
{
    if (track->kind() != TextTrack::captionsKeyword() && track->kind() != TextTrack::subtitlesKeyword() && track->mode() == TextTrack::showingKeyword())
        track->setMode(TextTrack::hiddenKeyword());
}

void HTMLMediaElement::beginIgnoringTrackDisplayUpdateRequests()
{
    ++m_ignoreTrackDisplayUpdate;
}

void HTMLMediaElement::endIgnoringTrackDisplayUpdateRequests()
{
    ASSERT(m_ignoreTrackDisplayUpdate);
    --m_ignoreTrackDisplayUpdate;
    if (!m_ignoreTrackDisplayUpdate && m_inActiveDocument)
        updateActiveTextTrackCues(currentMediaTime());
}

void HTMLMediaElement::textTrackAddCues(TextTrack* track, const TextTrackCueList* cues) 
{
    if (track->mode() == TextTrack::disabledKeyword())
        return;

    TrackDisplayUpdateScope scope(this);
    for (size_t i = 0; i < cues->length(); ++i)
        textTrackAddCue(track, cues->item(i));
}

void HTMLMediaElement::textTrackRemoveCues(TextTrack*, const TextTrackCueList* cues) 
{
    TrackDisplayUpdateScope scope(this);
    for (size_t i = 0; i < cues->length(); ++i)
        textTrackRemoveCue(cues->item(i)->track(), cues->item(i));
}

void HTMLMediaElement::textTrackAddCue(TextTrack* track, PassRefPtr<TextTrackCue> prpCue)
{
    if (track->mode() == TextTrack::disabledKeyword())
        return;

    RefPtr<TextTrackCue> cue = prpCue;

    // Negative duration cues need be treated in the interval tree as
    // zero-length cues.
    MediaTime endTime = std::max(cue->startMediaTime(), cue->endMediaTime());

    CueInterval interval = m_cueTree.createInterval(cue->startMediaTime(), endTime, cue.get());
    if (!m_cueTree.contains(interval))
        m_cueTree.add(interval);
    updateActiveTextTrackCues(currentMediaTime());
}

void HTMLMediaElement::textTrackRemoveCue(TextTrack*, PassRefPtr<TextTrackCue> prpCue)
{
    RefPtr<TextTrackCue> cue = prpCue;
    // Negative duration cues need to be treated in the interval tree as
    // zero-length cues.
    MediaTime endTime = std::max(cue->startMediaTime(), cue->endMediaTime());

    CueInterval interval = m_cueTree.createInterval(cue->startMediaTime(), endTime, cue.get());
    m_cueTree.remove(interval);

#if ENABLE(WEBVTT_REGIONS)
    // Since the cue will be removed from the media element and likely the
    // TextTrack might also be destructed, notifying the region of the cue
    // removal shouldn't be done.
    if (cue->isRenderable())
        toVTTCue(cue.get())->notifyRegionWhenRemovingDisplayTree(false);
#endif

    size_t index = m_currentlyActiveCues.find(interval);
    if (index != notFound) {
        cue->setIsActive(false);
        m_currentlyActiveCues.remove(index);
    }

    if (cue->isRenderable())
        toVTTCue(cue.get())->removeDisplayTree();
    updateActiveTextTrackCues(currentMediaTime());

#if ENABLE(WEBVTT_REGIONS)
    if (cue->isRenderable())
        toVTTCue(cue.get())->notifyRegionWhenRemovingDisplayTree(true);
#endif
}

#endif

bool HTMLMediaElement::isSafeToLoadURL(const URL& url, InvalidURLAction actionIfInvalid)
{
    if (!url.isValid()) {
        LOG(Media, "HTMLMediaElement::isSafeToLoadURL(%p) - %s -> FALSE because url is invalid", this, urlForLoggingMedia(url).utf8().data());
        return false;
    }

    Frame* frame = document().frame();
    if (!frame || !document().securityOrigin().canDisplay(url)) {
        if (actionIfInvalid == Complain)
            FrameLoader::reportLocalLoadFailed(frame, url.stringCenterEllipsizedToLength());
        LOG(Media, "HTMLMediaElement::isSafeToLoadURL(%p) - %s -> FALSE rejected by SecurityOrigin", this, urlForLoggingMedia(url).utf8().data());
        return false;
    }

    if (!document().contentSecurityPolicy()->allowMediaFromSource(url, isInUserAgentShadowTree())) {
        LOG(Media, "HTMLMediaElement::isSafeToLoadURL(%p) - %s -> rejected by Content Security Policy", this, urlForLoggingMedia(url).utf8().data());
        return false;
    }

    return true;
}

void HTMLMediaElement::startProgressEventTimer()
{
    if (m_progressEventTimer.isActive())
        return;

    m_previousProgressTime = monotonicallyIncreasingTime();
    // 350ms is not magic, it is in the spec!
    m_progressEventTimer.startRepeating(0.350);
}

void HTMLMediaElement::waitForSourceChange()
{
    LOG(Media, "HTMLMediaElement::waitForSourceChange(%p)", this);

    stopPeriodicTimers();
    m_loadState = WaitingForSource;

    // 6.17 - Waiting: Set the element's networkState attribute to the NETWORK_NO_SOURCE value
    m_networkState = NETWORK_NO_SOURCE;

    // 6.18 - Set the element's delaying-the-load-event flag to false. This stops delaying the load event.
    setShouldDelayLoadEvent(false);

    updateDisplayState();

    if (renderer())
        renderer()->updateFromElement();
}

void HTMLMediaElement::noneSupported()
{
    LOG(Media, "HTMLMediaElement::noneSupported(%p)", this);

    stopPeriodicTimers();
    m_loadState = WaitingForSource;
    m_currentSourceNode = nullptr;

    // 4.8.10.5 
    // 6 - Reaching this step indicates that the media resource failed to load or that the given 
    // URL could not be resolved. In one atomic operation, run the following steps:

    // 6.1 - Set the error attribute to a new MediaError object whose code attribute is set to
    // MEDIA_ERR_SRC_NOT_SUPPORTED.
    m_error = MediaError::create(MediaError::MEDIA_ERR_SRC_NOT_SUPPORTED);

    // 6.2 - Forget the media element's media-resource-specific text tracks.
    forgetResourceSpecificTracks();

    // 6.3 - Set the element's networkState attribute to the NETWORK_NO_SOURCE value.
    m_networkState = NETWORK_NO_SOURCE;

    // 7 - Queue a task to fire a simple event named error at the media element.
    scheduleEvent(eventNames().errorEvent);

#if ENABLE(MEDIA_SOURCE)
    closeMediaSource();
#endif

    // 8 - Set the element's delaying-the-load-event flag to false. This stops delaying the load event.
    setShouldDelayLoadEvent(false);

    // 9 - Abort these steps. Until the load() method is invoked or the src attribute is changed, 
    // the element won't attempt to load another resource.

    updateDisplayState();

    if (renderer())
        renderer()->updateFromElement();
}

void HTMLMediaElement::mediaLoadingFailedFatally(MediaPlayer::NetworkState error)
{
    LOG(Media, "HTMLMediaElement::mediaLoadingFailedFatally(%p) - error = %d", this, static_cast<int>(error));

    // 1 - The user agent should cancel the fetching process.
    stopPeriodicTimers();
    m_loadState = WaitingForSource;

    // 2 - Set the error attribute to a new MediaError object whose code attribute is 
    // set to MEDIA_ERR_NETWORK/MEDIA_ERR_DECODE.
    if (error == MediaPlayer::NetworkError)
        m_error = MediaError::create(MediaError::MEDIA_ERR_NETWORK);
    else if (error == MediaPlayer::DecodeError)
        m_error = MediaError::create(MediaError::MEDIA_ERR_DECODE);
    else
        ASSERT_NOT_REACHED();

    // 3 - Queue a task to fire a simple event named error at the media element.
    scheduleEvent(eventNames().errorEvent);

#if ENABLE(MEDIA_SOURCE)
    closeMediaSource();
#endif

    // 4 - Set the element's networkState attribute to the NETWORK_EMPTY value and queue a
    // task to fire a simple event called emptied at the element.
    m_networkState = NETWORK_EMPTY;
    scheduleEvent(eventNames().emptiedEvent);

    // 5 - Set the element's delaying-the-load-event flag to false. This stops delaying the load event.
    setShouldDelayLoadEvent(false);

    // 6 - Abort the overall resource selection algorithm.
    m_currentSourceNode = nullptr;

#if PLATFORM(COCOA)
    if (is<MediaDocument>(document()))
        downcast<MediaDocument>(document()).mediaElementSawUnsupportedTracks();
#endif
}

void HTMLMediaElement::cancelPendingEventsAndCallbacks()
{
    LOG(Media, "HTMLMediaElement::cancelPendingEventsAndCallbacks(%p)", this);
    m_asyncEventQueue.cancelAllEvents();

    for (auto& source : childrenOfType<HTMLSourceElement>(*this))
        source.cancelPendingErrorEvent();
}

void HTMLMediaElement::mediaPlayerNetworkStateChanged(MediaPlayer*)
{
    beginProcessingMediaPlayerCallback();
    setNetworkState(m_player->networkState());
    endProcessingMediaPlayerCallback();
}

static void logMediaLoadRequest(Page* page, const String& mediaEngine, const String& errorMessage, bool succeeded)
{
    if (!page)
        return;

    DiagnosticLoggingClient& diagnosticLoggingClient = page->mainFrame().diagnosticLoggingClient();
    if (!succeeded) {
        diagnosticLoggingClient.logDiagnosticMessageWithResult(DiagnosticLoggingKeys::mediaLoadingFailedKey(), errorMessage, DiagnosticLoggingResultFail, ShouldSample::No);
        return;
    }

    diagnosticLoggingClient.logDiagnosticMessage(DiagnosticLoggingKeys::mediaLoadedKey(), mediaEngine, ShouldSample::No);

    if (!page->hasSeenAnyMediaEngine())
        diagnosticLoggingClient.logDiagnosticMessage(DiagnosticLoggingKeys::pageContainsAtLeastOneMediaEngineKey(), emptyString(), ShouldSample::No);

    if (!page->hasSeenMediaEngine(mediaEngine))
        diagnosticLoggingClient.logDiagnosticMessage(DiagnosticLoggingKeys::pageContainsMediaEngineKey(), mediaEngine, ShouldSample::No);

    page->sawMediaEngine(mediaEngine);
}

static String stringForNetworkState(MediaPlayer::NetworkState state)
{
    switch (state) {
    case MediaPlayer::Empty: return ASCIILiteral("Empty");
    case MediaPlayer::Idle: return ASCIILiteral("Idle");
    case MediaPlayer::Loading: return ASCIILiteral("Loading");
    case MediaPlayer::Loaded: return ASCIILiteral("Loaded");
    case MediaPlayer::FormatError: return ASCIILiteral("FormatError");
    case MediaPlayer::NetworkError: return ASCIILiteral("NetworkError");
    case MediaPlayer::DecodeError: return ASCIILiteral("DecodeError");
    default: return emptyString();
    }
}

void HTMLMediaElement::mediaLoadingFailed(MediaPlayer::NetworkState error)
{
    stopPeriodicTimers();
    
    // If we failed while trying to load a <source> element, the movie was never parsed, and there are more
    // <source> children, schedule the next one
    if (m_readyState < HAVE_METADATA && m_loadState == LoadingFromSourceElement) {
        
        // resource selection algorithm
        // Step 9.Otherwise.9 - Failed with elements: Queue a task, using the DOM manipulation task source, to fire a simple event named error at the candidate element.
        if (m_currentSourceNode)
            m_currentSourceNode->scheduleErrorEvent();
        else
            LOG(Media, "HTMLMediaElement::setNetworkState(%p) - error event not sent, <source> was removed", this);
        
        // 9.Otherwise.10 - Asynchronously await a stable state. The synchronous section consists of all the remaining steps of this algorithm until the algorithm says the synchronous section has ended.
        
        // 9.Otherwise.11 - Forget the media element's media-resource-specific tracks.
        forgetResourceSpecificTracks();

        if (havePotentialSourceChild()) {
            LOG(Media, "HTMLMediaElement::setNetworkState(%p) - scheduling next <source>", this);
            scheduleNextSourceChild();
        } else {
            LOG(Media, "HTMLMediaElement::setNetworkState(%p) - no more <source> elements, waiting", this);
            waitForSourceChange();
        }
        
        return;
    }
    
    if ((error == MediaPlayer::NetworkError && m_readyState >= HAVE_METADATA) || error == MediaPlayer::DecodeError)
        mediaLoadingFailedFatally(error);
    else if ((error == MediaPlayer::FormatError || error == MediaPlayer::NetworkError) && m_loadState == LoadingFromSrcAttr)
        noneSupported();
    
    updateDisplayState();
    if (hasMediaControls()) {
        mediaControls()->reset();
        mediaControls()->reportedError();
    }

    logMediaLoadRequest(document().page(), String(), stringForNetworkState(error), false);
}

void HTMLMediaElement::setNetworkState(MediaPlayer::NetworkState state)
{
    LOG(Media, "HTMLMediaElement::setNetworkState(%p) - new state = %d, current state = %d", this, static_cast<int>(state), static_cast<int>(m_networkState));

    if (state == MediaPlayer::Empty) {
        // Just update the cached state and leave, we can't do anything.
        m_networkState = NETWORK_EMPTY;
        return;
    }

#if PLATFORM(WKC)
    if (state == MediaPlayer::FormatError)
        m_formatErrorSrc = m_currentSrc;
#endif

    if (state == MediaPlayer::FormatError || state == MediaPlayer::NetworkError || state == MediaPlayer::DecodeError) {
        mediaLoadingFailed(state);
        return;
    }

    if (state == MediaPlayer::Idle) {
        if (m_networkState > NETWORK_IDLE) {
            changeNetworkStateFromLoadingToIdle();
            setShouldDelayLoadEvent(false);
        } else {
            m_networkState = NETWORK_IDLE;
        }
    }

    if (state == MediaPlayer::Loading) {
        if (m_networkState < NETWORK_LOADING || m_networkState == NETWORK_NO_SOURCE)
            startProgressEventTimer();
        m_networkState = NETWORK_LOADING;
    }

    if (state == MediaPlayer::Loaded) {
        if (m_networkState != NETWORK_IDLE)
            changeNetworkStateFromLoadingToIdle();
        m_completelyLoaded = true;
    }

    if (hasMediaControls())
        mediaControls()->updateStatusDisplay();
}

void HTMLMediaElement::changeNetworkStateFromLoadingToIdle()
{
    m_progressEventTimer.stop();
    if (hasMediaControls() && m_player->didLoadingProgress())
        mediaControls()->bufferingProgressed();

    // Schedule one last progress event so we guarantee that at least one is fired
    // for files that load very quickly.
    scheduleEvent(eventNames().progressEvent);
    scheduleEvent(eventNames().suspendEvent);
    m_networkState = NETWORK_IDLE;
}

void HTMLMediaElement::mediaPlayerReadyStateChanged(MediaPlayer*)
{
    beginProcessingMediaPlayerCallback();

    setReadyState(m_player->readyState());

    endProcessingMediaPlayerCallback();
}

static bool elementCanTransitionFromAutoplayToPlay(HTMLMediaElement& element)
{
    return element.isAutoplaying()
        && element.paused()
        && element.autoplay()
        && !element.document().isSandboxed(SandboxAutomaticFeatures)
        && element.mediaSession().playbackPermitted(element);
}

void HTMLMediaElement::setReadyState(MediaPlayer::ReadyState state)
{
    LOG(Media, "HTMLMediaElement::setReadyState(%p) - new state = %d, current state = %d,", this, static_cast<int>(state), static_cast<int>(m_readyState));

    // Set "wasPotentiallyPlaying" BEFORE updating m_readyState, potentiallyPlaying() uses it
    bool wasPotentiallyPlaying = potentiallyPlaying();

    ReadyState oldState = m_readyState;
    ReadyState newState = static_cast<ReadyState>(state);

#if ENABLE(VIDEO_TRACK)
    bool tracksAreReady = !RuntimeEnabledFeatures::sharedFeatures().webkitVideoTrackEnabled() || textTracksAreReady();

    if (newState == oldState && m_tracksAreReady == tracksAreReady)
        return;

    m_tracksAreReady = tracksAreReady;
#else
    if (newState == oldState)
        return;
    bool tracksAreReady = true;
#endif
    
    if (tracksAreReady)
        m_readyState = newState;
    else {
        // If a media file has text tracks the readyState may not progress beyond HAVE_FUTURE_DATA until
        // the text tracks are ready, regardless of the state of the media file.
        if (newState <= HAVE_METADATA)
            m_readyState = newState;
        else
            m_readyState = HAVE_CURRENT_DATA;
    }
    
    if (oldState > m_readyStateMaximum)
        m_readyStateMaximum = oldState;

    if (m_networkState == NETWORK_EMPTY)
        return;

    if (m_seeking) {
        // 4.8.10.9, step 11
        if (wasPotentiallyPlaying && m_readyState < HAVE_FUTURE_DATA)
            scheduleEvent(eventNames().waitingEvent);

        // 4.8.10.10 step 14 & 15.
        if (!m_player->seeking() && m_readyState >= HAVE_CURRENT_DATA)
            finishSeek();
    } else {
        if (wasPotentiallyPlaying && m_readyState < HAVE_FUTURE_DATA) {
            // 4.8.10.8
            invalidateCachedTime();
            scheduleTimeupdateEvent(false);
            scheduleEvent(eventNames().waitingEvent);
        }
    }

    if (m_readyState >= HAVE_METADATA && oldState < HAVE_METADATA) {
        prepareMediaFragmentURI();
        scheduleEvent(eventNames().durationchangeEvent);
        scheduleEvent(eventNames().loadedmetadataEvent);
#if ENABLE(WIRELESS_PLAYBACK_TARGET)
        if (hasEventListeners(eventNames().webkitplaybacktargetavailabilitychangedEvent))
            enqueuePlaybackTargetAvailabilityChangedEvent();
#endif
        m_initiallyMuted = m_volume < 0.05 || muted();

        if (hasMediaControls())
            mediaControls()->loadedMetadata();
        if (renderer())
            renderer()->updateFromElement();

        if (is<MediaDocument>(document()))
            downcast<MediaDocument>(document()).mediaElementNaturalSizeChanged(expandedIntSize(m_player->naturalSize()));

        logMediaLoadRequest(document().page(), m_player->engineDescription(), String(), true);

#if ENABLE(WIRELESS_PLAYBACK_TARGET)
        updateMediaState(UpdateMediaState::Asynchronously);
#endif
    }

    bool shouldUpdateDisplayState = false;

    if (m_readyState >= HAVE_CURRENT_DATA && oldState < HAVE_CURRENT_DATA && !m_haveFiredLoadedData) {
        m_haveFiredLoadedData = true;
        shouldUpdateDisplayState = true;
        scheduleEvent(eventNames().loadeddataEvent);
        setShouldDelayLoadEvent(false);
        applyMediaFragmentURI();
    }

    bool isPotentiallyPlaying = potentiallyPlaying();
    if (m_readyState == HAVE_FUTURE_DATA && oldState <= HAVE_CURRENT_DATA && tracksAreReady) {
        scheduleEvent(eventNames().canplayEvent);
        if (isPotentiallyPlaying)
            scheduleEvent(eventNames().playingEvent);
        shouldUpdateDisplayState = true;
    }

    if (m_readyState == HAVE_ENOUGH_DATA && oldState < HAVE_ENOUGH_DATA && tracksAreReady) {
        if (oldState <= HAVE_CURRENT_DATA)
            scheduleEvent(eventNames().canplayEvent);

        scheduleEvent(eventNames().canplaythroughEvent);

        if (isPotentiallyPlaying && oldState <= HAVE_CURRENT_DATA)
            scheduleEvent(eventNames().playingEvent);

        if (m_readyState == HAVE_ENOUGH_DATA && elementCanTransitionFromAutoplayToPlay(*this)) {
            m_paused = false;
            invalidateCachedTime();
            scheduleEvent(eventNames().playEvent);
            scheduleEvent(eventNames().playingEvent);
        }

        shouldUpdateDisplayState = true;
    }

    if (shouldUpdateDisplayState) {
        updateDisplayState();
        if (hasMediaControls()) {
            mediaControls()->refreshClosedCaptionsButtonVisibility();
            mediaControls()->updateStatusDisplay();
        }
    }

    updatePlayState();
    updateMediaController();
#if ENABLE(VIDEO_TRACK)
    if (RuntimeEnabledFeatures::sharedFeatures().webkitVideoTrackEnabled())
        updateActiveTextTrackCues(currentMediaTime());
#endif
}

#if ENABLE(ENCRYPTED_MEDIA)
void HTMLMediaElement::mediaPlayerKeyAdded(MediaPlayer*, const String& keySystem, const String& sessionId)
{
    MediaKeyEventInit initializer;
    initializer.keySystem = keySystem;
    initializer.sessionId = sessionId;
    initializer.bubbles = false;
    initializer.cancelable = false;

    RefPtr<Event> event = MediaKeyEvent::create(eventNames().keyaddedEvent, initializer);
    event->setTarget(this);
    m_asyncEventQueue.enqueueEvent(event.release());
}

void HTMLMediaElement::mediaPlayerKeyError(MediaPlayer*, const String& keySystem, const String& sessionId, MediaPlayerClient::MediaKeyErrorCode errorCode, unsigned short systemCode)
{
    MediaKeyError::Code mediaKeyErrorCode = MediaKeyError::MEDIA_KEYERR_UNKNOWN;
    switch (errorCode) {
    case MediaPlayerClient::UnknownError:
        mediaKeyErrorCode = MediaKeyError::MEDIA_KEYERR_UNKNOWN;
        break;
    case MediaPlayerClient::ClientError:
        mediaKeyErrorCode = MediaKeyError::MEDIA_KEYERR_CLIENT;
        break;
    case MediaPlayerClient::ServiceError:
        mediaKeyErrorCode = MediaKeyError::MEDIA_KEYERR_SERVICE;
        break;
    case MediaPlayerClient::OutputError:
        mediaKeyErrorCode = MediaKeyError::MEDIA_KEYERR_OUTPUT;
        break;
    case MediaPlayerClient::HardwareChangeError:
        mediaKeyErrorCode = MediaKeyError::MEDIA_KEYERR_HARDWARECHANGE;
        break;
    case MediaPlayerClient::DomainError:
        mediaKeyErrorCode = MediaKeyError::MEDIA_KEYERR_DOMAIN;
        break;
    }

    MediaKeyEventInit initializer;
    initializer.keySystem = keySystem;
    initializer.sessionId = sessionId;
    initializer.errorCode = MediaKeyError::create(mediaKeyErrorCode);
    initializer.systemCode = systemCode;
    initializer.bubbles = false;
    initializer.cancelable = false;

    RefPtr<Event> event = MediaKeyEvent::create(eventNames().keyerrorEvent, initializer);
    event->setTarget(this);
    m_asyncEventQueue.enqueueEvent(event.release());
}

void HTMLMediaElement::mediaPlayerKeyMessage(MediaPlayer*, const String& keySystem, const String& sessionId, const unsigned char* message, unsigned messageLength, const URL& defaultURL)
{
    MediaKeyEventInit initializer;
    initializer.keySystem = keySystem;
    initializer.sessionId = sessionId;
    initializer.message = Uint8Array::create(message, messageLength);
    initializer.defaultURL = defaultURL; 
    initializer.bubbles = false;
    initializer.cancelable = false;

    RefPtr<Event> event = MediaKeyEvent::create(eventNames().keymessageEvent, initializer);
    event->setTarget(this);
    m_asyncEventQueue.enqueueEvent(event.release());
}

bool HTMLMediaElement::mediaPlayerKeyNeeded(MediaPlayer*, const String& keySystem, const String& sessionId, const unsigned char* initData, unsigned initDataLength)
{
    if (!hasEventListeners(eventNames().needkeyEvent)) {
        m_error = MediaError::create(MediaError::MEDIA_ERR_ENCRYPTED);
        scheduleEvent(eventNames().errorEvent);
        return false;
    }

    MediaKeyEventInit initializer;
    initializer.keySystem = keySystem;
    initializer.sessionId = sessionId;
    initializer.initData = Uint8Array::create(initData, initDataLength);
    initializer.bubbles = false;
    initializer.cancelable = false;

    RefPtr<Event> event = MediaKeyEvent::create(eventNames().needkeyEvent, initializer);
    event->setTarget(this);
    m_asyncEventQueue.enqueueEvent(event.release());
    return true;
}
#endif

#if ENABLE(ENCRYPTED_MEDIA_V2)
RefPtr<ArrayBuffer> HTMLMediaElement::mediaPlayerCachedKeyForKeyId(const String& keyId) const
{
    return m_mediaKeys ? m_mediaKeys->cachedKeyForKeyId(keyId) : nullptr;
}

bool HTMLMediaElement::mediaPlayerKeyNeeded(MediaPlayer*, Uint8Array* initData)
{
    if (!hasEventListeners("needkey")) {
        m_error = MediaError::create(MediaError::MEDIA_ERR_ENCRYPTED);
        scheduleEvent(eventNames().errorEvent);
        return false;
    }

    MediaKeyNeededEventInit initializer;
    initializer.initData = initData;
    initializer.bubbles = false;
    initializer.cancelable = false;

    RefPtr<Event> event = MediaKeyNeededEvent::create(eventNames().needkeyEvent, initializer);
    event->setTarget(this);
    m_asyncEventQueue.enqueueEvent(event.release());

    return true;
}

String HTMLMediaElement::mediaPlayerMediaKeysStorageDirectory() const
{
    Settings* settings = document().settings();
    if (!settings)
        return emptyString();

    String storageDirectory = settings->mediaKeysStorageDirectory();
    if (storageDirectory.isEmpty())
        return emptyString();

    return pathByAppendingComponent(storageDirectory, document().securityOrigin()->databaseIdentifier());
}

void HTMLMediaElement::setMediaKeys(MediaKeys* mediaKeys)
{
    if (m_mediaKeys == mediaKeys)
        return;

    if (m_mediaKeys)
        m_mediaKeys->setMediaElement(0);
    m_mediaKeys = mediaKeys;
    if (m_mediaKeys)
        m_mediaKeys->setMediaElement(this);
}

void HTMLMediaElement::keyAdded()
{
    if (m_player)
        m_player->keyAdded();
}
#endif

void HTMLMediaElement::progressEventTimerFired()
{
    ASSERT(m_player);
    if (m_networkState != NETWORK_LOADING)
        return;

    double time = monotonicallyIncreasingTime();
    double timedelta = time - m_previousProgressTime;

    if (m_player->didLoadingProgress()) {
        scheduleEvent(eventNames().progressEvent);
        m_previousProgressTime = time;
        m_sentStalledEvent = false;
        if (renderer())
            renderer()->updateFromElement();
        if (hasMediaControls())
            mediaControls()->bufferingProgressed();
    } else if (timedelta > 3.0 && !m_sentStalledEvent) {
        scheduleEvent(eventNames().stalledEvent);
        m_sentStalledEvent = true;
        setShouldDelayLoadEvent(false);
    }
}

void HTMLMediaElement::rewind(double timeDelta)
{
    LOG(Media, "HTMLMediaElement::rewind(%p) - %f", this, timeDelta);
    setCurrentTime(std::max(currentMediaTime() - MediaTime::createWithDouble(timeDelta), minTimeSeekable()));
}

void HTMLMediaElement::returnToRealtime()
{
    LOG(Media, "HTMLMediaElement::returnToRealtime(%p)", this);
    setCurrentTime(maxTimeSeekable());
}

void HTMLMediaElement::addPlayedRange(const MediaTime& start, const MediaTime& end)
{
    LOG(Media, "HTMLMediaElement::addPlayedRange(%p) - [%s, %s]", this, toString(start).utf8().data(), toString(end).utf8().data());
    if (!m_playedTimeRanges)
        m_playedTimeRanges = TimeRanges::create();
    m_playedTimeRanges->ranges().add(start, end);
}  

bool HTMLMediaElement::supportsScanning() const
{
    return m_player ? m_player->supportsScanning() : false;
}

void HTMLMediaElement::prepareToPlay()
{
    LOG(Media, "HTMLMediaElement::prepareToPlay(%p)", this);
#if !PLATFORM(WKC)
    if (m_havePreparedToPlay)
        return;
#endif
    m_havePreparedToPlay = true;
    m_player->prepareToPlay();
}

void HTMLMediaElement::fastSeek(double time)
{
    fastSeek(MediaTime::createWithDouble(time));
}

void HTMLMediaElement::fastSeek(const MediaTime& time)
{
    LOG(Media, "HTMLMediaElement::fastSeek(%p) - %s", this, toString(time).utf8().data());
    // 4.7.10.9 Seeking
    // 9. If the approximate-for-speed flag is set, adjust the new playback position to a value that will
    // allow for playback to resume promptly. If new playback position before this step is before current
    // playback position, then the adjusted new playback position must also be before the current playback
    // position. Similarly, if the new playback position before this step is after current playback position,
    // then the adjusted new playback position must also be after the current playback position.
    refreshCachedTime();
    MediaTime delta = time - currentMediaTime();
    MediaTime negativeTolerance = delta >= MediaTime::zeroTime() ? delta : MediaTime::positiveInfiniteTime();
    MediaTime positiveTolerance = delta < MediaTime::zeroTime() ? -delta : MediaTime::positiveInfiniteTime();

    seekWithTolerance(time, negativeTolerance, positiveTolerance, true);
}

void HTMLMediaElement::seek(const MediaTime& time)
{
    LOG(Media, "HTMLMediaElement::seek(%p) - %s", this, toString(time).utf8().data());
    seekWithTolerance(time, MediaTime::zeroTime(), MediaTime::zeroTime(), true);
}

void HTMLMediaElement::seekInternal(const MediaTime& time)
{
    LOG(Media, "HTMLMediaElement::seekInternal(%p) - %s", this, toString(time).utf8().data());
    seekWithTolerance(time, MediaTime::zeroTime(), MediaTime::zeroTime(), false);
}

void HTMLMediaElement::seekWithTolerance(const MediaTime& inTime, const MediaTime& negativeTolerance, const MediaTime& positiveTolerance, bool fromDOM)
{
    // 4.8.10.9 Seeking
    MediaTime time = inTime;

    // 1 - Set the media element's show poster flag to false.
    setDisplayMode(Video);

    // 2 - If the media element's readyState is HAVE_NOTHING, abort these steps.
    if (m_readyState == HAVE_NOTHING || !m_player)
        return;

    // If the media engine has been told to postpone loading data, let it go ahead now.
    if (m_preload < MediaPlayer::Auto && m_readyState < HAVE_FUTURE_DATA)
        prepareToPlay();

    // Get the current time before setting m_seeking, m_lastSeekTime is returned once it is set.
    refreshCachedTime();
    MediaTime now = currentMediaTime();

    // 3 - If the element's seeking IDL attribute is true, then another instance of this algorithm is
    // already running. Abort that other instance of the algorithm without waiting for the step that
    // it is running to complete.
    if (m_seekTaskQueue.hasPendingTasks()) {
        LOG(Media, "HTMLMediaElement::seekWithTolerance(%p) - cancelling pending seeks", this);
        m_seekTaskQueue.cancelAllTasks();
        if (m_pendingSeek) {
            now = m_pendingSeek->now;
            m_pendingSeek = nullptr;
        }
        m_pendingSeekType = NoSeek;
    }

    // 4 - Set the seeking IDL attribute to true.
    // The flag will be cleared when the engine tells us the time has actually changed.
    m_seeking = true;
    if (m_playing) {
        if (m_lastSeekTime < now)
            addPlayedRange(m_lastSeekTime, now);
    }
    m_lastSeekTime = time;

    // 5 - If the seek was in response to a DOM method call or setting of an IDL attribute, then continue
    // the script. The remainder of these steps must be run asynchronously.
    m_pendingSeek = std::make_unique<PendingSeek>(now, time, negativeTolerance, positiveTolerance);
    if (fromDOM) {
        LOG(Media, "HTMLMediaElement::seekWithTolerance(%p) - enqueuing seek from %s to %s", this, toString(now).utf8().data(), toString(time).utf8().data());
        m_seekTaskQueue.enqueueTask(std::bind(&HTMLMediaElement::seekTask, this));
    } else
        seekTask();
}

void HTMLMediaElement::seekTask()
{
    LOG(Media, "HTMLMediaElement::seekTask(%p)", this);

    if (!m_player) {
        clearSeeking();
        return;
    }

    ASSERT(m_pendingSeek);
    MediaTime now = m_pendingSeek->now;
    MediaTime time = m_pendingSeek->targetTime;
    MediaTime negativeTolerance = m_pendingSeek->negativeTolerance;
    MediaTime positiveTolerance = m_pendingSeek->positiveTolerance;
    m_pendingSeek = nullptr;

    // 6 - If the new playback position is later than the end of the media resource, then let it be the end 
    // of the media resource instead.
    time = std::min(time, durationMediaTime());

    // 7 - If the new playback position is less than the earliest possible position, let it be that position instead.
    MediaTime earliestTime = m_player->startTime();
    time = std::max(time, earliestTime);

    // Ask the media engine for the time value in the movie's time scale before comparing with current time. This
    // is necessary because if the seek time is not equal to currentTime but the delta is less than the movie's
    // time scale, we will ask the media engine to "seek" to the current movie time, which may be a noop and
    // not generate a timechanged callback. This means m_seeking will never be cleared and we will never 
    // fire a 'seeked' event.
#if !LOG_DISABLED
    MediaTime mediaTime = m_player->mediaTimeForTimeValue(time);
    if (time != mediaTime)
        LOG(Media, "HTMLMediaElement::seekTask(%p) - %s - media timeline equivalent is %s", this, toString(time).utf8().data(), toString(mediaTime).utf8().data());
#endif
    time = m_player->mediaTimeForTimeValue(time);

    // 8 - If the (possibly now changed) new playback position is not in one of the ranges given in the
    // seekable attribute, then let it be the position in one of the ranges given in the seekable attribute 
    // that is the nearest to the new playback position. ... If there are no ranges given in the seekable
    // attribute then set the seeking IDL attribute to false and abort these steps.
    RefPtr<TimeRanges> seekableRanges = seekable();
    bool noSeekRequired = !seekableRanges->length();

    // Short circuit seeking to the current time by just firing the events if no seek is required.
    // Don't skip calling the media engine if 1) we are in poster mode (because a seek should always cancel
    // poster display), or 2) if there is a pending fast seek, or 3) if this seek is not an exact seek
    SeekType thisSeekType = (negativeTolerance == MediaTime::zeroTime() && positiveTolerance == MediaTime::zeroTime()) ? Precise : Fast;
    if (!noSeekRequired && time == now && thisSeekType == Precise && m_pendingSeekType != Fast && displayMode() != Poster)
        noSeekRequired = true;

#if ENABLE(MEDIA_SOURCE)
    // Always notify the media engine of a seek if the source is not closed. This ensures that the source is
    // always in a flushed state when the 'seeking' event fires.
    if (m_mediaSource && !m_mediaSource->isClosed())
        noSeekRequired = false;
#endif

    if (noSeekRequired) {
        LOG(Media, "HTMLMediaElement::seekTask(%p) - seek to %s ignored", this, toString(time).utf8().data());
        if (time == now) {
            scheduleEvent(eventNames().seekingEvent);
            scheduleTimeupdateEvent(false);
            scheduleEvent(eventNames().seekedEvent);
        }
        clearSeeking();
        return;
    }
    time = seekableRanges->ranges().nearest(time);

    m_sentEndEvent = false;
    m_lastSeekTime = time;
    m_pendingSeekType = thisSeekType;
    m_seeking = true;

    // 10 - Queue a task to fire a simple event named seeking at the element.
    scheduleEvent(eventNames().seekingEvent);

    // 11 - Set the current playback position to the given new playback position
    m_player->seekWithTolerance(time, negativeTolerance, positiveTolerance);

    // 12 - Wait until the user agent has established whether or not the media data for the new playback
    // position is available, and, if it is, until it has decoded enough data to play back that position.
    // 13 - Await a stable state. The synchronous section consists of all the remaining steps of this algorithm.
}

void HTMLMediaElement::clearSeeking()
{
    m_seeking = false;
    m_pendingSeekType = NoSeek;
    invalidateCachedTime();
}

#if PLATFORM(WKC)
void HTMLMediaElement::seekBegin()
{
    LOG(Media, "HTMLMediaElement::seekBegin");

    // 4.8.10.9 Seeking

    // Get the current time before setting m_seeking, m_lastSeekTime is returned once it is set.
    refreshCachedTime();
    MediaTime now = MediaTime::createWithDouble(currentTime());

    // 3 - Set the seeking IDL attribute to true.
    // The flag will be cleared when the engine tells us the time has actually changed.
    m_seeking = true;

    if (m_playing) {
        if (m_lastSeekTime < now)
            addPlayedRange(m_lastSeekTime, now);
    }
    m_sentEndEvent = false;

    // 8 - Queue a task to fire a simple event named seeking at the element.
    scheduleEvent(eventNames().seekingEvent);
}

void HTMLMediaElement::seekEnd()
{
    LOG(Media, "HTMLMediaElement::seekEnd");

    if (!m_seeking)
        return;

    // 4.8.10.9 Seeking step 12
    m_seeking = false;

    refreshCachedTime();
    m_lastSeekTime = MediaTime::createWithDouble(currentTime());

    // 4.8.10.9 Seeking step 13
    scheduleTimeupdateEvent(false);

    // 4.8.10.9 Seeking step 14
    scheduleEvent(eventNames().seekedEvent);
}
#endif

void HTMLMediaElement::finishSeek()
{
    // 4.8.10.9 Seeking
    // 14 - Set the seeking IDL attribute to false.
    clearSeeking();

    LOG(Media, "HTMLMediaElement::finishSeek(%p) - current time = %s", this, toString(currentMediaTime()).utf8().data());

    // 15 - Run the time maches on steps.
    // Handled by mediaPlayerTimeChanged().

    // 16 - Queue a task to fire a simple event named timeupdate at the element.
    scheduleEvent(eventNames().timeupdateEvent);

    // 17 - Queue a task to fire a simple event named seeked at the element.
    scheduleEvent(eventNames().seekedEvent);

    setDisplayMode(Video);

#if ENABLE(MEDIA_SOURCE)
    if (m_mediaSource)
        m_mediaSource->monitorSourceBuffers();
#endif
}

HTMLMediaElement::ReadyState HTMLMediaElement::readyState() const
{
    return m_readyState;
}

MediaPlayer::MovieLoadType HTMLMediaElement::movieLoadType() const
{
    return m_player ? m_player->movieLoadType() : MediaPlayer::Unknown;
}

bool HTMLMediaElement::hasAudio() const
{
    return m_player ? m_player->hasAudio() : false;
}

bool HTMLMediaElement::seeking() const
{
    return m_seeking;
}

void HTMLMediaElement::refreshCachedTime() const
{
    if (!m_player)
        return;

    m_cachedTime = m_player->currentTime();
    if (!m_cachedTime) {
        // Do not use m_cachedTime until the media engine returns a non-zero value because we can't 
        // estimate current time until playback actually begins. 
        invalidateCachedTime(); 
        return; 
    } 

    m_clockTimeAtLastCachedTimeUpdate = monotonicallyIncreasingTime();
}

void HTMLMediaElement::invalidateCachedTime() const
{
    if (!m_player || !m_player->maximumDurationToCacheMediaTime())
        return;

#if !LOG_DISABLED
    if (m_cachedTime.isValid())
        LOG(Media, "HTMLMediaElement::invalidateCachedTime(%p)", this);
#endif

    // Don't try to cache movie time when playback first starts as the time reported by the engine
    // sometimes fluctuates for a short amount of time, so the cached time will be off if we take it
    // too early.
    static const double minimumTimePlayingBeforeCacheSnapshot = 0.5;

    m_minimumClockTimeToUpdateCachedTime = monotonicallyIncreasingTime() + minimumTimePlayingBeforeCacheSnapshot;
    m_cachedTime = MediaTime::invalidTime();
}

// playback state
double HTMLMediaElement::currentTime() const
{
    return currentMediaTime().toDouble();
}

MediaTime HTMLMediaElement::currentMediaTime() const
{
#if LOG_CACHED_TIME_WARNINGS
    static const MediaTime minCachedDeltaForWarning = MediaTime::create(1, 100);
#endif

    if (!m_player)
        return MediaTime::zeroTime();

    if (m_seeking) {
        LOG(Media, "HTMLMediaElement::currentTime(%p) - seeking, returning %s", this, toString(m_lastSeekTime).utf8().data());
        return m_lastSeekTime;
    }

    if (m_cachedTime.isValid() && m_paused) {
#if LOG_CACHED_TIME_WARNINGS
        MediaTime delta = m_cachedTime - m_player->currentTime();
        if (delta > minCachedDeltaForWarning)
            LOG(Media, "HTMLMediaElement::currentTime(%p) - WARNING, cached time is %s seconds off of media time when paused", this, toString(delta).utf8().data());
#endif
        return m_cachedTime;
    }

    // Is it too soon use a cached time?
    double now = monotonicallyIncreasingTime();
    double maximumDurationToCacheMediaTime = m_player->maximumDurationToCacheMediaTime();

    if (maximumDurationToCacheMediaTime && m_cachedTime.isValid() && !m_paused && now > m_minimumClockTimeToUpdateCachedTime) {
        double clockDelta = now - m_clockTimeAtLastCachedTimeUpdate;

        // Not too soon, use the cached time only if it hasn't expired.
        if (clockDelta < maximumDurationToCacheMediaTime) {
            MediaTime adjustedCacheTime = m_cachedTime + MediaTime::createWithDouble(effectivePlaybackRate() * clockDelta);

#if LOG_CACHED_TIME_WARNINGS
            MediaTime delta = adjustedCacheTime - m_player->currentTime();
            if (delta > minCachedDeltaForWarning)
                LOG(Media, "HTMLMediaElement::currentTime(%p) - WARNING, cached time is %f seconds off of media time when playing", this, delta);
#endif
            return adjustedCacheTime;
        }
    }

#if LOG_CACHED_TIME_WARNINGS
    if (maximumDurationToCacheMediaTime && now > m_minimumClockTimeToUpdateCachedTime && m_cachedTime != MediaPlayer::invalidTime()) {
        double clockDelta = now - m_clockTimeAtLastCachedTimeUpdate;
        MediaTime delta = m_cachedTime + MediaTime::createWithDouble(effectivePlaybackRate() * clockDelta) - m_player->currentTime();
        LOG(Media, "HTMLMediaElement::currentTime(%p) - cached time was %s seconds off of media time when it expired", this, toString(delta).utf8().data());
    }
#endif

    refreshCachedTime();

    if (m_cachedTime.isInvalid())
        return MediaTime::zeroTime();
    
    return m_cachedTime;
}

void HTMLMediaElement::setCurrentTime(double time)
{
    setCurrentTime(MediaTime::createWithDouble(time));
}

void HTMLMediaElement::setCurrentTime(const MediaTime& time)
{
    if (m_mediaController)
        return;

    seekInternal(time);
}

void HTMLMediaElement::setCurrentTime(double time, ExceptionCode& ec)
{
    // On setting, if the media element has a current media controller, then the user agent must
    // throw an InvalidStateError exception
    if (m_mediaController) {
        ec = INVALID_STATE_ERR;
        return;
    }

    seek(MediaTime::createWithDouble(time));
}

double HTMLMediaElement::duration() const
{
    return durationMediaTime().toDouble();
}

MediaTime HTMLMediaElement::durationMediaTime() const
{
    if (m_player && m_readyState >= HAVE_METADATA)
        return m_player->duration();

    return MediaTime::invalidTime();
}

bool HTMLMediaElement::paused() const
{
    // As of this writing, JavaScript garbage collection calls this function directly. In the past
    // we had problems where this was called on an object after a bad cast. The assertion below
    // made our regression test detect the problem, so we should keep it because of that. But note
    // that the value of the assertion relies on the compiler not being smart enough to know that
    // isHTMLUnknownElement is guaranteed to return false for an HTMLMediaElement.
    ASSERT(!isHTMLUnknownElement());

    return m_paused;
}

double HTMLMediaElement::defaultPlaybackRate() const
{
    return m_defaultPlaybackRate;
}

void HTMLMediaElement::setDefaultPlaybackRate(double rate)
{
    if (m_defaultPlaybackRate != rate) {
        LOG(Media, "HTMLMediaElement::setDefaultPlaybackRate(%p) - %f", this, rate);
        m_defaultPlaybackRate = rate;
        scheduleEvent(eventNames().ratechangeEvent);
    }
}

double HTMLMediaElement::effectivePlaybackRate() const
{
#if PLATFORM(WKC)
    return m_player ? m_player->rate() : 1;
#else
    return m_mediaController ? m_mediaController->playbackRate() : m_reportedPlaybackRate;
#endif
}

double HTMLMediaElement::requestedPlaybackRate() const
{
#if PLATFORM(WKC)
    return m_player ? m_player->rate() : 1;
#else
    return m_mediaController ? m_mediaController->playbackRate() : m_requestedPlaybackRate;
#endif
}

double HTMLMediaElement::playbackRate() const
{
#if PLATFORM(WKC)
    return m_player ? m_player->rate() : 1;
#else
    return m_requestedPlaybackRate;
#endif
}

void HTMLMediaElement::setPlaybackRate(double rate)
{
    LOG(Media, "HTMLMediaElement::setPlaybackRate(%p) - %f", this, rate);

#if PLATFORM(WKC)
    if (m_player && m_player->rate() != rate && !m_mediaController)
#else
    if (m_player && potentiallyPlaying() && m_player->rate() != rate && !m_mediaController)
#endif
        m_player->setRate(rate);

    if (m_requestedPlaybackRate != rate) {
        m_reportedPlaybackRate = m_requestedPlaybackRate = rate;
        invalidateCachedTime();
        scheduleEvent(eventNames().ratechangeEvent);
    }
}

void HTMLMediaElement::updatePlaybackRate()
{
    double requestedRate = requestedPlaybackRate();
    if (m_player && potentiallyPlaying() && m_player->rate() != requestedRate)
        m_player->setRate(requestedRate);
}

bool HTMLMediaElement::webkitPreservesPitch() const
{
    return m_webkitPreservesPitch;
}

void HTMLMediaElement::setWebkitPreservesPitch(bool preservesPitch)
{
    LOG(Media, "HTMLMediaElement::setWebkitPreservesPitch(%p) - %s", this, boolString(preservesPitch));

    m_webkitPreservesPitch = preservesPitch;
    
    if (!m_player)
        return;

    m_player->setPreservesPitch(preservesPitch);
}

bool HTMLMediaElement::ended() const
{
    // 4.8.10.8 Playing the media resource
    // The ended attribute must return true if the media element has ended 
    // playback and the direction of playback is forwards, and false otherwise.
    return endedPlayback() && requestedPlaybackRate() > 0;
}

bool HTMLMediaElement::autoplay() const
{
#if PLATFORM(WKC)
    if (!m_mediaSession->playbackPermitted(*this))
        return false;
#endif
    return fastHasAttribute(autoplayAttr);
}

String HTMLMediaElement::preload() const
{
    switch (m_preload) {
    case MediaPlayer::None:
        return ASCIILiteral("none");
    case MediaPlayer::MetaData:
        return ASCIILiteral("metadata");
    case MediaPlayer::Auto:
        return ASCIILiteral("auto");
    }

    ASSERT_NOT_REACHED();
    return String();
}

void HTMLMediaElement::setPreload(const String& preload)
{
    LOG(Media, "HTMLMediaElement::setPreload(%p) - %s", this, preload.utf8().data());
    setAttribute(preloadAttr, preload);
}

void HTMLMediaElement::play()
{
    LOG(Media, "HTMLMediaElement::play(%p)", this);

    if (!m_mediaSession->playbackPermitted(*this))
        return;
    if (ScriptController::processingUserGestureForMedia())
        removeBehaviorsRestrictionsAfterFirstUserGesture();

#if PLATFORM(WKC)
    m_playRequested = true;
#endif

    playInternal();
}

void HTMLMediaElement::playInternal()
{
    LOG(Media, "HTMLMediaElement::playInternal(%p)", this);
    
    if (!m_mediaSession->clientWillBeginPlayback()) {
        LOG(Media, "  returning because of interruption");
        return;
    }
    
    // 4.8.10.9. Playing the media resource
    if (!m_player || m_networkState == NETWORK_EMPTY)
        scheduleDelayedAction(LoadMediaResource);

    if (endedPlayback())
        seekInternal(MediaTime::zeroTime());

    if (m_mediaController)
        m_mediaController->bringElementUpToSpeed(this);

    if (m_paused) {
        m_paused = false;
        invalidateCachedTime();
        scheduleEvent(eventNames().playEvent);

        if (m_readyState <= HAVE_CURRENT_DATA)
            scheduleEvent(eventNames().waitingEvent);
        else if (m_readyState >= HAVE_FUTURE_DATA)
            scheduleEvent(eventNames().playingEvent);

#if ENABLE(MEDIA_SESSION)
        // 6.3 Activating a media session from a media element
        // When the play() method is invoked, the paused attribute is true, and the readyState attribute has the value
        // HAVE_FUTURE_DATA or HAVE_ENOUGH_DATA, then
        // 1. Let media session be the value of the current media session.
        // 2. If we are not currently in media session's list of active participating media elements then append
        //    ourselves to this list.
        // 3. Let activated be the result of running the media session invocation algorithm for media session.
        // 4. If activated is failure, pause ourselves.
        if (m_readyState == HAVE_ENOUGH_DATA || m_readyState == HAVE_FUTURE_DATA) {
            if (m_session) {
                m_session->addActiveMediaElement(*this);
                
                if (!m_session->invoke()) {
                    pause();
                    return;
                }
            }
        }
#endif
    }
    m_autoplaying = false;
    updatePlayState();
    updateMediaController();
}

void HTMLMediaElement::pause()
{
    LOG(Media, "HTMLMediaElement::pause(%p)", this);

    if (!m_mediaSession->playbackPermitted(*this))
        return;

    pauseInternal();
}


void HTMLMediaElement::pauseInternal()
{
    LOG(Media, "HTMLMediaElement::pauseInternal(%p)", this);

    if (!m_mediaSession->clientWillPausePlayback()) {
        LOG(Media, "  returning because of interruption");
        return;
    }
    
    // 4.8.10.9. Playing the media resource
    if (!m_player || m_networkState == NETWORK_EMPTY) {
        // Unless the restriction on media requiring user action has been lifted
        // don't trigger loading if a script calls pause().
        if (!m_mediaSession->playbackPermitted(*this))
            return;
        scheduleDelayedAction(LoadMediaResource);
    }

    m_autoplaying = false;

    if (!m_paused) {
        m_paused = true;
        scheduleTimeupdateEvent(false);
        scheduleEvent(eventNames().pauseEvent);

        if (MemoryPressureHandler::singleton().isUnderMemoryPressure())
            purgeBufferedDataIfPossible();
    }

    updatePlayState();
}

#if ENABLE(MEDIA_SOURCE)
void HTMLMediaElement::closeMediaSource()
{
    if (!m_mediaSource)
        return;

    m_mediaSource->close();
    m_mediaSource = nullptr;
}
#endif

#if ENABLE(ENCRYPTED_MEDIA)
void HTMLMediaElement::generateKeyRequest(const String& keySystem, PassRefPtr<Uint8Array> initData, ExceptionCode& ec)
{
#if ENABLE(ENCRYPTED_MEDIA_V2)
    static bool firstTime = true;
    if (firstTime && scriptExecutionContext()) {
        scriptExecutionContext()->addConsoleMessage(MessageSource::JS, MessageLevel::Warning, ASCIILiteral("'HTMLMediaElement.generateKeyRequest()' is deprecated.  Use 'MediaKeys.createSession()' instead."));
        firstTime = false;
    }
#endif

    if (keySystem.isEmpty()) {
        ec = SYNTAX_ERR;
        return;
    }

    if (!m_player) {
        ec = INVALID_STATE_ERR;
        return;
    }

    const unsigned char* initDataPointer = 0;
    unsigned initDataLength = 0;
    if (initData) {
        initDataPointer = initData->data();
        initDataLength = initData->length();
    }

    MediaPlayer::MediaKeyException result = m_player->generateKeyRequest(keySystem, initDataPointer, initDataLength);
    ec = exceptionCodeForMediaKeyException(result);
}

void HTMLMediaElement::generateKeyRequest(const String& keySystem, ExceptionCode& ec)
{
    generateKeyRequest(keySystem, Uint8Array::create(0), ec);
}

void HTMLMediaElement::addKey(const String& keySystem, PassRefPtr<Uint8Array> key, PassRefPtr<Uint8Array> initData, const String& sessionId, ExceptionCode& ec)
{
#if ENABLE(ENCRYPTED_MEDIA_V2)
    static bool firstTime = true;
    if (firstTime && scriptExecutionContext()) {
        scriptExecutionContext()->addConsoleMessage(MessageSource::JS, MessageLevel::Warning, ASCIILiteral("'HTMLMediaElement.addKey()' is deprecated.  Use 'MediaKeySession.update()' instead."));
        firstTime = false;
    }
#endif

    if (keySystem.isEmpty()) {
        ec = SYNTAX_ERR;
        return;
    }

    if (!key) {
        ec = SYNTAX_ERR;
        return;
    }

    if (!key->length()) {
        ec = TYPE_MISMATCH_ERR;
        return;
    }

    if (!m_player) {
        ec = INVALID_STATE_ERR;
        return;
    }

    const unsigned char* initDataPointer = 0;
    unsigned initDataLength = 0;
    if (initData) {
        initDataPointer = initData->data();
        initDataLength = initData->length();
    }

    MediaPlayer::MediaKeyException result = m_player->addKey(keySystem, key->data(), key->length(), initDataPointer, initDataLength, sessionId);
    ec = exceptionCodeForMediaKeyException(result);
}

void HTMLMediaElement::addKey(const String& keySystem, PassRefPtr<Uint8Array> key, ExceptionCode& ec)
{
    addKey(keySystem, key, Uint8Array::create(0), String(), ec);
}

void HTMLMediaElement::cancelKeyRequest(const String& keySystem, const String& sessionId, ExceptionCode& ec)
{
    if (keySystem.isEmpty()) {
        ec = SYNTAX_ERR;
        return;
    }

    if (!m_player) {
        ec = INVALID_STATE_ERR;
        return;
    }

    MediaPlayer::MediaKeyException result = m_player->cancelKeyRequest(keySystem, sessionId);
    ec = exceptionCodeForMediaKeyException(result);
}

#endif

bool HTMLMediaElement::loop() const
{
    return fastHasAttribute(loopAttr);
}

void HTMLMediaElement::setLoop(bool b)
{
    LOG(Media, "HTMLMediaElement::setLoop(%p) - %s", this, boolString(b));
    setBooleanAttribute(loopAttr, b);
}

bool HTMLMediaElement::controls() const
{
    Frame* frame = document().frame();

    // always show controls when scripting is disabled
    if (frame && !frame->script().canExecuteScripts(NotAboutToExecuteScript))
        return true;

    return fastHasAttribute(controlsAttr);
}

void HTMLMediaElement::setControls(bool b)
{
    LOG(Media, "HTMLMediaElement::setControls(%p) - %s", this, boolString(b));
    setBooleanAttribute(controlsAttr, b);
}

double HTMLMediaElement::volume() const
{
    return m_volume;
}

void HTMLMediaElement::setVolume(double vol, ExceptionCode& ec)
{
    LOG(Media, "HTMLMediaElement::setVolume(%p) - %f", this, vol);

    if (vol < 0.0f || vol > 1.0f) {
        ec = INDEX_SIZE_ERR;
        return;
    }
    
#if !PLATFORM(IOS) && !PLATFORM(WKC)
    if (m_volume != vol) {
        m_volume = vol;
        m_volumeInitialized = true;
        updateVolume();
        scheduleEvent(eventNames().volumechangeEvent);
    }
#endif
}

bool HTMLMediaElement::muted() const
{
    return m_explicitlyMuted ? m_muted : fastHasAttribute(mutedAttr);
}

void HTMLMediaElement::setMuted(bool muted)
{
    LOG(Media, "HTMLMediaElement::setMuted(%p) - %s", this, boolString(muted));

#if PLATFORM(IOS) || PLATFORM(WKC)
    UNUSED_PARAM(muted);
#else
    if (m_muted != muted || !m_explicitlyMuted) {
        m_muted = muted;
        m_explicitlyMuted = true;
        // Avoid recursion when the player reports volume changes.
        if (!processingMediaPlayerCallback()) {
            if (m_player) {
                m_player->setMuted(effectiveMuted());
                if (hasMediaControls())
                    mediaControls()->changedMute();
            }
        }
        scheduleEvent(eventNames().volumechangeEvent);
        document().updateIsPlayingMedia();

#if ENABLE(WIRELESS_PLAYBACK_TARGET)
        updateMediaState(UpdateMediaState::Asynchronously);
#endif
    }
#endif
}

void HTMLMediaElement::togglePlayState()
{
    LOG(Media, "HTMLMediaElement::togglePlayState(%p) - canPlay() is %s", this, boolString(canPlay()));

    // We can safely call the internal play/pause methods, which don't check restrictions, because
    // this method is only called from the built-in media controller
    if (canPlay()) {
#if PLATFORM(WKC)
        removeBehaviorsRestrictionsAfterFirstUserGesture();
#endif
        updatePlaybackRate();
        playInternal();
#if PLATFORM(WKC)
    }
#else
    } else
        pauseInternal();
#endif
}

void HTMLMediaElement::beginScrubbing()
{
    LOG(Media, "HTMLMediaElement::beginScrubbing(%p) - paused() is %s", this, boolString(paused()));

    if (!paused()) {
        if (ended()) {
            // Because a media element stays in non-paused state when it reaches end, playback resumes 
            // when the slider is dragged from the end to another position unless we pause first. Do 
            // a "hard pause" so an event is generated, since we want to stay paused after scrubbing finishes.
            pause();
        } else {
            // Not at the end but we still want to pause playback so the media engine doesn't try to
            // continue playing during scrubbing. Pause without generating an event as we will 
            // unpause after scrubbing finishes.
            setPausedInternal(true);
        }
    }
}

void HTMLMediaElement::endScrubbing()
{
    LOG(Media, "HTMLMediaElement::endScrubbing(%p) - m_pausedInternal is %s", this, boolString(m_pausedInternal));

    if (m_pausedInternal)
        setPausedInternal(false);
}

void HTMLMediaElement::beginScanning(ScanDirection direction)
{
    m_scanType = supportsScanning() ? Scan : Seek;
    m_scanDirection = direction;

    if (m_scanType == Seek) {
        // Scanning by seeking requires the video to be paused during scanning.
        m_actionAfterScan = paused() ? Nothing : Play;
        pause();
    } else {
        // Scanning by scanning requires the video to be playing during scanninging.
        m_actionAfterScan = paused() ? Pause : Nothing;
        play();
        setPlaybackRate(nextScanRate());
    }

    m_scanTimer.start(0, m_scanType == Seek ? SeekRepeatDelay : ScanRepeatDelay);
}

void HTMLMediaElement::endScanning()
{
    if (m_scanType == Scan)
        setPlaybackRate(defaultPlaybackRate());

    if (m_actionAfterScan == Play)
        play();
    else if (m_actionAfterScan == Pause)
        pause();

    if (m_scanTimer.isActive())
        m_scanTimer.stop();
}

double HTMLMediaElement::nextScanRate()
{
    double rate = std::min(ScanMaximumRate, fabs(playbackRate() * 2));
    if (m_scanDirection == Backward)
        rate *= -1;
#if PLATFORM(IOS)
    rate = std::min(std::max(rate, minFastReverseRate()), maxFastForwardRate());
#endif
    return rate;
}

void HTMLMediaElement::scanTimerFired()
{
    if (m_scanType == Seek) {
        double seekTime = m_scanDirection == Forward ? SeekTime : -SeekTime;
        setCurrentTime(currentTime() + seekTime);
    } else
        setPlaybackRate(nextScanRate());
}

// The spec says to fire periodic timeupdate events (those sent while playing) every
// "15 to 250ms", we choose the slowest frequency
static const double maxTimeupdateEventFrequency = 0.25;

void HTMLMediaElement::startPlaybackProgressTimer()
{
    if (m_playbackProgressTimer.isActive())
        return;

    m_previousProgressTime = monotonicallyIncreasingTime();
    m_playbackProgressTimer.startRepeating(maxTimeupdateEventFrequency);
}

void HTMLMediaElement::playbackProgressTimerFired()
{
    ASSERT(m_player);

    if (m_fragmentEndTime.isValid() && currentMediaTime() >= m_fragmentEndTime && requestedPlaybackRate() > 0) {
        m_fragmentEndTime = MediaTime::invalidTime();
        if (!m_mediaController && !m_paused) {
            // changes paused to true and fires a simple event named pause at the media element.
            pauseInternal();
        }
    }
    
    scheduleTimeupdateEvent(true);

    if (!requestedPlaybackRate())
        return;

    if (!m_paused && hasMediaControls())
        mediaControls()->playbackProgressed();

#if ENABLE(VIDEO_TRACK)
    if (RuntimeEnabledFeatures::sharedFeatures().webkitVideoTrackEnabled())
        updateActiveTextTrackCues(currentMediaTime());
#endif

#if ENABLE(MEDIA_SOURCE)
    if (m_mediaSource)
        m_mediaSource->monitorSourceBuffers();
#endif
}

void HTMLMediaElement::scheduleTimeupdateEvent(bool periodicEvent)
{
    double now = monotonicallyIncreasingTime();
    double timedelta = now - m_clockTimeAtLastUpdateEvent;

    // throttle the periodic events
    if (periodicEvent && timedelta < maxTimeupdateEventFrequency)
        return;

    // Some media engines make multiple "time changed" callbacks at the same time, but we only want one
    // event at a given time so filter here
    MediaTime movieTime = currentMediaTime();
    if (movieTime != m_lastTimeUpdateEventMovieTime) {
        scheduleEvent(eventNames().timeupdateEvent);
        m_clockTimeAtLastUpdateEvent = now;
        m_lastTimeUpdateEventMovieTime = movieTime;
    }
}

bool HTMLMediaElement::canPlay() const
{
    return paused() || ended() || m_readyState < HAVE_METADATA;
}

double HTMLMediaElement::percentLoaded() const
{
    if (!m_player)
        return 0;
    MediaTime duration = m_player->duration();

    if (!duration || duration.isPositiveInfinite() || duration.isNegativeInfinite())
        return 0;

    MediaTime buffered = MediaTime::zeroTime();
    bool ignored;
    std::unique_ptr<PlatformTimeRanges> timeRanges = m_player->buffered();
    for (unsigned i = 0; i < timeRanges->length(); ++i) {
        MediaTime start = timeRanges->start(i, ignored);
        MediaTime end = timeRanges->end(i, ignored);
        buffered += end - start;
    }
    return buffered.toDouble() / duration.toDouble();
}

#if ENABLE(VIDEO_TRACK)

void HTMLMediaElement::mediaPlayerDidAddAudioTrack(PassRefPtr<AudioTrackPrivate> prpTrack)
{
    if (isPlaying() && !m_mediaSession->playbackPermitted(*this))
        pauseInternal();

    if (!RuntimeEnabledFeatures::sharedFeatures().webkitVideoTrackEnabled())
        return;

    addAudioTrack(AudioTrack::create(this, prpTrack));
}

void HTMLMediaElement::mediaPlayerDidAddTextTrack(PassRefPtr<InbandTextTrackPrivate> prpTrack)
{
    if (!RuntimeEnabledFeatures::sharedFeatures().webkitVideoTrackEnabled())
        return;
    
    // 4.8.10.12.2 Sourcing in-band text tracks
    // 1. Associate the relevant data with a new text track and its corresponding new TextTrack object.
    RefPtr<InbandTextTrack> textTrack = InbandTextTrack::create(ActiveDOMObject::scriptExecutionContext(), this, prpTrack);
    textTrack->setMediaElement(this);
    
    // 2. Set the new text track's kind, label, and language based on the semantics of the relevant data,
    // as defined by the relevant specification. If there is no label in that data, then the label must
    // be set to the empty string.
    // 3. Associate the text track list of cues with the rules for updating the text track rendering appropriate
    // for the format in question.
    // 4. If the new text track's kind is metadata, then set the text track in-band metadata track dispatch type
    // as follows, based on the type of the media resource:
    // 5. Populate the new text track's list of cues with the cues parsed so far, folllowing the guidelines for exposing
    // cues, and begin updating it dynamically as necessary.
    //   - Thess are all done by the media engine.
    
    // 6. Set the new text track's readiness state to loaded.
    textTrack->setReadinessState(TextTrack::Loaded);
    
    // 7. Set the new text track's mode to the mode consistent with the user's preferences and the requirements of
    // the relevant specification for the data.
    //  - This will happen in configureTextTracks()
    scheduleDelayedAction(ConfigureTextTracks);
    
    // 8. Add the new text track to the media element's list of text tracks.
    // 9. Fire an event with the name addtrack, that does not bubble and is not cancelable, and that uses the TrackEvent
    // interface, with the track attribute initialized to the text track's TextTrack object, at the media element's
    // textTracks attribute's TextTrackList object.
    addTextTrack(textTrack.release());
}

void HTMLMediaElement::mediaPlayerDidAddVideoTrack(PassRefPtr<VideoTrackPrivate> prpTrack)
{
    if (!RuntimeEnabledFeatures::sharedFeatures().webkitVideoTrackEnabled())
        return;

    addVideoTrack(VideoTrack::create(this, prpTrack));
}

void HTMLMediaElement::mediaPlayerDidRemoveAudioTrack(PassRefPtr<AudioTrackPrivate> prpTrack)
{
    prpTrack->willBeRemoved();
}

void HTMLMediaElement::mediaPlayerDidRemoveTextTrack(PassRefPtr<InbandTextTrackPrivate> prpTrack)
{
    prpTrack->willBeRemoved();
}

void HTMLMediaElement::mediaPlayerDidRemoveVideoTrack(PassRefPtr<VideoTrackPrivate> prpTrack)
{
    prpTrack->willBeRemoved();
}

#if USE(PLATFORM_TEXT_TRACK_MENU)
void HTMLMediaElement::setSelectedTextTrack(PassRefPtr<PlatformTextTrack> platformTrack)
{
    if (!m_textTracks)
        return;

    TrackDisplayUpdateScope scope(this);

    if (!platformTrack) {
        setSelectedTextTrack(TextTrack::captionMenuOffItem());
        return;
    }

    TextTrack* textTrack;
    if (platformTrack == PlatformTextTrack::captionMenuOffItem())
        textTrack = TextTrack::captionMenuOffItem();
    else if (platformTrack == PlatformTextTrack::captionMenuAutomaticItem())
        textTrack = TextTrack::captionMenuAutomaticItem();
    else {
        size_t i;
        for (i = 0; i < m_textTracks->length(); ++i) {
            textTrack = m_textTracks->item(i);
            
            if (textTrack->platformTextTrack() == platformTrack)
                break;
        }
        if (i == m_textTracks->length())
            return;
    }

    setSelectedTextTrack(textTrack);
}

Vector<RefPtr<PlatformTextTrack>> HTMLMediaElement::platformTextTracks()
{
    if (!m_textTracks || !m_textTracks->length())
        return Vector<RefPtr<PlatformTextTrack>>();
    
    Vector<RefPtr<PlatformTextTrack>> platformTracks;
    for (size_t i = 0; i < m_textTracks->length(); ++i)
        platformTracks.append(m_textTracks->item(i)->platformTextTrack());
    
    return platformTracks;
}

void HTMLMediaElement::notifyMediaPlayerOfTextTrackChanges()
{
    if (!m_textTracks || !m_textTracks->length() || !platformTextTrackMenu())
        return;
    
    m_platformMenu->tracksDidChange();
}

PlatformTextTrackMenuInterface* HTMLMediaElement::platformTextTrackMenu()
{
    if (m_platformMenu)
        return m_platformMenu.get();

    if (!m_player || !m_player->implementsTextTrackControls())
        return 0;

    m_platformMenu = m_player->textTrackMenu();
    if (!m_platformMenu)
        return 0;

    m_platformMenu->setClient(this);

    return m_platformMenu.get();
}
#endif // #if USE(PLATFORM_TEXT_TRACK_MENU)
    
void HTMLMediaElement::closeCaptionTracksChanged()
{
    if (hasMediaControls())
        mediaControls()->closedCaptionTracksChanged();

#if USE(PLATFORM_TEXT_TRACK_MENU)
    if (m_player && m_player->implementsTextTrackControls())
        scheduleDelayedAction(TextTrackChangesNotification);
#endif
}

void HTMLMediaElement::addAudioTrack(PassRefPtr<AudioTrack> track)
{
    if (!RuntimeEnabledFeatures::sharedFeatures().webkitVideoTrackEnabled())
        return;

    audioTracks()->append(track);
}

void HTMLMediaElement::addTextTrack(PassRefPtr<TextTrack> track)
{
    if (!RuntimeEnabledFeatures::sharedFeatures().webkitVideoTrackEnabled())
        return;

    if (!m_requireCaptionPreferencesChangedCallbacks) {
        m_requireCaptionPreferencesChangedCallbacks = true;
        Document& document = this->document();
        document.registerForCaptionPreferencesChangedCallbacks(this);
        if (Page* page = document.page())
            m_captionDisplayMode = page->group().captionPreferences()->captionDisplayMode();
    }

    textTracks()->append(track);

    closeCaptionTracksChanged();
}

void HTMLMediaElement::addVideoTrack(PassRefPtr<VideoTrack> track)
{
    if (!RuntimeEnabledFeatures::sharedFeatures().webkitVideoTrackEnabled())
        return;

    videoTracks()->append(track);
}

void HTMLMediaElement::removeAudioTrack(AudioTrack* track)
{
    if (!RuntimeEnabledFeatures::sharedFeatures().webkitVideoTrackEnabled())
        return;

    m_audioTracks->remove(track);
    track->clearClient();
}

void HTMLMediaElement::removeTextTrack(TextTrack* track, bool scheduleEvent)
{
    if (!RuntimeEnabledFeatures::sharedFeatures().webkitVideoTrackEnabled())
        return;

    TrackDisplayUpdateScope scope(this);
    if (TextTrackCueList* cues = track->cues())
        textTrackRemoveCues(track, cues);
    track->clearClient();
    if (m_textTracks)
        m_textTracks->remove(track, scheduleEvent);

    closeCaptionTracksChanged();
}

void HTMLMediaElement::removeVideoTrack(VideoTrack* track)
{
    if (!RuntimeEnabledFeatures::sharedFeatures().webkitVideoTrackEnabled())
        return;

    m_videoTracks->remove(track);
    track->clearClient();
}

void HTMLMediaElement::forgetResourceSpecificTracks()
{
    while (m_audioTracks &&  m_audioTracks->length())
        removeAudioTrack(m_audioTracks->lastItem());

    if (m_textTracks) {
        TrackDisplayUpdateScope scope(this);
        for (int i = m_textTracks->length() - 1; i >= 0; --i) {
            TextTrack* track = m_textTracks->item(i);

            if (track->trackType() == TextTrack::InBand)
                removeTextTrack(track, false);
        }
    }

    while (m_videoTracks &&  m_videoTracks->length())
        removeVideoTrack(m_videoTracks->lastItem());
}

PassRefPtr<TextTrack> HTMLMediaElement::addTextTrack(const String& kind, const String& label, const String& language, ExceptionCode& ec)
{
    if (!RuntimeEnabledFeatures::sharedFeatures().webkitVideoTrackEnabled())
        return 0;

    // 4.8.10.12.4 Text track API
    // The addTextTrack(kind, label, language) method of media elements, when invoked, must run the following steps:

    // 1. If kind is not one of the following strings, then throw a SyntaxError exception and abort these steps
    if (!TextTrack::isValidKindKeyword(kind)) {
        ec = SYNTAX_ERR;
        return 0;
    }

    // 2. If the label argument was omitted, let label be the empty string.
    // 3. If the language argument was omitted, let language be the empty string.
    // 4. Create a new TextTrack object.

    // 5. Create a new text track corresponding to the new object, and set its text track kind to kind, its text 
    // track label to label, its text track language to language...
    RefPtr<TextTrack> textTrack = TextTrack::create(ActiveDOMObject::scriptExecutionContext(), this, kind, emptyString(), label, language);

    // Note, due to side effects when changing track parameters, we have to
    // first append the track to the text track list.

    // 6. Add the new text track to the media element's list of text tracks.
    addTextTrack(textTrack);

    // ... its text track readiness state to the text track loaded state ...
    textTrack->setReadinessState(TextTrack::Loaded);

    // ... its text track mode to the text track hidden mode, and its text track list of cues to an empty list ...
    textTrack->setMode(TextTrack::hiddenKeyword());

    return textTrack.release();
}

AudioTrackList* HTMLMediaElement::audioTracks()
{
    if (!RuntimeEnabledFeatures::sharedFeatures().webkitVideoTrackEnabled())
        return 0;

    if (!m_audioTracks)
        m_audioTracks = AudioTrackList::create(this, ActiveDOMObject::scriptExecutionContext());

    return m_audioTracks.get();
}

TextTrackList* HTMLMediaElement::textTracks() 
{
    if (!RuntimeEnabledFeatures::sharedFeatures().webkitVideoTrackEnabled())
        return 0;

    if (!m_textTracks)
        m_textTracks = TextTrackList::create(this, ActiveDOMObject::scriptExecutionContext());

    return m_textTracks.get();
}

VideoTrackList* HTMLMediaElement::videoTracks()
{
    if (!RuntimeEnabledFeatures::sharedFeatures().webkitVideoTrackEnabled())
        return 0;

    if (!m_videoTracks)
        m_videoTracks = VideoTrackList::create(this, ActiveDOMObject::scriptExecutionContext());

    return m_videoTracks.get();
}

void HTMLMediaElement::didAddTextTrack(HTMLTrackElement* trackElement)
{
    ASSERT(trackElement->hasTagName(trackTag));

    if (!RuntimeEnabledFeatures::sharedFeatures().webkitVideoTrackEnabled())
        return;

    // 4.8.10.12.3 Sourcing out-of-band text tracks
    // When a track element's parent element changes and the new parent is a media element, 
    // then the user agent must add the track element's corresponding text track to the 
    // media element's list of text tracks ... [continues in TextTrackList::append]
    RefPtr<TextTrack> textTrack = trackElement->track();
    if (!textTrack)
        return;
    
    addTextTrack(textTrack.release());
    
    // Do not schedule the track loading until parsing finishes so we don't start before all tracks
    // in the markup have been added.
    if (!m_parsingInProgress)
        scheduleDelayedAction(ConfigureTextTracks);

    if (hasMediaControls())
        mediaControls()->closedCaptionTracksChanged();
}

void HTMLMediaElement::didRemoveTextTrack(HTMLTrackElement* trackElement)
{
    ASSERT(trackElement->hasTagName(trackTag));

    if (!RuntimeEnabledFeatures::sharedFeatures().webkitVideoTrackEnabled())
        return;

#if !LOG_DISABLED
    if (trackElement->hasTagName(trackTag)) {
        URL url = trackElement->getNonEmptyURLAttribute(srcAttr);
        LOG(Media, "HTMLMediaElement::didRemoveTrack(%p) - 'src' is %s", this, urlForLoggingMedia(url).utf8().data());
    }
#endif

    RefPtr<TextTrack> textTrack = trackElement->track();
    if (!textTrack)
        return;
    
    textTrack->setHasBeenConfigured(false);

    if (!m_textTracks)
        return;
    
    // 4.8.10.12.3 Sourcing out-of-band text tracks
    // When a track element's parent element changes and the old parent was a media element, 
    // then the user agent must remove the track element's corresponding text track from the 
    // media element's list of text tracks.
    removeTextTrack(textTrack.get());

    size_t index = m_textTracksWhenResourceSelectionBegan.find(textTrack.get());
    if (index != notFound)
        m_textTracksWhenResourceSelectionBegan.remove(index);
}

void HTMLMediaElement::configureTextTrackGroup(const TrackGroup& group)
{
    ASSERT(group.tracks.size());

    LOG(Media, "HTMLMediaElement::configureTextTrackGroup(%p)", this);

    Page* page = document().page();
    CaptionUserPreferences* captionPreferences = page ? page->group().captionPreferences() : 0;
    CaptionUserPreferences::CaptionDisplayMode displayMode = captionPreferences ? captionPreferences->captionDisplayMode() : CaptionUserPreferences::Automatic;

    // First, find the track in the group that should be enabled (if any).
    Vector<RefPtr<TextTrack>> currentlyEnabledTracks;
    RefPtr<TextTrack> trackToEnable;
    RefPtr<TextTrack> defaultTrack;
    RefPtr<TextTrack> fallbackTrack;
    RefPtr<TextTrack> forcedSubitleTrack;
    int highestTrackScore = 0;
    int highestForcedScore = 0;

    // If there is a visible track, it has already been configured so it won't be considered in the loop below. We don't want to choose another
    // track if it is less suitable, and we do want to disable it if another track is more suitable.
    int alreadyVisibleTrackScore = 0;
    if (group.visibleTrack && captionPreferences) {
        alreadyVisibleTrackScore = captionPreferences->textTrackSelectionScore(group.visibleTrack.get(), this);
        currentlyEnabledTracks.append(group.visibleTrack);
    }

    for (size_t i = 0; i < group.tracks.size(); ++i) {
        RefPtr<TextTrack> textTrack = group.tracks[i];

        if (m_processingPreferenceChange && textTrack->mode() == TextTrack::showingKeyword())
            currentlyEnabledTracks.append(textTrack);

        int trackScore = captionPreferences ? captionPreferences->textTrackSelectionScore(textTrack.get(), this) : 0;
        LOG(Media, "HTMLMediaElement::configureTextTrackGroup(%p) -  '%s' track with language '%s' has score %i", this, textTrack->kind().string().utf8().data(), textTrack->language().string().utf8().data(), trackScore);

        if (trackScore) {

            // * If the text track kind is { [subtitles or captions] [descriptions] } and the user has indicated an interest in having a
            // track with this text track kind, text track language, and text track label enabled, and there is no
            // other text track in the media element's list of text tracks with a text track kind of either subtitles
            // or captions whose text track mode is showing
            // ...
            // * If the text track kind is chapters and the text track language is one that the user agent has reason
            // to believe is appropriate for the user, and there is no other text track in the media element's list of
            // text tracks with a text track kind of chapters whose text track mode is showing
            //    Let the text track mode be showing.
            if (trackScore > highestTrackScore && trackScore > alreadyVisibleTrackScore) {
                highestTrackScore = trackScore;
                trackToEnable = textTrack;
            }

            if (!defaultTrack && textTrack->isDefault())
                defaultTrack = textTrack;
            if (!defaultTrack && !fallbackTrack)
                fallbackTrack = textTrack;
            if (textTrack->containsOnlyForcedSubtitles() && trackScore > highestForcedScore) {
                forcedSubitleTrack = textTrack;
                highestForcedScore = trackScore;
            }
        } else if (!group.visibleTrack && !defaultTrack && textTrack->isDefault()) {
            // * If the track element has a default attribute specified, and there is no other text track in the media
            // element's list of text tracks whose text track mode is showing or showing by default
            //    Let the text track mode be showing by default.
            if (group.kind != TrackGroup::CaptionsAndSubtitles || displayMode != CaptionUserPreferences::ForcedOnly)
                defaultTrack = textTrack;
        }
    }

    if (displayMode != CaptionUserPreferences::Manual) {
        if (!trackToEnable && defaultTrack)
            trackToEnable = defaultTrack;

        // If no track matches the user's preferred language, none was marked as 'default', and there is a forced subtitle track
        // in the same language as the language of the primary audio track, enable it.
        if (!trackToEnable && forcedSubitleTrack)
            trackToEnable = forcedSubitleTrack;

        // If no track matches, don't disable an already visible track unless preferences say they all should be off.
        if (group.kind != TrackGroup::CaptionsAndSubtitles || displayMode != CaptionUserPreferences::ForcedOnly) {
            if (!trackToEnable && !defaultTrack && group.visibleTrack)
                trackToEnable = group.visibleTrack;
        }

        // If no track matches the user's preferred language and non was marked 'default', enable the first track
        // because the user has explicitly stated a preference for this kind of track.
        if (!trackToEnable && fallbackTrack)
            trackToEnable = fallbackTrack;

        if (trackToEnable)
            m_subtitleTrackLanguage = trackToEnable->language();
        else
            m_subtitleTrackLanguage = emptyString();
    }

    if (currentlyEnabledTracks.size()) {
        for (size_t i = 0; i < currentlyEnabledTracks.size(); ++i) {
            RefPtr<TextTrack> textTrack = currentlyEnabledTracks[i];
            if (textTrack != trackToEnable)
                textTrack->setMode(TextTrack::disabledKeyword());
        }
    }

    if (trackToEnable) {
        trackToEnable->setMode(TextTrack::showingKeyword());

        // If user preferences indicate we should always display captions, make sure we reflect the
        // proper status via the webkitClosedCaptionsVisible API call:
        if (!webkitClosedCaptionsVisible() && closedCaptionsVisible() && displayMode == CaptionUserPreferences::AlwaysOn)
            m_webkitLegacyClosedCaptionOverride = true;
    }

    m_processingPreferenceChange = false;
}

static JSC::JSValue controllerJSValue(JSC::ExecState& exec, JSDOMGlobalObject& globalObject, HTMLMediaElement& media)
{
    auto mediaJSWrapper = toJS(&exec, &globalObject, &media);
    
    // Retrieve the controller through the JS object graph
    JSC::JSObject* mediaJSWrapperObject = JSC::jsDynamicCast<JSC::JSObject*>(mediaJSWrapper);
    if (!mediaJSWrapperObject)
        return JSC::jsNull();
    
    JSC::Identifier controlsHost = JSC::Identifier::fromString(&exec.vm(), "controlsHost");
    JSC::JSValue controlsHostJSWrapper = mediaJSWrapperObject->get(&exec, controlsHost);
    if (exec.hadException())
        return JSC::jsNull();

    JSC::JSObject* controlsHostJSWrapperObject = JSC::jsDynamicCast<JSC::JSObject*>(controlsHostJSWrapper);
    if (!controlsHostJSWrapperObject)
        return JSC::jsNull();

    JSC::Identifier controllerID = JSC::Identifier::fromString(&exec.vm(), "controller");
    JSC::JSValue controllerJSWrapper = controlsHostJSWrapperObject->get(&exec, controllerID);
    if (exec.hadException())
        return JSC::jsNull();

    return controllerJSWrapper;
}

void HTMLMediaElement::ensureMediaControlsShadowRoot()
{
    ASSERT(!m_creatingControls);
    m_creatingControls = true;
    ensureUserAgentShadowRoot();
    m_creatingControls = false;
}

void HTMLMediaElement::updateCaptionContainer()
{
    LOG(Media, "HTMLMediaElement::updateCaptionContainer(%p)", this);
#if ENABLE(MEDIA_CONTROLS_SCRIPT)
    if (m_haveSetUpCaptionContainer)
        return;

    Page* page = document().page();
    if (!page)
        return;

    DOMWrapperWorld& world = ensureIsolatedWorld();

    if (!ensureMediaControlsInjectedScript())
        return;

    ensureMediaControlsShadowRoot();

    if (!m_mediaControlsHost)
        m_mediaControlsHost = MediaControlsHost::create(this);

    ScriptController& scriptController = document().frame()->script();
    JSDOMGlobalObject* globalObject = JSC::jsCast<JSDOMGlobalObject*>(scriptController.globalObject(world));
    JSC::ExecState* exec = globalObject->globalExec();
    JSC::JSLockHolder lock(exec);

    JSC::JSValue controllerValue = controllerJSValue(*exec, *globalObject, *this);
    JSC::JSObject* controllerObject = JSC::jsDynamicCast<JSC::JSObject*>(controllerValue);
    if (!controllerObject)
        return;

    // The media controls script must provide a method on the Controller object with the following details.
    // Name: updateCaptionContainer
    // Parameters:
    //     None
    // Return value:
    //     None
    JSC::JSValue methodValue = controllerObject->get(exec, JSC::Identifier::fromString(exec, "updateCaptionContainer"));
    JSC::JSObject* methodObject = JSC::jsDynamicCast<JSC::JSObject*>(methodValue);
    if (!methodObject)
        return;

    JSC::CallData callData;
    JSC::CallType callType = methodObject->methodTable()->getCallData(methodObject, callData);
    if (callType == JSC::CallTypeNone)
        return;

    JSC::MarkedArgumentBuffer noArguments;
    JSC::call(exec, methodObject, callType, callData, controllerObject, noArguments);
    exec->clearException();

    m_haveSetUpCaptionContainer = true;
#endif
}

void HTMLMediaElement::layoutSizeChanged()
{
#if ENABLE(MEDIA_CONTROLS_SCRIPT)
    RefPtr<HTMLMediaElement> strongThis = this;
#if PLATFORM(WKC)
    WTF::Function<void()> task = [strongThis] {
#else
    std::function<void()> task = [strongThis] {
#endif
        if (ShadowRoot* root = strongThis->userAgentShadowRoot())
            root->dispatchEvent(Event::create("resize", false, false));
    };
#if PLATFORM(WKC)
    m_resizeTaskQueue.enqueueTask(WTF::move(task));
#else
    m_resizeTaskQueue.enqueueTask(task);
#endif
#endif
}

void HTMLMediaElement::visibilityDidChange()
{
    updateShouldAutoplay();
}

void HTMLMediaElement::setSelectedTextTrack(TextTrack* trackToSelect)
{
    TextTrackList* trackList = textTracks();
    if (!trackList || !trackList->length())
        return;

    if (trackToSelect != TextTrack::captionMenuOffItem() && trackToSelect != TextTrack::captionMenuAutomaticItem()) {
        if (!trackList->contains(trackToSelect))
            return;
        
        for (int i = 0, length = trackList->length(); i < length; ++i) {
            TextTrack* track = trackList->item(i);
            if (!trackToSelect || track != trackToSelect)
                track->setMode(TextTrack::disabledKeyword());
            else
                track->setMode(TextTrack::showingKeyword());
        }
    } else if (trackToSelect == TextTrack::captionMenuOffItem()) {
        for (int i = 0, length = trackList->length(); i < length; ++i)
            trackList->item(i)->setMode(TextTrack::disabledKeyword());
    }

    CaptionUserPreferences* captionPreferences = document().page() ? document().page()->group().captionPreferences() : 0;
    if (!captionPreferences)
        return;

    CaptionUserPreferences::CaptionDisplayMode displayMode;
    if (trackToSelect == TextTrack::captionMenuOffItem())
        displayMode = CaptionUserPreferences::ForcedOnly;
    else if (trackToSelect == TextTrack::captionMenuAutomaticItem())
        displayMode = CaptionUserPreferences::Automatic;
    else {
        displayMode = CaptionUserPreferences::AlwaysOn;
        if (trackToSelect->language().length())
            captionPreferences->setPreferredLanguage(trackToSelect->language());
    }

    captionPreferences->setCaptionDisplayMode(displayMode);
}

void HTMLMediaElement::configureTextTracks()
{
    TrackGroup captionAndSubtitleTracks(TrackGroup::CaptionsAndSubtitles);
    TrackGroup descriptionTracks(TrackGroup::Description);
    TrackGroup chapterTracks(TrackGroup::Chapter);
    TrackGroup metadataTracks(TrackGroup::Metadata);
    TrackGroup otherTracks(TrackGroup::Other);

    if (!m_textTracks)
        return;

    for (size_t i = 0; i < m_textTracks->length(); ++i) {
        RefPtr<TextTrack> textTrack = m_textTracks->item(i);
        if (!textTrack)
            continue;

        String kind = textTrack->kind();
        TrackGroup* currentGroup;
        if (kind == TextTrack::subtitlesKeyword() || kind == TextTrack::captionsKeyword() || kind == TextTrack::forcedKeyword())
            currentGroup = &captionAndSubtitleTracks;
        else if (kind == TextTrack::descriptionsKeyword())
            currentGroup = &descriptionTracks;
        else if (kind == TextTrack::chaptersKeyword())
            currentGroup = &chapterTracks;
        else if (kind == TextTrack::metadataKeyword())
            currentGroup = &metadataTracks;
        else
            currentGroup = &otherTracks;

        if (!currentGroup->visibleTrack && textTrack->mode() == TextTrack::showingKeyword())
            currentGroup->visibleTrack = textTrack;
        if (!currentGroup->defaultTrack && textTrack->isDefault())
            currentGroup->defaultTrack = textTrack;

        // Do not add this track to the group if it has already been automatically configured
        // as we only want to call configureTextTrack once per track so that adding another 
        // track after the initial configuration doesn't reconfigure every track - only those 
        // that should be changed by the new addition. For example all metadata tracks are 
        // disabled by default, and we don't want a track that has been enabled by script 
        // to be disabled automatically when a new metadata track is added later.
        if (textTrack->hasBeenConfigured())
            continue;
        
        if (textTrack->language().length())
            currentGroup->hasSrcLang = true;
        currentGroup->tracks.append(textTrack);
    }
    
    if (captionAndSubtitleTracks.tracks.size())
        configureTextTrackGroup(captionAndSubtitleTracks);
    if (descriptionTracks.tracks.size())
        configureTextTrackGroup(descriptionTracks);
    if (chapterTracks.tracks.size())
        configureTextTrackGroup(chapterTracks);
    if (metadataTracks.tracks.size())
        configureTextTrackGroup(metadataTracks);
    if (otherTracks.tracks.size())
        configureTextTrackGroup(otherTracks);

    updateCaptionContainer();
    configureTextTrackDisplay();
    if (hasMediaControls())
        mediaControls()->closedCaptionTracksChanged();
}
#endif

bool HTMLMediaElement::havePotentialSourceChild()
{
    // Stash the current <source> node and next nodes so we can restore them after checking
    // to see there is another potential.
    RefPtr<HTMLSourceElement> currentSourceNode = m_currentSourceNode;
    RefPtr<Node> nextNode = m_nextChildNodeToConsider;

    URL nextURL = selectNextSourceChild(0, 0, DoNothing);

    m_currentSourceNode = currentSourceNode;
    m_nextChildNodeToConsider = nextNode;

    return nextURL.isValid();
}

URL HTMLMediaElement::selectNextSourceChild(ContentType* contentType, String* keySystem, InvalidURLAction actionIfInvalid)
{
#if !LOG_DISABLED
    // Don't log if this was just called to find out if there are any valid <source> elements.
    bool shouldLog = actionIfInvalid != DoNothing;
    if (shouldLog)
        LOG(Media, "HTMLMediaElement::selectNextSourceChild(%p)", this);
#endif

    if (!m_nextChildNodeToConsider) {
#if !LOG_DISABLED
        if (shouldLog)
            LOG(Media, "HTMLMediaElement::selectNextSourceChild(%p) - end of list, stopping", this);
#endif
        return URL();
    }

    URL mediaURL;
    HTMLSourceElement* source = nullptr;
    String type;
    String system;
    bool lookingForStartNode = m_nextChildNodeToConsider;
    bool canUseSourceElement = false;
    bool okToLoadSourceURL;

    NodeVector potentialSourceNodes;
    getChildNodes(*this, potentialSourceNodes);

    for (unsigned i = 0; !canUseSourceElement && i < potentialSourceNodes.size(); ++i) {
        Node& node = potentialSourceNodes[i].get();
        if (lookingForStartNode && m_nextChildNodeToConsider != &node)
            continue;
        lookingForStartNode = false;

        if (!node.hasTagName(sourceTag))
            continue;
        if (node.parentNode() != this)
            continue;

        source = downcast<HTMLSourceElement>(&node);

        // If candidate does not have a src attribute, or if its src attribute's value is the empty string ... jump down to the failed step below
        mediaURL = source->getNonEmptyURLAttribute(srcAttr);
#if !LOG_DISABLED
        if (shouldLog)
            LOG(Media, "HTMLMediaElement::selectNextSourceChild(%p) - 'src' is %s", this, urlForLoggingMedia(mediaURL).utf8().data());
#endif
        if (mediaURL.isEmpty())
            goto check_again;
        
        if (source->fastHasAttribute(mediaAttr)) {
            MediaQueryEvaluator screenEval("screen", document().frame(), renderer() ? &renderer()->style() : nullptr);
            RefPtr<MediaQuerySet> media = MediaQuerySet::createAllowingDescriptionSyntax(source->media());
#if !LOG_DISABLED
            if (shouldLog)
                LOG(Media, "HTMLMediaElement::selectNextSourceChild(%p) - 'media' is %s", this, source->media().utf8().data());
#endif
            if (!screenEval.eval(media.get())) 
                goto check_again;
        }

        type = source->type();
        // FIXME(82965): Add support for keySystem in <source> and set system from source.
        if (type.isEmpty() && mediaURL.protocolIsData())
            type = mimeTypeFromDataURL(mediaURL);
        if (!type.isEmpty() || !system.isEmpty()) {
#if !LOG_DISABLED
            if (shouldLog)
                LOG(Media, "HTMLMediaElement::selectNextSourceChild(%p) - 'type' is '%s' - key system is '%s'", this, type.utf8().data(), system.utf8().data());
#endif
            MediaEngineSupportParameters parameters;
            ContentType contentType(type);
            parameters.type = contentType.type().lower();
            parameters.codecs = contentType.parameter(ASCIILiteral("codecs"));
            parameters.url = mediaURL;
#if ENABLE(ENCRYPTED_MEDIA)
            parameters.keySystem = system;
#endif
#if ENABLE(MEDIA_SOURCE)
            parameters.isMediaSource = mediaURL.protocolIs(mediaSourceBlobProtocol);
#endif
#if ENABLE(MEDIA_STREAM)
            parameters.isMediaStream = mediaURL.protocolIs(mediaStreamBlobProtocol);
#endif
            if (!MediaPlayer::supportsType(parameters, this))
                goto check_again;
        }

        // Is it safe to load this url?
        okToLoadSourceURL = isSafeToLoadURL(mediaURL, actionIfInvalid) && dispatchBeforeLoadEvent(mediaURL.string());

        // A 'beforeload' event handler can mutate the DOM, so check to see if the source element is still a child node.
        if (node.parentNode() != this) {
            LOG(Media, "HTMLMediaElement::selectNextSourceChild(%p) - 'beforeload' removed current element", this);
            source = 0;
            goto check_again;
        }

        if (!okToLoadSourceURL)
            goto check_again;

        // Making it this far means the <source> looks reasonable.
        canUseSourceElement = true;

check_again:
        if (!canUseSourceElement && actionIfInvalid == Complain && source)
            source->scheduleErrorEvent();
    }

    if (canUseSourceElement) {
        if (contentType)
            *contentType = ContentType(type);
        if (keySystem)
            *keySystem = system;
        m_currentSourceNode = source;
        m_nextChildNodeToConsider = source->nextSibling();
    } else {
        m_currentSourceNode = nullptr;
        m_nextChildNodeToConsider = nullptr;
    }

#if !LOG_DISABLED
    if (shouldLog)
        LOG(Media, "HTMLMediaElement::selectNextSourceChild(%p) -> %p, %s", this, m_currentSourceNode.get(), canUseSourceElement ? urlForLoggingMedia(mediaURL).utf8().data() : "");
#endif
    return canUseSourceElement ? mediaURL : URL();
}

void HTMLMediaElement::sourceWasAdded(HTMLSourceElement* source)
{
    LOG(Media, "HTMLMediaElement::sourceWasAdded(%p) - %p", this, source);

#if !LOG_DISABLED
    if (source->hasTagName(sourceTag)) {
        URL url = source->getNonEmptyURLAttribute(srcAttr);
        LOG(Media, "HTMLMediaElement::sourceWasAdded(%p) - 'src' is %s", this, urlForLoggingMedia(url).utf8().data());
    }
#endif
    
    // We should only consider a <source> element when there is not src attribute at all.
    if (fastHasAttribute(srcAttr))
        return;

    // 4.8.8 - If a source element is inserted as a child of a media element that has no src 
    // attribute and whose networkState has the value NETWORK_EMPTY, the user agent must invoke 
    // the media element's resource selection algorithm.
    if (networkState() == HTMLMediaElement::NETWORK_EMPTY) {
        scheduleDelayedAction(LoadMediaResource);
        m_nextChildNodeToConsider = source;
        return;
    }

    if (m_currentSourceNode && source == m_currentSourceNode->nextSibling()) {
        LOG(Media, "HTMLMediaElement::sourceWasAdded(%p) - <source> inserted immediately after current source", this);
        m_nextChildNodeToConsider = source;
        return;
    }

    if (m_nextChildNodeToConsider)
        return;
    
    // 4.8.9.5, resource selection algorithm, source elements section:
    // 21. Wait until the node after pointer is a node other than the end of the list. (This step might wait forever.)
    // 22. Asynchronously await a stable state...
    // 23. Set the element's delaying-the-load-event flag back to true (this delays the load event again, in case 
    // it hasn't been fired yet).
    setShouldDelayLoadEvent(true);

    // 24. Set the networkState back to NETWORK_LOADING.
    m_networkState = NETWORK_LOADING;
    
    // 25. Jump back to the find next candidate step above.
    m_nextChildNodeToConsider = source;
    scheduleNextSourceChild();
}

void HTMLMediaElement::sourceWasRemoved(HTMLSourceElement* source)
{
    LOG(Media, "HTMLMediaElement::sourceWasRemoved(%p) - %p", this, source);

#if !LOG_DISABLED
    if (source->hasTagName(sourceTag)) {
        URL url = source->getNonEmptyURLAttribute(srcAttr);
        LOG(Media, "HTMLMediaElement::sourceWasRemoved(%p) - 'src' is %s", this, urlForLoggingMedia(url).utf8().data());
    }
#endif

    if (source != m_currentSourceNode && source != m_nextChildNodeToConsider)
        return;

    if (source == m_nextChildNodeToConsider) {
        if (m_currentSourceNode)
            m_nextChildNodeToConsider = m_currentSourceNode->nextSibling();
        LOG(Media, "HTMLMediaElement::sourceRemoved(%p) - m_nextChildNodeToConsider set to %p", this, m_nextChildNodeToConsider.get());
    } else if (source == m_currentSourceNode) {
        // Clear the current source node pointer, but don't change the movie as the spec says:
        // 4.8.8 - Dynamically modifying a source element and its attribute when the element is already 
        // inserted in a video or audio element will have no effect.
        m_currentSourceNode = nullptr;
        LOG(Media, "HTMLMediaElement::sourceRemoved(%p) - m_currentSourceNode set to 0", this);
    }
}

void HTMLMediaElement::mediaPlayerTimeChanged(MediaPlayer*)
{
    LOG(Media, "HTMLMediaElement::mediaPlayerTimeChanged(%p)", this);

#if ENABLE(VIDEO_TRACK)
    if (RuntimeEnabledFeatures::sharedFeatures().webkitVideoTrackEnabled())
        updateActiveTextTrackCues(currentMediaTime());
#endif

    beginProcessingMediaPlayerCallback();

    invalidateCachedTime();

    // 4.8.10.9 step 14 & 15.  Needed if no ReadyState change is associated with the seek.
    if (m_seeking && m_readyState >= HAVE_CURRENT_DATA && !m_player->seeking())
        finishSeek();
    
    // Always call scheduleTimeupdateEvent when the media engine reports a time discontinuity, 
    // it will only queue a 'timeupdate' event if we haven't already posted one at the current
    // movie time.
    else
        scheduleTimeupdateEvent(false);

    MediaTime now = currentMediaTime();
    MediaTime dur = durationMediaTime();
    double playbackRate = requestedPlaybackRate();
    
    // When the current playback position reaches the end of the media resource then the user agent must follow these steps:
    if (dur.isValid() && dur) {
        // If the media element has a loop attribute specified and does not have a current media controller,
        if (loop() && !m_mediaController && playbackRate > 0) {
            m_sentEndEvent = false;
            // then seek to the earliest possible position of the media resource and abort these steps when the direction of
            // playback is forwards,
            if (now >= dur)
                seekInternal(MediaTime::zeroTime());
        } else if ((now <= MediaTime::zeroTime() && playbackRate < 0) || (now >= dur && playbackRate > 0)) {
            // If the media element does not have a current media controller, and the media element
            // has still ended playback and paused is false,
            if (!m_mediaController && !m_paused) {
                // changes paused to true and fires a simple event named pause at the media element.
                m_paused = true;
                scheduleEvent(eventNames().pauseEvent);
                m_mediaSession->clientWillPausePlayback();
            }
            // Queue a task to fire a simple event named ended at the media element.
            if (!m_sentEndEvent) {
                m_sentEndEvent = true;
                scheduleEvent(eventNames().endedEvent);
            }
            // If the media element has a current media controller, then report the controller state
            // for the media element's current media controller.
            updateMediaController();
        } else
            m_sentEndEvent = false;
    } else {
#if PLATFORM(IOS)
        // The controller changes movie time directly instead of calling through here so we need
        // to post timeupdate events in response to time changes.
        scheduleTimeupdateEvent(false);
#endif
        m_sentEndEvent = false;
    }

    updatePlayState();
    endProcessingMediaPlayerCallback();
}

void HTMLMediaElement::handleSeekToPlaybackPosition(double position)
{
#if PLATFORM(MAC)
    // FIXME: This should ideally use faskSeek, but this causes MediaRemote's playhead to flicker upon release.
    // Please see <rdar://problem/28457219> for more details.
    seek(MediaTime::createWithDouble(position));
    m_seekToPlaybackPositionEndedTimer.stop();
    m_seekToPlaybackPositionEndedTimer.startOneShot(0.5);

    if (!m_isScrubbingRemotely) {
        m_isScrubbingRemotely = true;
        if (!paused())
            pauseInternal();
    }
#else
    fastSeek(position);
#endif
}

void HTMLMediaElement::seekToPlaybackPositionEndedTimerFired()
{
#if PLATFORM(MAC)
    if (!m_isScrubbingRemotely)
        return;

    PlatformMediaSessionManager::sharedManager().sessionDidEndRemoteScrubbing(*m_mediaSession);
    m_isScrubbingRemotely = false;
    m_seekToPlaybackPositionEndedTimer.stop();
#endif
}

void HTMLMediaElement::mediaPlayerVolumeChanged(MediaPlayer*)
{
    LOG(Media, "HTMLMediaElement::mediaPlayerVolumeChanged(%p)", this);

    beginProcessingMediaPlayerCallback();
    if (m_player) {
        double vol = m_player->volume();
        if (vol != m_volume) {
            m_volume = vol;
            updateVolume();
            scheduleEvent(eventNames().volumechangeEvent);
        }
    }
    endProcessingMediaPlayerCallback();
}

void HTMLMediaElement::mediaPlayerMuteChanged(MediaPlayer*)
{
    LOG(Media, "HTMLMediaElement::mediaPlayerMuteChanged(%p)", this);

    beginProcessingMediaPlayerCallback();
    if (m_player)
        setMuted(m_player->muted());
    endProcessingMediaPlayerCallback();
}

void HTMLMediaElement::mediaPlayerDurationChanged(MediaPlayer* player)
{
    LOG(Media, "HTMLMediaElement::mediaPlayerDurationChanged(%p)", this);

    beginProcessingMediaPlayerCallback();

    scheduleEvent(eventNames().durationchangeEvent);
    mediaPlayerCharacteristicChanged(player);

    MediaTime now = currentMediaTime();
    MediaTime dur = durationMediaTime();
    if (now > dur)
        seekInternal(dur);

    endProcessingMediaPlayerCallback();
}

void HTMLMediaElement::mediaPlayerRateChanged(MediaPlayer*)
{
    beginProcessingMediaPlayerCallback();

    // Stash the rate in case the one we tried to set isn't what the engine is
    // using (eg. it can't handle the rate we set)
    m_reportedPlaybackRate = m_player->rate();

    LOG(Media, "HTMLMediaElement::mediaPlayerRateChanged(%p) - rate: %lf", this, m_reportedPlaybackRate);

    if (m_playing)
        invalidateCachedTime();

    updateSleepDisabling();

    endProcessingMediaPlayerCallback();
}

void HTMLMediaElement::mediaPlayerPlaybackStateChanged(MediaPlayer*)
{
    LOG(Media, "HTMLMediaElement::mediaPlayerPlaybackStateChanged(%p)", this);

    if (!m_player || m_pausedInternal)
        return;

    beginProcessingMediaPlayerCallback();
    if (m_player->paused())
        pauseInternal();
    else
        playInternal();
    endProcessingMediaPlayerCallback();
}

void HTMLMediaElement::mediaPlayerSawUnsupportedTracks(MediaPlayer*)
{
    LOG(Media, "HTMLMediaElement::mediaPlayerSawUnsupportedTracks(%p)", this);

    // The MediaPlayer came across content it cannot completely handle.
    // This is normally acceptable except when we are in a standalone
    // MediaDocument. If so, tell the document what has happened.
    if (is<MediaDocument>(document()))
        downcast<MediaDocument>(document()).mediaElementSawUnsupportedTracks();
}

void HTMLMediaElement::mediaPlayerResourceNotSupported(MediaPlayer*)
{
    LOG(Media, "HTMLMediaElement::mediaPlayerResourceNotSupported(%p)", this);

    // The MediaPlayer came across content which no installed engine supports.
    mediaLoadingFailed(MediaPlayer::FormatError);
}

// MediaPlayerPresentation methods
void HTMLMediaElement::mediaPlayerRepaint(MediaPlayer*)
{
    beginProcessingMediaPlayerCallback();
    updateDisplayState();
    if (renderer())
        renderer()->repaint();
    endProcessingMediaPlayerCallback();
}

void HTMLMediaElement::mediaPlayerSizeChanged(MediaPlayer*)
{
    LOG(Media, "HTMLMediaElement::mediaPlayerSizeChanged(%p)", this);

    if (is<MediaDocument>(document()) && m_player)
        downcast<MediaDocument>(document()).mediaElementNaturalSizeChanged(expandedIntSize(m_player->naturalSize()));

    beginProcessingMediaPlayerCallback();
    if (renderer())
        renderer()->updateFromElement();
    endProcessingMediaPlayerCallback();
}

bool HTMLMediaElement::mediaPlayerRenderingCanBeAccelerated(MediaPlayer*)
{
    if (is<RenderVideo>(renderer()))
        return renderer()->view().compositor().canAccelerateVideoRendering(downcast<RenderVideo>(*renderer()));
    return false;
}

void HTMLMediaElement::mediaPlayerRenderingModeChanged(MediaPlayer*)
{
    LOG(Media, "HTMLMediaElement::mediaPlayerRenderingModeChanged(%p)", this);

    // Kick off a fake recalcStyle that will update the compositing tree.
    setNeedsStyleRecalc(SyntheticStyleChange);
}

#if PLATFORM(WIN) && USE(AVFOUNDATION)
GraphicsDeviceAdapter* HTMLMediaElement::mediaPlayerGraphicsDeviceAdapter(const MediaPlayer*) const
{
    if (!document().page())
        return 0;

    return document().page()->chrome().client().graphicsDeviceAdapter();
}
#endif

void HTMLMediaElement::mediaPlayerEngineUpdated(MediaPlayer*)
{
    LOG(Media, "HTMLMediaElement::mediaPlayerEngineUpdated(%p)", this);
    beginProcessingMediaPlayerCallback();
    if (renderer())
        renderer()->updateFromElement();
    endProcessingMediaPlayerCallback();

#if ENABLE(MEDIA_SOURCE)
    m_droppedVideoFrames = 0;
#endif

    m_havePreparedToPlay = false;

    m_mediaSession->mediaEngineUpdated(*this);

#if ENABLE(WEB_AUDIO)
    if (m_audioSourceNode && audioSourceProvider()) {
        m_audioSourceNode->lock();
        audioSourceProvider()->setClient(m_audioSourceNode);
        m_audioSourceNode->unlock();
    }
#endif

#if PLATFORM(IOS)
    if (!m_player)
        return;
    m_player->setVideoFullscreenFrame(m_videoFullscreenFrame);
    m_player->setVideoFullscreenGravity(m_videoFullscreenGravity);
    m_player->setVideoFullscreenLayer(m_videoFullscreenLayer.get());
#endif
#if ENABLE(WIRELESS_PLAYBACK_TARGET)
    updateMediaState(UpdateMediaState::Asynchronously);
#endif
}

void HTMLMediaElement::mediaPlayerFirstVideoFrameAvailable(MediaPlayer*)
{
    LOG(Media, "HTMLMediaElement::mediaPlayerFirstVideoFrameAvailable(%p) - current display mode = %i", this, (int)displayMode());

    beginProcessingMediaPlayerCallback();
    if (displayMode() == PosterWaitingForVideo) {
        setDisplayMode(Video);
        mediaPlayerRenderingModeChanged(m_player.get());
    }
    endProcessingMediaPlayerCallback();
}

void HTMLMediaElement::mediaPlayerCharacteristicChanged(MediaPlayer*)
{
    LOG(Media, "HTMLMediaElement::mediaPlayerCharacteristicChanged(%p)", this);
    
    beginProcessingMediaPlayerCallback();

#if ENABLE(VIDEO_TRACK)
    if (m_captionDisplayMode == CaptionUserPreferences::Automatic && m_subtitleTrackLanguage != m_player->languageOfPrimaryAudioTrack())
        markCaptionAndSubtitleTracksAsUnconfigured(AfterDelay);
#endif

    if (potentiallyPlaying() && displayMode() == PosterWaitingForVideo) {
        setDisplayMode(Video);
        mediaPlayerRenderingModeChanged(m_player.get());
    }

    if (hasMediaControls())
        mediaControls()->reset();
    if (renderer())
        renderer()->updateFromElement();

    if (isPlaying() && !m_mediaSession->playbackPermitted(*this))
        pauseInternal();

    document().updateIsPlayingMedia();

    endProcessingMediaPlayerCallback();
}

#if PLATFORM(WKC)
void HTMLMediaElement::mediaPlayerSeekBegin(MediaPlayer*)
{
    LOG(Media, "HTMLMediaElement::mediaPlayerSeekBegin");

    if (!m_player)
        return;

    beginProcessingMediaPlayerCallback();
    seekBegin();
    endProcessingMediaPlayerCallback();
}

void HTMLMediaElement::mediaPlayerSeekEnd(MediaPlayer*)
{
    LOG(Media, "HTMLMediaElement::mediaPlayerSeekEnd");

    if (!m_player)
        return;

    beginProcessingMediaPlayerCallback();
    seekEnd();
    endProcessingMediaPlayerCallback();
}

void HTMLMediaElement::mediaPlayerResetUserGestureRestriction(MediaPlayer*)
{
    LOG(Media, "HTMLMediaElement::mediaPlayerResetUserGestureRestriction");

    if (!m_player)
        return;

    beginProcessingMediaPlayerCallback();
    m_mediaSession->addBehaviorRestriction(MediaElementSession::RequireUserGestureForRateChange);
    m_playRequested = false;
    endProcessingMediaPlayerCallback();
}

void HTMLMediaElement::mediaPlayerRemoveUserGestureRestriction(MediaPlayer*)
{
    LOG(Media, "HTMLMediaElement::mediaPlayerRemoveUserGestureRestriction");

    if (!m_player)
        return;

    beginProcessingMediaPlayerCallback();
    m_mediaSession->removeBehaviorRestriction(MediaElementSession::RequireUserGestureForRateChange);
    endProcessingMediaPlayerCallback();
}

bool HTMLMediaElement::canPrepareForLoad()
{
    if (m_formatErrorSrc.isEmpty())
        return true;

    if (hasAttribute(srcAttr)) {
        URL mediaURL = getNonEmptyURLAttribute(srcAttr);
        if (m_formatErrorSrc == mediaURL)
            return false;
    } else {
        // Save current values.
        RefPtr<HTMLSourceElement> currentSourceNode = m_currentSourceNode;
        RefPtr<Node> nextNode = m_nextChildNodeToConsider;

        m_currentSourceNode = nullptr;
        m_nextChildNodeToConsider = firstChild();

        URL nextURL;
        do {
            nextURL = selectNextSourceChild(0, 0, DoNothing);
        } while (m_formatErrorSrc == nextURL);

        // Restore.
        m_currentSourceNode = currentSourceNode;
        m_nextChildNodeToConsider = nextNode;

        if (!nextURL.isValid())
            return false;
    }

    return true;
}
#endif

PassRefPtr<TimeRanges> HTMLMediaElement::buffered() const
{
    if (!m_player)
        return TimeRanges::create();

#if ENABLE(MEDIA_SOURCE)
    if (m_mediaSource)
        return TimeRanges::create(*m_mediaSource->buffered());
#endif

    return TimeRanges::create(*m_player->buffered());
}

double HTMLMediaElement::maxBufferedTime() const
{
    RefPtr<TimeRanges> bufferedRanges = buffered();
    unsigned numRanges = bufferedRanges->length();
    if (!numRanges)
        return 0;

    return bufferedRanges->end(numRanges - 1, ASSERT_NO_EXCEPTION);
}

PassRefPtr<TimeRanges> HTMLMediaElement::played()
{
    if (m_playing) {
        MediaTime time = currentMediaTime();
        if (time > m_lastSeekTime)
            addPlayedRange(m_lastSeekTime, time);
    }

    if (!m_playedTimeRanges)
        m_playedTimeRanges = TimeRanges::create();

    return m_playedTimeRanges->copy();
}

PassRefPtr<TimeRanges> HTMLMediaElement::seekable() const
{
    if (m_player)
        return TimeRanges::create(*m_player->seekable());

    return TimeRanges::create();
}

bool HTMLMediaElement::potentiallyPlaying() const
{
    if (isBlockedOnMediaController())
        return false;
    
    if (!couldPlayIfEnoughData())
        return false;

    if (m_readyState >= HAVE_FUTURE_DATA)
        return true;

    return m_readyStateMaximum >= HAVE_FUTURE_DATA && m_readyState < HAVE_FUTURE_DATA;
}

bool HTMLMediaElement::couldPlayIfEnoughData() const
{
    if (paused())
        return false;

    if (endedPlayback())
        return false;

    if (stoppedDueToErrors())
        return false;

    if (pausedForUserInteraction())
        return false;

    return true;
}

bool HTMLMediaElement::endedPlayback() const
{
    MediaTime dur = durationMediaTime();
    if (!m_player || !dur.isValid())
        return false;

    // 4.8.10.8 Playing the media resource

    // A media element is said to have ended playback when the element's 
    // readyState attribute is HAVE_METADATA or greater, 
    if (m_readyState < HAVE_METADATA)
        return false;

    // and the current playback position is the end of the media resource and the direction
    // of playback is forwards, Either the media element does not have a loop attribute specified,
    // or the media element has a current media controller.
    MediaTime now = currentMediaTime();
    if (requestedPlaybackRate() > 0)
        return dur > MediaTime::zeroTime() && now >= dur && (!loop() || m_mediaController);

    // or the current playback position is the earliest possible position and the direction 
    // of playback is backwards
    if (requestedPlaybackRate() < 0)
        return now <= MediaTime::zeroTime();

    return false;
}

bool HTMLMediaElement::stoppedDueToErrors() const
{
    if (m_readyState >= HAVE_METADATA && m_error) {
#if PLATFORM(WKC)
        if (m_error->code() == MediaError::MEDIA_ERR_NETWORK || m_error->code() == MediaError::MEDIA_ERR_DECODE)
            return false;
#endif
        RefPtr<TimeRanges> seekableRanges = seekable();
        if (!seekableRanges->contain(currentTime()))
            return true;
    }
    
    return false;
}

bool HTMLMediaElement::pausedForUserInteraction() const
{
    if (m_mediaSession->state() == PlatformMediaSession::Interrupted)
        return true;

    return false;
}

MediaTime HTMLMediaElement::minTimeSeekable() const
{
    return m_player ? m_player->minTimeSeekable() : MediaTime::zeroTime();
}

MediaTime HTMLMediaElement::maxTimeSeekable() const
{
    return m_player ? m_player->maxTimeSeekable() : MediaTime::zeroTime();
}
    
void HTMLMediaElement::updateVolume()
{
#if PLATFORM(IOS)
    // Only the user can change audio volume so update the cached volume and post the changed event.
    float volume = m_player->volume();
    if (m_volume != volume) {
        m_volume = volume;
        scheduleEvent(eventNames().volumechangeEvent);
    }
#else
    if (!m_player)
        return;

    // Avoid recursion when the player reports volume changes.
    if (!processingMediaPlayerCallback()) {
        Page* page = document().page();
        double volumeMultiplier = page ? page->mediaVolume() : 1;
        bool shouldMute = effectiveMuted();

        if (m_mediaController) {
            volumeMultiplier *= m_mediaController->volume();
            shouldMute = m_mediaController->muted() || (page && page->isMuted());
        }

        m_player->setMuted(shouldMute);
        m_player->setVolume(m_volume * volumeMultiplier);
    }

    if (hasMediaControls())
        mediaControls()->changedVolume();
#endif
}

void HTMLMediaElement::updatePlayState()
{
    if (!m_player)
        return;

    if (m_pausedInternal) {
        if (!m_player->paused())
            m_player->pause();
        refreshCachedTime();
        m_playbackProgressTimer.stop();
        if (hasMediaControls())
            mediaControls()->playbackStopped();
        m_activityToken = nullptr;
        return;
    }
    
    bool shouldBePlaying = potentiallyPlaying();
    bool playerPaused = m_player->paused();

    LOG(Media, "HTMLMediaElement::updatePlayState(%p) - shouldBePlaying = %s, playerPaused = %s", this, boolString(shouldBePlaying), boolString(playerPaused));

    if (shouldBePlaying) {
        setDisplayMode(Video);
        invalidateCachedTime();

        if (playerPaused) {
            m_mediaSession->clientWillBeginPlayback();

            if (m_mediaSession->requiresFullscreenForVideoPlayback(*this) && !isFullscreen())
                enterFullscreen();

            // Set rate, muted before calling play in case they were set before the media engine was setup.
            // The media engine should just stash the rate and muted values since it isn't already playing.
            m_player->setRate(requestedPlaybackRate());
            m_player->setMuted(effectiveMuted());

            if (m_firstTimePlaying) {
                // Log that a media element was played.
                if (Frame* frame = document().frame())
                    frame->mainFrame().diagnosticLoggingClient().logDiagnosticMessageWithValue(DiagnosticLoggingKeys::mediaKey(), isVideo() ? DiagnosticLoggingKeys::videoKey() : DiagnosticLoggingKeys::audioKey(), DiagnosticLoggingKeys::playedKey(), ShouldSample::No);
                m_firstTimePlaying = false;
            }

            m_player->play();
        }

        if (hasMediaControls())
            mediaControls()->playbackStarted();
        if (document().page())
            m_activityToken = document().page()->pageThrottler().mediaActivityToken();

        startPlaybackProgressTimer();
        setPlaying(true);
    } else {
        if (!playerPaused)
            m_player->pause();
        refreshCachedTime();

        m_playbackProgressTimer.stop();
        setPlaying(false);
        MediaTime time = currentMediaTime();
        if (time > m_lastSeekTime)
            addPlayedRange(m_lastSeekTime, time);

        if (couldPlayIfEnoughData())
            prepareToPlay();

        if (hasMediaControls())
            mediaControls()->playbackStopped();
        m_activityToken = nullptr;
    }
    
    updateMediaController();

    if (renderer())
        renderer()->updateFromElement();
}

void HTMLMediaElement::setPlaying(bool playing)
{
    if (m_playing == playing)
        return;

    m_playing = playing;
    document().updateIsPlayingMedia();

#if ENABLE(WIRELESS_PLAYBACK_TARGET)
    updateMediaState(UpdateMediaState::Asynchronously);
#endif
}

void HTMLMediaElement::setPausedInternal(bool b)
{
    m_pausedInternal = b;
    updatePlayState();
}

void HTMLMediaElement::stopPeriodicTimers()
{
    m_progressEventTimer.stop();
    m_playbackProgressTimer.stop();
}

void HTMLMediaElement::userCancelledLoad()
{
    LOG(Media, "HTMLMediaElement::userCancelledLoad(%p)", this);

    // FIXME: We should look to reconcile the iOS and non-iOS code (below).
#if PLATFORM(IOS)
    if (m_networkState == NETWORK_EMPTY || m_readyState >= HAVE_METADATA)
        return;
#else
    if (m_networkState == NETWORK_EMPTY || m_completelyLoaded)
        return;
#endif

    // If the media data fetching process is aborted by the user:

    // 1 - The user agent should cancel the fetching process.
    clearMediaPlayer(EveryDelayedAction);

    // 2 - Set the error attribute to a new MediaError object whose code attribute is set to MEDIA_ERR_ABORTED.
    m_error = MediaError::create(MediaError::MEDIA_ERR_ABORTED);

    // 3 - Queue a task to fire a simple event named error at the media element.
    scheduleEvent(eventNames().abortEvent);

#if ENABLE(MEDIA_SOURCE)
    closeMediaSource();
#endif

    // 4 - If the media element's readyState attribute has a value equal to HAVE_NOTHING, set the 
    // element's networkState attribute to the NETWORK_EMPTY value and queue a task to fire a 
    // simple event named emptied at the element. Otherwise, set the element's networkState 
    // attribute to the NETWORK_IDLE value.
    if (m_readyState == HAVE_NOTHING) {
        m_networkState = NETWORK_EMPTY;
        scheduleEvent(eventNames().emptiedEvent);
    }
    else
        m_networkState = NETWORK_IDLE;

    // 5 - Set the element's delaying-the-load-event flag to false. This stops delaying the load event.
    setShouldDelayLoadEvent(false);

    // 6 - Abort the overall resource selection algorithm.
    m_currentSourceNode = nullptr;

    // Reset m_readyState since m_player is gone.
    m_readyState = HAVE_NOTHING;
    updateMediaController();
#if ENABLE(VIDEO_TRACK)
    if (RuntimeEnabledFeatures::sharedFeatures().webkitVideoTrackEnabled())
        updateActiveTextTrackCues(MediaTime::zeroTime());
#endif
}

void HTMLMediaElement::clearMediaPlayer(DelayedActionType flags)
{
    LOG(Media, "HTMLMediaElement::clearMediaPlayer(%p) - flags = %s", this, actionName(flags).utf8().data());

#if USE(PLATFORM_TEXT_TRACK_MENU)
    if (platformTextTrackMenu()) {
        m_platformMenu->setClient(nullptr);
        m_platformMenu = nullptr;
    }
#endif

#if ENABLE(VIDEO_TRACK)
    forgetResourceSpecificTracks();
#endif

#if ENABLE(MEDIA_SOURCE)
    closeMediaSource();
#endif

#if ENABLE(WIRELESS_PLAYBACK_TARGET)
    if (hasEventListeners(eventNames().webkitplaybacktargetavailabilitychangedEvent)) {
        m_hasPlaybackTargetAvailabilityListeners = false;
        m_mediaSession->setHasPlaybackTargetAvailabilityListeners(*this, false);

        // Send an availability event in case scripts want to hide the picker when the element
        // doesn't support playback to a target.
        enqueuePlaybackTargetAvailabilityChangedEvent();
    }
#endif

    m_player = nullptr;

    stopPeriodicTimers();
    m_pendingActionTimer.stop();

    clearFlags(m_pendingActionFlags, flags);
    m_loadState = WaitingForSource;

#if ENABLE(VIDEO_TRACK)
    if (m_textTracks)
        configureTextTrackDisplay();
#endif

    updateSleepDisabling();
}

bool HTMLMediaElement::canSuspendForPageCache() const
{
    return true; 
}

const char* HTMLMediaElement::activeDOMObjectName() const
{
    return "HTMLMediaElement";
}

void HTMLMediaElement::stopWithoutDestroyingMediaPlayer()
{
    LOG(Media, "HTMLMediaElement::stopWithoutDestroyingMediaPlayer(%p)", this);

#if PLATFORM(WKC)
    m_mediaSession->addBehaviorRestriction(MediaElementSession::RequireUserGestureForRateChange);
    m_playRequested = false;
    if (m_player)
        m_player->willBecomeInactive();
#endif
    if (m_videoFullscreenMode != VideoFullscreenModeNone)
        exitFullscreen();
    
    m_inActiveDocument = false;

    // Stop the playback without generating events
    setPlaying(false);
    setPausedInternal(true);
    m_mediaSession->clientWillPausePlayback();

    userCancelledLoad();

    if (renderer())
        renderer()->updateFromElement();

#if PLATFORM(WKC)
    // Make sure the media controls are properly updated on resume.
    if (renderer())
        renderer()->setNeedsLayout();
#endif

    stopPeriodicTimers();

    updateSleepDisabling();

#if PLATFORM(WKC)
    // Ensure m_player is cleared.
    m_player.reset();
#endif
}

void HTMLMediaElement::contextDestroyed()
{
    m_seekTaskQueue.close();
    m_resizeTaskQueue.close();
    m_shadowDOMTaskQueue.close();

    ActiveDOMObject::contextDestroyed();
}

void HTMLMediaElement::stop()
{
    LOG(Media, "HTMLMediaElement::stop(%p)", this);

    stopWithoutDestroyingMediaPlayer();

    m_asyncEventQueue.close();

    // Once an active DOM object has been stopped it can not be restarted, so we can deallocate
    // the media player now. Note that userCancelledLoad will already called clearMediaPlayer
    // if the media was not fully loaded, but we need the same cleanup if the file was completely
    // loaded and calling it again won't cause any problems.
    clearMediaPlayer(EveryDelayedAction);
}

void HTMLMediaElement::suspend(ReasonForSuspension why)
{
    LOG(Media, "HTMLMediaElement::suspend(%p)", this);

    switch (why)
    {
        case PageCache:
            stopWithoutDestroyingMediaPlayer();
            m_asyncEventQueue.suspend();
            setShouldBufferData(false);
            m_mediaSession->addBehaviorRestriction(MediaElementSession::RequirePageConsentToResumeMedia);
            break;
        case DocumentWillBePaused:
        case JavaScriptDebuggerPaused:
        case PageWillBeSuspended:
        case WillDeferLoading:
            // Do nothing, we don't pause media playback in these cases.
            break;
    }
}

void HTMLMediaElement::resume()
{
    LOG(Media, "HTMLMediaElement::resume(%p)", this);

    m_inActiveDocument = true;

    m_asyncEventQueue.resume();

    setShouldBufferData(true);

    if (!m_mediaSession->pageAllowsPlaybackAfterResuming(*this))
        document().addMediaCanStartListener(this);
    else
        setPausedInternal(false);

    m_mediaSession->removeBehaviorRestriction(MediaElementSession::RequirePageConsentToResumeMedia);

    if (m_error && m_error->code() == MediaError::MEDIA_ERR_ABORTED) {
        // Restart the load if it was aborted in the middle by moving the document to the page cache.
        // m_error is only left at MEDIA_ERR_ABORTED when the document becomes inactive (it is set to
        //  MEDIA_ERR_ABORTED while the abortEvent is being sent, but cleared immediately afterwards).
        // This behavior is not specified but it seems like a sensible thing to do.
        // As it is not safe to immedately start loading now, let's schedule a load.
        scheduleDelayedAction(LoadMediaResource);
    }

    if (renderer())
        renderer()->updateFromElement();
}

bool HTMLMediaElement::hasPendingActivity() const
{
    return (hasAudio() && isPlaying()) || m_asyncEventQueue.hasPendingEvents() || m_creatingControls;
}

void HTMLMediaElement::mediaVolumeDidChange()
{
    LOG(Media, "HTMLMediaElement::mediaVolumeDidChange(%p)", this);
    updateVolume();
}

void HTMLMediaElement::visibilityStateChanged()
{
    LOG(Media, "HTMLMediaElement::visibilityStateChanged(%p)", this);
    m_elementIsHidden = document().hidden();
    updateSleepDisabling();
    m_mediaSession->visibilityChanged();
}

#if ENABLE(VIDEO_TRACK)
bool HTMLMediaElement::requiresTextTrackRepresentation() const
{
    return (m_videoFullscreenMode != VideoFullscreenModeNone) && m_player ? m_player->requiresTextTrackRepresentation() : false;
}

void HTMLMediaElement::setTextTrackRepresentation(TextTrackRepresentation* representation)
{
    if (m_player)
        m_player->setTextTrackRepresentation(representation);
}

void HTMLMediaElement::syncTextTrackBounds()
{
    if (m_player)
        m_player->syncTextTrackBounds();
}
#endif // ENABLE(VIDEO_TRACK)

#if ENABLE(WIRELESS_PLAYBACK_TARGET)
void HTMLMediaElement::webkitShowPlaybackTargetPicker()
{
    LOG(Media, "HTMLMediaElement::webkitShowPlaybackTargetPicker(%p)", this);
    m_mediaSession->showPlaybackTargetPicker(*this);
}

bool HTMLMediaElement::webkitCurrentPlaybackTargetIsWireless() const
{
    return m_player && m_player->isCurrentPlaybackTargetWireless();
}

void HTMLMediaElement::wirelessRoutesAvailableDidChange()
{
    enqueuePlaybackTargetAvailabilityChangedEvent();
}

void HTMLMediaElement::mediaPlayerCurrentPlaybackTargetIsWirelessChanged(MediaPlayer*)
{
    LOG(Media, "HTMLMediaElement::mediaPlayerCurrentPlaybackTargetIsWirelessChanged(%p) - webkitCurrentPlaybackTargetIsWireless = %s", this, boolString(webkitCurrentPlaybackTargetIsWireless()));
    ASSERT(m_player);
    configureMediaControls();
    scheduleEvent(eventNames().webkitcurrentplaybacktargetiswirelesschangedEvent);
    m_mediaSession->isPlayingToWirelessPlaybackTargetChanged(m_player->isCurrentPlaybackTargetWireless());
    updateMediaState(UpdateMediaState::Asynchronously);
}

bool HTMLMediaElement::dispatchEvent(PassRefPtr<Event> prpEvent)
{
    RefPtr<Event> event = prpEvent;
    if (event->type() == eventNames().webkitcurrentplaybacktargetiswirelesschangedEvent)
        scheduleDelayedAction(CheckPlaybackTargetCompatablity);
    return HTMLElement::dispatchEvent(event);
}

bool HTMLMediaElement::addEventListener(const AtomicString& eventType, PassRefPtr<EventListener> listener, bool useCapture)
{
    if (eventType != eventNames().webkitplaybacktargetavailabilitychangedEvent)
        return Node::addEventListener(eventType, listener, useCapture);

    bool isFirstAvailabilityChangedListener = !hasEventListeners(eventNames().webkitplaybacktargetavailabilitychangedEvent);
    if (!Node::addEventListener(eventType, listener, useCapture))
        return false;

    if (isFirstAvailabilityChangedListener) {
        m_hasPlaybackTargetAvailabilityListeners = true;
        m_mediaSession->setHasPlaybackTargetAvailabilityListeners(*this, true);
    }

    LOG(Media, "HTMLMediaElement::addEventListener(%p) - 'webkitplaybacktargetavailabilitychanged'", this);
    
    enqueuePlaybackTargetAvailabilityChangedEvent(); // Ensure the event listener gets at least one event.
    return true;
}

bool HTMLMediaElement::removeEventListener(const AtomicString& eventType, EventListener* listener, bool useCapture)
{
    if (eventType != eventNames().webkitplaybacktargetavailabilitychangedEvent)
        return Node::removeEventListener(eventType, listener, useCapture);

    if (!Node::removeEventListener(eventType, listener, useCapture))
        return false;

    bool didRemoveLastAvailabilityChangedListener = !hasEventListeners(eventNames().webkitplaybacktargetavailabilitychangedEvent);
    LOG(Media, "HTMLMediaElement::removeEventListener(%p) - removed last listener = %s", this, boolString(didRemoveLastAvailabilityChangedListener));
    if (didRemoveLastAvailabilityChangedListener) {
        m_hasPlaybackTargetAvailabilityListeners = false;
        m_mediaSession->setHasPlaybackTargetAvailabilityListeners(*this, false);
        updateMediaState(UpdateMediaState::Asynchronously);
    }

    return true;
}

void HTMLMediaElement::enqueuePlaybackTargetAvailabilityChangedEvent()
{
    bool hasTargets = m_mediaSession->hasWirelessPlaybackTargets(*this);
    LOG(Media, "HTMLMediaElement::enqueuePlaybackTargetAvailabilityChangedEvent(%p) - hasTargets = %s", this, boolString(hasTargets));
    RefPtr<Event> event = WebKitPlaybackTargetAvailabilityEvent::create(eventNames().webkitplaybacktargetavailabilitychangedEvent, hasTargets);
    event->setTarget(this);
    m_asyncEventQueue.enqueueEvent(event.release());
    updateMediaState(UpdateMediaState::Asynchronously);
}

void HTMLMediaElement::setWirelessPlaybackTarget(Ref<MediaPlaybackTarget>&& device)
{
    LOG(Media, "HTMLMediaElement::setWirelessPlaybackTarget(%p)", this);
    if (m_player)
        m_player->setWirelessPlaybackTarget(WTF::move(device));
}

bool HTMLMediaElement::canPlayToWirelessPlaybackTarget() const
{
    bool canPlay = m_player && m_player->canPlayToWirelessPlaybackTarget();

    LOG(Media, "HTMLMediaElement::canPlayToWirelessPlaybackTarget(%p) - returning %s", this, boolString(canPlay));

    return canPlay;
}

bool HTMLMediaElement::isPlayingToWirelessPlaybackTarget() const
{
    bool isPlaying = m_player && m_player->isCurrentPlaybackTargetWireless();

    LOG(Media, "HTMLMediaElement::isPlayingToWirelessPlaybackTarget(%p) - returning %s", this, boolString(isPlaying));
    
    return isPlaying;
}

void HTMLMediaElement::setShouldPlayToPlaybackTarget(bool shouldPlay)
{
    LOG(Media, "HTMLMediaElement::setShouldPlayToPlaybackTarget(%p) - shouldPlay = %s", this, boolString(shouldPlay));

    if (m_player)
        m_player->setShouldPlayToPlaybackTarget(shouldPlay);
}
#else // ENABLE(WIRELESS_PLAYBACK_TARGET)

bool HTMLMediaElement::webkitCurrentPlaybackTargetIsWireless() const
{
    return false;
}

#endif // ENABLE(WIRELESS_PLAYBACK_TARGET)

double HTMLMediaElement::minFastReverseRate() const
{
    return m_player ? m_player->minFastReverseRate() : 0;
}

double HTMLMediaElement::maxFastForwardRate() const
{
    return m_player ? m_player->maxFastForwardRate() : 0;
}
    
bool HTMLMediaElement::isFullscreen() const
{
    if (m_videoFullscreenMode != VideoFullscreenModeNone)
        return true;
    
#if ENABLE(FULLSCREEN_API)
    if (document().webkitIsFullScreen() && document().webkitCurrentFullScreenElement() == this)
        return true;
#endif
    
    return false;
}

void HTMLMediaElement::toggleFullscreenState()
{
    LOG(Media, "HTMLMediaElement::toggleFullscreenState(%p) - isFullscreen() is %s", this, boolString(isFullscreen()));
    
    if (isFullscreen())
        exitFullscreen();
    else
        enterFullscreen();
}

void HTMLMediaElement::enterFullscreen(VideoFullscreenMode mode)
{
    LOG(Media, "HTMLMediaElement::enterFullscreen(%p)", this);
    ASSERT(mode != VideoFullscreenModeNone);

    if (m_videoFullscreenMode == mode)
        return;
    if (!document().page() || !document().page()->chrome().client().isViewVisible()) {
        return;
    }

#if ENABLE(FULLSCREEN_API)
    if (document().settings() && document().settings()->fullScreenEnabled()) {
        document().requestFullScreenForElement(this, 0, Document::ExemptIFrameAllowFullScreenRequirement);
        return;
    }
#endif

    fullscreenModeChanged(mode);
    if (hasMediaControls())
        mediaControls()->enteredFullscreen();
    if (is<HTMLVideoElement>(*this)) {
        HTMLVideoElement& asVideo = downcast<HTMLVideoElement>(*this);
        if (document().page()->chrome().client().supportsVideoFullscreen()) {
            document().page()->chrome().client().enterVideoFullscreenForVideoElement(asVideo, m_videoFullscreenMode);
            scheduleEvent(eventNames().webkitbeginfullscreenEvent);
        }
    }
}

void HTMLMediaElement::enterFullscreen()
{
    enterFullscreen(VideoFullscreenModeStandard);
}

void HTMLMediaElement::exitFullscreen()
{
    LOG(Media, "HTMLMediaElement::exitFullscreen(%p)", this);

#if ENABLE(FULLSCREEN_API)
    if (document().settings() && document().settings()->fullScreenEnabled()) {
        if (document().webkitIsFullScreen() && document().webkitCurrentFullScreenElement() == this)
            document().webkitCancelFullScreen();

        if (m_videoFullscreenMode == VideoFullscreenModeStandard)
            return;
    }
#endif
    ASSERT(m_videoFullscreenMode != VideoFullscreenModeNone);
    fullscreenModeChanged(VideoFullscreenModeNone);
    if (hasMediaControls())
        mediaControls()->exitedFullscreen();
    if (document().page() && is<HTMLVideoElement>(*this)) {
        if (m_mediaSession->requiresFullscreenForVideoPlayback(*this))
            pauseInternal();

        if (document().page()->chrome().client().supportsVideoFullscreen()) {
            document().page()->chrome().client().exitVideoFullscreenForVideoElement(downcast<HTMLVideoElement>(*this));
            scheduleEvent(eventNames().webkitendfullscreenEvent);
        }
    }
}

void HTMLMediaElement::didBecomeFullscreenElement()
{
    if (hasMediaControls())
        mediaControls()->enteredFullscreen();
}

void HTMLMediaElement::willStopBeingFullscreenElement()
{
    if (hasMediaControls())
        mediaControls()->exitedFullscreen();
}

PlatformMedia HTMLMediaElement::platformMedia() const
{
    return m_player ? m_player->platformMedia() : NoPlatformMedia;
}

PlatformLayer* HTMLMediaElement::platformLayer() const
{
    return m_player ? m_player->platformLayer() : nullptr;
}

#if PLATFORM(IOS)
void HTMLMediaElement::setVideoFullscreenLayer(PlatformLayer* platformLayer)
{
    m_videoFullscreenLayer = platformLayer;
    if (!m_player)
        return;
    
    m_player->setVideoFullscreenLayer(platformLayer);
    setNeedsStyleRecalc(SyntheticStyleChange);
#if ENABLE(VIDEO_TRACK)
    if (RuntimeEnabledFeatures::sharedFeatures().webkitVideoTrackEnabled())
        updateTextTrackDisplay();
#endif
}
    
void HTMLMediaElement::setVideoFullscreenFrame(FloatRect frame)
{
    m_videoFullscreenFrame = frame;
    if (m_player)
        m_player->setVideoFullscreenFrame(frame);
}

void HTMLMediaElement::setVideoFullscreenGravity(MediaPlayer::VideoGravity gravity)
{
    m_videoFullscreenGravity = gravity;
    if (m_player)
        m_player->setVideoFullscreenGravity(gravity);
}
#endif

bool HTMLMediaElement::hasClosedCaptions() const
{
    if (m_player && m_player->hasClosedCaptions())
        return true;

#if ENABLE(VIDEO_TRACK)
    if (!RuntimeEnabledFeatures::sharedFeatures().webkitVideoTrackEnabled() || !m_textTracks)
        return false;

    for (unsigned i = 0; i < m_textTracks->length(); ++i) {
        if (m_textTracks->item(i)->readinessState() == TextTrack::FailedToLoad)
            continue;

        if (m_textTracks->item(i)->kind() == TextTrack::captionsKeyword()
            || m_textTracks->item(i)->kind() == TextTrack::subtitlesKeyword())
            return true;
    }
#endif

    return false;
}

bool HTMLMediaElement::closedCaptionsVisible() const
{
    return m_closedCaptionsVisible;
}

#if ENABLE(VIDEO_TRACK)

void HTMLMediaElement::updateTextTrackDisplay()
{
#if ENABLE(MEDIA_CONTROLS_SCRIPT)
    ensureMediaControlsShadowRoot();
    ASSERT(m_mediaControlsHost);
    m_mediaControlsHost->updateTextTrackContainer();
#else
    if (!hasMediaControls() && !createMediaControls())
        return;
    
    mediaControls()->updateTextTrackDisplay();
#endif
}

#endif

void HTMLMediaElement::setClosedCaptionsVisible(bool closedCaptionVisible)
{
    LOG(Media, "HTMLMediaElement::setClosedCaptionsVisible(%p) - %s", this, boolString(closedCaptionVisible));

    m_closedCaptionsVisible = false;

    if (!m_player || !hasClosedCaptions())
        return;

    m_closedCaptionsVisible = closedCaptionVisible;
    m_player->setClosedCaptionsVisible(closedCaptionVisible);

#if ENABLE(VIDEO_TRACK)
    if (RuntimeEnabledFeatures::sharedFeatures().webkitVideoTrackEnabled()) {
        markCaptionAndSubtitleTracksAsUnconfigured(Immediately);
        updateTextTrackDisplay();
    }
#else
    if (hasMediaControls())
        mediaControls()->changedClosedCaptionsVisibility();
#endif
}

void HTMLMediaElement::setWebkitClosedCaptionsVisible(bool visible)
{
    m_webkitLegacyClosedCaptionOverride = visible;
    setClosedCaptionsVisible(visible);
}

bool HTMLMediaElement::webkitClosedCaptionsVisible() const
{
    return m_webkitLegacyClosedCaptionOverride && m_closedCaptionsVisible;
}


bool HTMLMediaElement::webkitHasClosedCaptions() const
{
    return hasClosedCaptions();
}

#if ENABLE(MEDIA_STATISTICS)
unsigned HTMLMediaElement::webkitAudioDecodedByteCount() const
{
    if (!m_player)
        return 0;
    return m_player->audioDecodedByteCount();
}

unsigned HTMLMediaElement::webkitVideoDecodedByteCount() const
{
    if (!m_player)
        return 0;
    return m_player->videoDecodedByteCount();
}
#endif

void HTMLMediaElement::mediaCanStart()
{
    LOG(Media, "HTMLMediaElement::mediaCanStart(%p)", this);

    ASSERT(m_isWaitingUntilMediaCanStart || m_pausedInternal);
    if (m_isWaitingUntilMediaCanStart) {
        m_isWaitingUntilMediaCanStart = false;
        loadInternal();
    }
    if (m_pausedInternal)
        setPausedInternal(false);
}

bool HTMLMediaElement::isURLAttribute(const Attribute& attribute) const
{
    return attribute.name() == srcAttr || HTMLElement::isURLAttribute(attribute);
}

void HTMLMediaElement::setShouldDelayLoadEvent(bool shouldDelay)
{
    if (m_shouldDelayLoadEvent == shouldDelay)
        return;

    LOG(Media, "HTMLMediaElement::setShouldDelayLoadEvent(%p) - %s", this, boolString(shouldDelay));

    m_shouldDelayLoadEvent = shouldDelay;
    if (shouldDelay)
        document().incrementLoadEventDelayCount();
    else
        document().decrementLoadEventDelayCount();
}
    

void HTMLMediaElement::getSitesInMediaCache(Vector<String>& sites)
{
    MediaPlayer::getSitesInMediaCache(sites);
}

void HTMLMediaElement::clearMediaCache()
{
    MediaPlayer::clearMediaCache();
}

void HTMLMediaElement::clearMediaCacheForSite(const String& site)
{
    MediaPlayer::clearMediaCacheForSite(site);
}

void HTMLMediaElement::resetMediaEngines()
{
    MediaPlayer::resetMediaEngines();
}

void HTMLMediaElement::privateBrowsingStateDidChange()
{
    if (!m_player)
        return;

    bool privateMode = document().page() && document().page()->usesEphemeralSession();
    LOG(Media, "HTMLMediaElement::privateBrowsingStateDidChange(%p) - %s", this, boolString(privateMode));
    m_player->setPrivateBrowsingMode(privateMode);
}

MediaControls* HTMLMediaElement::mediaControls() const
{
#if ENABLE(MEDIA_CONTROLS_SCRIPT)
    return nullptr;
#else
    ShadowRoot* root = userAgentShadowRoot();
    if (!root)
        return nullptr;
    
    return childrenOfType<MediaControls>(*root).first();
#endif
}

bool HTMLMediaElement::hasMediaControls() const
{
#if ENABLE(MEDIA_CONTROLS_SCRIPT)
    return false;
#else

    if (ShadowRoot* userAgent = userAgentShadowRoot()) {
        Node* node = childrenOfType<MediaControls>(*root).first();
        ASSERT_WITH_SECURITY_IMPLICATION(!node || node->isMediaControls());
        return node;
    }

    return false;
#endif
}

bool HTMLMediaElement::createMediaControls()
{
#if ENABLE(MEDIA_CONTROLS_SCRIPT)
    ensureMediaControlsShadowRoot();
    return false;
#else
    if (hasMediaControls())
        return true;

    RefPtr<MediaControls> mediaControls = MediaControls::create(document());
    if (!mediaControls)
        return false;

    mediaControls->setMediaController(m_mediaController ? m_mediaController.get() : static_cast<MediaControllerInterface*>(this));
    mediaControls->reset();
    if (isFullscreen())
        mediaControls->enteredFullscreen();

    ensureUserAgentShadowRoot().appendChild(mediaControls, ASSERT_NO_EXCEPTION);

    if (!controls() || !inDocument())
        mediaControls->hide();

    return true;
#endif
}

void HTMLMediaElement::configureMediaControls()
{
    bool requireControls = controls();

    // Always create controls for video when fullscreen playback is required.
    if (isVideo() && m_mediaSession->requiresFullscreenForVideoPlayback(*this))
        requireControls = true;

    // Always create controls when in full screen mode.
    if (isFullscreen())
        requireControls = true;

#if ENABLE(WIRELESS_PLAYBACK_TARGET)
    if (m_player && m_player->isCurrentPlaybackTargetWireless())
        requireControls = true;
#endif

#if ENABLE(MEDIA_CONTROLS_SCRIPT)
    if (!requireControls || !inDocument() || !inActiveDocument())
        return;

    ensureMediaControlsShadowRoot();
#else
    if (!requireControls || !inDocument() || !inActiveDocument()) {
        if (hasMediaControls())
            mediaControls()->hide();
        return;
    }

    if (!hasMediaControls() && !createMediaControls())
        return;

    mediaControls()->show();
#endif
}

#if ENABLE(VIDEO_TRACK)
void HTMLMediaElement::configureTextTrackDisplay(TextTrackVisibilityCheckType checkType)
{
#if PLATFORM(WKC)
    if (!m_textTracks)
        return;
#else
    ASSERT(m_textTracks);
#endif

    if (m_processingPreferenceChange)
        return;

    if (document().activeDOMObjectsAreStopped())
        return;

    bool haveVisibleTextTrack = false;
    for (unsigned i = 0; i < m_textTracks->length(); ++i) {
        if (m_textTracks->item(i)->mode() == TextTrack::showingKeyword()) {
            haveVisibleTextTrack = true;
            break;
        }
    }

    if (checkType == CheckTextTrackVisibility && m_haveVisibleTextTrack == haveVisibleTextTrack) {
        updateActiveTextTrackCues(currentMediaTime());
        return;
    }

    m_haveVisibleTextTrack = haveVisibleTextTrack;
    m_closedCaptionsVisible = m_haveVisibleTextTrack;

#if ENABLE(MEDIA_CONTROLS_SCRIPT)
    if (!m_haveVisibleTextTrack)
        return;

    ensureMediaControlsShadowRoot();
#else
    if (!m_haveVisibleTextTrack && !hasMediaControls())
        return;
    if (!hasMediaControls() && !createMediaControls())
        return;

    mediaControls()->changedClosedCaptionsVisibility();
    
    if (RuntimeEnabledFeatures::sharedFeatures().webkitVideoTrackEnabled()) {
        updateTextTrackDisplay();
        updateActiveTextTrackCues(currentMediaTime());
    }
#endif
}

void HTMLMediaElement::captionPreferencesChanged()
{
    if (!isVideo())
        return;

    if (hasMediaControls())
        mediaControls()->textTrackPreferencesChanged();

#if ENABLE(MEDIA_CONTROLS_SCRIPT)
    if (m_mediaControlsHost)
        m_mediaControlsHost->updateCaptionDisplaySizes();
#endif

    if (m_player)
        m_player->tracksChanged();

    if (!document().page())
        return;

    CaptionUserPreferences::CaptionDisplayMode displayMode = document().page()->group().captionPreferences()->captionDisplayMode();
    if (m_captionDisplayMode == displayMode)
        return;

    m_captionDisplayMode = displayMode;
    setWebkitClosedCaptionsVisible(m_captionDisplayMode == CaptionUserPreferences::AlwaysOn);
}

void HTMLMediaElement::markCaptionAndSubtitleTracksAsUnconfigured(ReconfigureMode mode)
{
    if (!m_textTracks)
        return;

    LOG(Media, "HTMLMediaElement::markCaptionAndSubtitleTracksAsUnconfigured(%p)", this);

    // Mark all tracks as not "configured" so that configureTextTracks()
    // will reconsider which tracks to display in light of new user preferences
    // (e.g. default tracks should not be displayed if the user has turned off
    // captions and non-default tracks should be displayed based on language
    // preferences if the user has turned captions on).
    for (unsigned i = 0; i < m_textTracks->length(); ++i) {
        
        RefPtr<TextTrack> textTrack = m_textTracks->item(i);
        String kind = textTrack->kind();

        if (kind == TextTrack::subtitlesKeyword() || kind == TextTrack::captionsKeyword())
            textTrack->setHasBeenConfigured(false);
    }

    m_processingPreferenceChange = true;
    clearFlags(m_pendingActionFlags, ConfigureTextTracks);
    if (mode == Immediately) {
        Ref<HTMLMediaElement> protectedThis(*this); // configureTextTracks calls methods that can trigger arbitrary DOM mutations.
        configureTextTracks();
    } else
        scheduleDelayedAction(ConfigureTextTracks);
}

#endif

void HTMLMediaElement::createMediaPlayer()
{
    LOG(Media, "HTMLMediaElement::createMediaPlayer(%p)", this);

#if ENABLE(WEB_AUDIO)
    if (m_audioSourceNode)
        m_audioSourceNode->lock();
#endif

#if ENABLE(MEDIA_SOURCE)
    if (m_mediaSource)
        m_mediaSource->close();
#endif

#if ENABLE(VIDEO_TRACK)
    forgetResourceSpecificTracks();
#endif
    m_player = std::make_unique<MediaPlayer>(static_cast<MediaPlayerClient&>(*this));

#if ENABLE(WEB_AUDIO)
    if (m_audioSourceNode) {
        // When creating the player, make sure its AudioSourceProvider knows about the MediaElementAudioSourceNode.
        if (audioSourceProvider())
            audioSourceProvider()->setClient(m_audioSourceNode);

        m_audioSourceNode->unlock();
    }
#endif

#if ENABLE(WIRELESS_PLAYBACK_TARGET)
    if (hasEventListeners(eventNames().webkitplaybacktargetavailabilitychangedEvent)) {
        m_hasPlaybackTargetAvailabilityListeners = true;
        m_mediaSession->setHasPlaybackTargetAvailabilityListeners(*this, true);
        enqueuePlaybackTargetAvailabilityChangedEvent(); // Ensure the event listener gets at least one event.
    }
#endif
}

#if ENABLE(WEB_AUDIO)
void HTMLMediaElement::setAudioSourceNode(MediaElementAudioSourceNode* sourceNode)
{
    m_audioSourceNode = sourceNode;

    if (audioSourceProvider())
        audioSourceProvider()->setClient(m_audioSourceNode);
}

AudioSourceProvider* HTMLMediaElement::audioSourceProvider()
{
    if (m_player)
        return m_player->audioSourceProvider();

    return 0;
}
#endif

const String& HTMLMediaElement::mediaGroup() const
{
    return m_mediaGroup;
}

void HTMLMediaElement::setMediaGroup(const String& group)
{
    if (m_mediaGroup == group)
        return;
    m_mediaGroup = group;

    // When a media element is created with a mediagroup attribute, and when a media element's mediagroup 
    // attribute is set, changed, or removed, the user agent must run the following steps:
    // 1. Let m [this] be the media element in question.
    // 2. Let m have no current media controller, if it currently has one.
    setController(0);

    // 3. If m's mediagroup attribute is being removed, then abort these steps.
    if (group.isNull() || group.isEmpty())
        return;

    // 4. If there is another media element whose Document is the same as m's Document (even if one or both
    // of these elements are not actually in the Document), 
    HashSet<HTMLMediaElement*> elements = documentToElementSetMap().get(&document());
    for (HashSet<HTMLMediaElement*>::iterator i = elements.begin(); i != elements.end(); ++i) {
        if (*i == this)
            continue;

        // and which also has a mediagroup attribute, and whose mediagroup attribute has the same value as
        // the new value of m's mediagroup attribute,        
        if ((*i)->mediaGroup() == group) {
            //  then let controller be that media element's current media controller.
            setController((*i)->controller());
            return;
        }
    }

    // Otherwise, let controller be a newly created MediaController.
    setController(MediaController::create(document()));
}

MediaController* HTMLMediaElement::controller() const
{
    return m_mediaController.get();
}

void HTMLMediaElement::setController(PassRefPtr<MediaController> controller)
{
    if (m_mediaController)
        m_mediaController->removeMediaElement(this);

    m_mediaController = controller;

    if (m_mediaController)
        m_mediaController->addMediaElement(this);

    if (hasMediaControls())
        mediaControls()->setMediaController(m_mediaController ? m_mediaController.get() : static_cast<MediaControllerInterface*>(this));
}

void HTMLMediaElement::updateMediaController()
{
    if (m_mediaController)
        m_mediaController->reportControllerState();
}

bool HTMLMediaElement::isBlocked() const
{
    // A media element is a blocked media element if its readyState attribute is in the
    // HAVE_NOTHING state, the HAVE_METADATA state, or the HAVE_CURRENT_DATA state,
    if (m_readyState <= HAVE_CURRENT_DATA)
        return true;

    // or if the element has paused for user interaction.
    return pausedForUserInteraction();
}

bool HTMLMediaElement::isBlockedOnMediaController() const
{
    if (!m_mediaController)
        return false;

    // A media element is blocked on its media controller if the MediaController is a blocked 
    // media controller,
    if (m_mediaController->isBlocked())
        return true;

    // or if its media controller position is either before the media resource's earliest possible 
    // position relative to the MediaController's timeline or after the end of the media resource 
    // relative to the MediaController's timeline.
    double mediaControllerPosition = m_mediaController->currentTime();
    if (mediaControllerPosition < 0 || mediaControllerPosition > duration())
        return true;

    return false;
}

void HTMLMediaElement::prepareMediaFragmentURI()
{
    MediaFragmentURIParser fragmentParser(m_currentSrc);
    MediaTime dur = durationMediaTime();
    
    MediaTime start = fragmentParser.startTime();
    if (start.isValid() && start > MediaTime::zeroTime()) {
        m_fragmentStartTime = start;
        if (m_fragmentStartTime > dur)
            m_fragmentStartTime = dur;
    } else
        m_fragmentStartTime = MediaTime::invalidTime();
    
    MediaTime end = fragmentParser.endTime();
    if (end.isValid() && end > MediaTime::zeroTime() && (!m_fragmentStartTime.isValid() || end > m_fragmentStartTime)) {
        m_fragmentEndTime = end;
        if (m_fragmentEndTime > dur)
            m_fragmentEndTime = dur;
    } else
        m_fragmentEndTime = MediaTime::invalidTime();
    
    if (m_fragmentStartTime.isValid() && m_readyState < HAVE_FUTURE_DATA)
        prepareToPlay();
}

void HTMLMediaElement::applyMediaFragmentURI()
{
    if (m_fragmentStartTime.isValid()) {
        m_sentEndEvent = false;
        seek(m_fragmentStartTime);
    }
}

void HTMLMediaElement::updateSleepDisabling()
{
    bool shouldDisableSleep = this->shouldDisableSleep();
    if (!shouldDisableSleep && m_sleepDisabler)
        m_sleepDisabler = nullptr;
    else if (shouldDisableSleep && !m_sleepDisabler)
        m_sleepDisabler = DisplaySleepDisabler::create("com.apple.WebCore: HTMLMediaElement playback");

    if (m_player)
        m_player->setShouldDisableSleep(shouldDisableSleep);
}

bool HTMLMediaElement::shouldDisableSleep() const
{
#if !PLATFORM(COCOA)
    return false;
#endif

    if (m_elementIsHidden)
        return false;

    return m_player && !m_player->paused() && hasVideo() && hasAudio() && !loop();
}

String HTMLMediaElement::mediaPlayerReferrer() const
{
    Frame* frame = document().frame();
    if (!frame)
        return String();

    return SecurityPolicy::generateReferrerHeader(document().referrerPolicy(), m_currentSrc, frame->loader().outgoingReferrer());
}

String HTMLMediaElement::mediaPlayerUserAgent() const
{
    Frame* frame = document().frame();
    if (!frame)
        return String();

    return frame->loader().userAgent(m_currentSrc);
}

#if ENABLE(AVF_CAPTIONS)
Vector<RefPtr<PlatformTextTrack>> HTMLMediaElement::outOfBandTrackSources()
{
    Vector<RefPtr<PlatformTextTrack>> outOfBandTrackSources;
    for (auto& trackElement : childrenOfType<HTMLTrackElement>(*this)) {
        
        if (!trackElement.fastHasAttribute(srcAttr))
            continue;
        
        URL url = trackElement.getNonEmptyURLAttribute(srcAttr);
        if (url.isEmpty())
            continue;
        
        if (!document().contentSecurityPolicy()->allowMediaFromSource(url, trackElement.isInUserAgentShadowTree()))
            continue;

        PlatformTextTrack::TrackKind platformKind = PlatformTextTrack::Caption;
        if (trackElement.kind() == TextTrack::captionsKeyword())
            platformKind = PlatformTextTrack::Caption;
        else if (trackElement.kind() == TextTrack::subtitlesKeyword())
            platformKind = PlatformTextTrack::Subtitle;
        else if (trackElement.kind() == TextTrack::descriptionsKeyword())
            platformKind = PlatformTextTrack::Description;
        else if (trackElement.kind() == TextTrack::forcedKeyword())
            platformKind = PlatformTextTrack::Forced;
        else
            continue;
        
        const AtomicString& mode = trackElement.track()->mode();
        
        PlatformTextTrack::TrackMode platformMode = PlatformTextTrack::Disabled;
        if (TextTrack::hiddenKeyword() == mode)
            platformMode = PlatformTextTrack::Hidden;
        else if (TextTrack::disabledKeyword() == mode)
            platformMode = PlatformTextTrack::Disabled;
        else if (TextTrack::showingKeyword() == mode)
            platformMode = PlatformTextTrack::Showing;
        
        outOfBandTrackSources.append(PlatformTextTrack::createOutOfBand(trackElement.label(), trackElement.srclang(), url.string(), platformMode, platformKind, trackElement.track()->uniqueId(), trackElement.isDefault()));
    }
    
    return outOfBandTrackSources;
}
#endif

bool HTMLMediaElement::mediaPlayerNeedsSiteSpecificHacks() const
{
    Settings* settings = document().settings();
    return settings && settings->needsSiteSpecificQuirks();
}

String HTMLMediaElement::mediaPlayerDocumentHost() const
{
    return document().url().host();
}

void HTMLMediaElement::mediaPlayerEnterFullscreen()
{
    enterFullscreen();
}

void HTMLMediaElement::mediaPlayerExitFullscreen()
{
    exitFullscreen();
}

bool HTMLMediaElement::mediaPlayerIsFullscreen() const
{
    return isFullscreen();
}

bool HTMLMediaElement::mediaPlayerIsFullscreenPermitted() const
{
    return m_mediaSession->fullscreenPermitted(*this);
}

bool HTMLMediaElement::mediaPlayerIsVideo() const
{
    return isVideo();
}

LayoutRect HTMLMediaElement::mediaPlayerContentBoxRect() const
{
    if (renderer())
        return renderer()->enclosingBox().contentBoxRect();
    return LayoutRect();
}

float HTMLMediaElement::mediaPlayerContentsScale() const
{
    if (auto page = document().page())
        return page->pageScaleFactor() * page->deviceScaleFactor();
    return 1;
}

void HTMLMediaElement::mediaPlayerSetSize(const IntSize& size)
{
    setIntegralAttribute(widthAttr, size.width());
    setIntegralAttribute(heightAttr, size.height());
}

void HTMLMediaElement::mediaPlayerPause()
{
    pause();
}

void HTMLMediaElement::mediaPlayerPlay()
{
    play();
}

bool HTMLMediaElement::mediaPlayerPlatformVolumeConfigurationRequired() const
{
    return !m_volumeInitialized;
}

bool HTMLMediaElement::mediaPlayerIsPaused() const
{
    return paused();
}

bool HTMLMediaElement::mediaPlayerIsLooping() const
{
    return loop();
}

CachedResourceLoader* HTMLMediaElement::mediaPlayerCachedResourceLoader()
{
    return &document().cachedResourceLoader();
}

RefPtr<PlatformMediaResourceLoader> HTMLMediaElement::mediaPlayerCreateResourceLoader(std::unique_ptr<PlatformMediaResourceLoaderClient> client)
{
    return adoptRef(*new MediaResourceLoader(document(), fastGetAttribute(HTMLNames::crossoriginAttr), WTF::move(client)));
}

bool HTMLMediaElement::mediaPlayerShouldWaitForResponseToAuthenticationChallenge(const AuthenticationChallenge& challenge)
{
    Frame* frame = document().frame();
    if (!frame)
        return false;

    Page* page = frame->page();
    if (!page)
        return false;

    ResourceRequest request(m_currentSrc);
    ResourceLoadNotifier& notifier = frame->loader().notifier();
    DocumentLoader* documentLoader = document().loader();
    unsigned long identifier = page->progress().createUniqueIdentifier();

    notifier.assignIdentifierToInitialRequest(identifier, documentLoader, request);
    notifier.didReceiveAuthenticationChallenge(identifier, documentLoader, challenge);

    return true;
}

String HTMLMediaElement::sourceApplicationIdentifier() const
{
    if (Frame* frame = document().frame()) {
        if (NetworkingContext* networkingContext = frame->loader().networkingContext())
            return networkingContext->sourceApplicationIdentifier();
    }
    return emptyString();
}

Vector<String> HTMLMediaElement::mediaPlayerPreferredAudioCharacteristics() const
{
    Page* page = document().page();
    if (CaptionUserPreferences* captionPreferences = page ? page->group().captionPreferences() : nullptr)
        return captionPreferences->preferredAudioCharacteristics();
    return Vector<String>();
}

#if PLATFORM(IOS)
String HTMLMediaElement::mediaPlayerNetworkInterfaceName() const
{
    Settings* settings = document().settings();
    if (!settings)
        return emptyString();

    return settings->networkInterfaceName();
}

bool HTMLMediaElement::mediaPlayerGetRawCookies(const URL& url, Vector<Cookie>& cookies) const
{
    return getRawCookies(&document(), url, cookies);
}
#endif

bool HTMLMediaElement::mediaPlayerIsInMediaDocument() const
{
    return document().isMediaDocument();
}

void HTMLMediaElement::mediaPlayerEngineFailedToLoad() const
{
    if (!m_player)
        return;

    if (Frame* frame = document().frame())
        frame->mainFrame().diagnosticLoggingClient().logDiagnosticMessageWithValue(DiagnosticLoggingKeys::engineFailedToLoadKey(), m_player->engineDescription(), String::number(m_player->platformErrorCode()), ShouldSample::No);
}

double HTMLMediaElement::mediaPlayerRequestedPlaybackRate() const
{
    return potentiallyPlaying() ? requestedPlaybackRate() : 0;
}

void HTMLMediaElement::removeBehaviorsRestrictionsAfterFirstUserGesture()
{
    MediaElementSession::BehaviorRestrictions restrictionsToRemove = MediaElementSession::RequireUserGestureForLoad
#if ENABLE(WIRELESS_PLAYBACK_TARGET)
        | MediaElementSession::RequireUserGestureToShowPlaybackTargetPicker
        | MediaElementSession::RequireUserGestureToAutoplayToExternalDevice
#endif
        | MediaElementSession::RequireUserGestureForLoad
        | MediaElementSession::RequireUserGestureForRateChange
        | MediaElementSession::RequireUserGestureForAudioRateChange
        | MediaElementSession::RequireUserGestureForFullscreen
        | MediaElementSession::InvisibleAutoplayNotPermitted;
    m_mediaSession->removeBehaviorRestriction(restrictionsToRemove);
    document().topDocument().noteUserInteractionWithMediaElement();
}

#if ENABLE(MEDIA_SOURCE)
RefPtr<VideoPlaybackQuality> HTMLMediaElement::getVideoPlaybackQuality()
{
#if ENABLE(WEB_TIMING)
    DOMWindow* domWindow = document().domWindow();
    Performance* performance = domWindow ? domWindow->performance() : nullptr;
    double now = performance ? performance->now() : 0;
#else
    DocumentLoader* loader = document().loader();
    double now = loader ? 1000.0 * loader->timing().monotonicTimeToZeroBasedDocumentTime(monotonicallyIncreasingTime()) : 0;
#endif

    auto metrics = m_player ? m_player->videoPlaybackQualityMetrics() : Nullopt;
    if (!metrics)
        return VideoPlaybackQuality::create(now, 0, 0, 0, 0);

    return VideoPlaybackQuality::create(now,
        metrics.value().totalVideoFrames + m_droppedVideoFrames,
        metrics.value().droppedVideoFrames + m_droppedVideoFrames,
        metrics.value().corruptedVideoFrames,
        metrics.value().totalFrameDelay);
}
#endif

#if ENABLE(MEDIA_CONTROLS_SCRIPT)
DOMWrapperWorld& HTMLMediaElement::ensureIsolatedWorld()
{
    if (!m_isolatedWorld)
        m_isolatedWorld = DOMWrapperWorld::create(JSDOMWindow::commonVM());
    return *m_isolatedWorld;
}

bool HTMLMediaElement::ensureMediaControlsInjectedScript()
{
    LOG(Media, "HTMLMediaElement::ensureMediaControlsInjectedScript(%p)", this);
    Page* page = document().page();
    if (!page)
        return false;

    String mediaControlsScript = RenderTheme::themeForPage(page)->mediaControlsScript();
    if (!mediaControlsScript.length())
        return false;

    DOMWrapperWorld& world = ensureIsolatedWorld();
    ScriptController& scriptController = document().frame()->script();
    JSDOMGlobalObject* globalObject = JSC::jsCast<JSDOMGlobalObject*>(scriptController.globalObject(world));
    JSC::ExecState* exec = globalObject->globalExec();
    JSC::JSLockHolder lock(exec);

    JSC::JSValue functionValue = globalObject->get(exec, JSC::Identifier::fromString(exec, "createControls"));
    if (functionValue.isFunction())
        return true;

#ifndef NDEBUG
    // Setting a scriptURL allows the source to be debuggable in the inspector.
    URL scriptURL = URL(ParsedURLString, ASCIILiteral("mediaControlsScript"));
#else
    URL scriptURL;
#endif
    scriptController.evaluateInWorld(ScriptSourceCode(mediaControlsScript, scriptURL), world);
    if (exec->hadException()) {
        exec->clearException();
        return false;
    }

    return true;
}

static void setPageScaleFactorProperty(JSC::ExecState* exec, JSC::JSValue controllerValue, float pageScaleFactor)
{
    JSC::PutPropertySlot propertySlot(controllerValue);
    JSC::JSObject* controllerObject = controllerValue.toObject(exec);
    controllerObject->methodTable()->put(controllerObject, exec, JSC::Identifier::fromString(exec, "pageScaleFactor"), JSC::jsNumber(pageScaleFactor), propertySlot);
}

void HTMLMediaElement::didAddUserAgentShadowRoot(ShadowRoot* root)
{
    LOG(Media, "HTMLMediaElement::didAddUserAgentShadowRoot(%p)", this);

    Page* page = document().page();
    if (!page)
        return;

    DOMWrapperWorld& world = ensureIsolatedWorld();

    if (!ensureMediaControlsInjectedScript())
        return;

    ScriptController& scriptController = document().frame()->script();
    JSDOMGlobalObject* globalObject = JSC::jsCast<JSDOMGlobalObject*>(scriptController.globalObject(world));
    JSC::ExecState* exec = globalObject->globalExec();
    JSC::JSLockHolder lock(exec);

    // The media controls script must provide a method with the following details.
    // Name: createControls
    // Parameters:
    //     1. The ShadowRoot element that will hold the controls.
    //     2. This object (and HTMLMediaElement).
    //     3. The MediaControlsHost object.
    // Return value:
    //     A reference to the created media controller instance.

    JSC::JSValue functionValue = globalObject->get(exec, JSC::Identifier::fromString(exec, "createControls"));
    if (functionValue.isUndefinedOrNull())
        return;

    if (!m_mediaControlsHost)
        m_mediaControlsHost = MediaControlsHost::create(this);

    auto mediaJSWrapper = toJS(exec, globalObject, this);
    auto mediaControlsHostJSWrapper = toJS(exec, globalObject, m_mediaControlsHost.get());
    
    JSC::MarkedArgumentBuffer argList;
    argList.append(toJS(exec, globalObject, root));
    argList.append(mediaJSWrapper);
    argList.append(mediaControlsHostJSWrapper);

    JSC::JSObject* function = functionValue.toObject(exec);
    JSC::CallData callData;
    JSC::CallType callType = function->methodTable()->getCallData(function, callData);
    if (callType == JSC::CallTypeNone)
        return;

    JSC::JSValue controllerValue = JSC::call(exec, function, callType, callData, globalObject, argList);
    exec->clearException();
    JSC::JSObject* controllerObject = JSC::jsDynamicCast<JSC::JSObject*>(controllerValue);
    if (!controllerObject)
        return;

    // Connect the Media, MediaControllerHost, and Controller so the GC knows about their relationship
    JSC::JSObject* mediaJSWrapperObject = mediaJSWrapper.toObject(exec);
    JSC::Identifier controlsHost = JSC::Identifier::fromString(&exec->vm(), "controlsHost");
    
    ASSERT(!mediaJSWrapperObject->hasProperty(exec, controlsHost));

    mediaJSWrapperObject->putDirect(exec->vm(), controlsHost, mediaControlsHostJSWrapper, JSC::DontDelete | JSC::DontEnum | JSC::ReadOnly);

    JSC::JSObject* mediaControlsHostJSWrapperObject = JSC::jsDynamicCast<JSC::JSObject*>(mediaControlsHostJSWrapper);
    if (!mediaControlsHostJSWrapperObject)
        return;
    
    JSC::Identifier controller = JSC::Identifier::fromString(&exec->vm(), "controller");

    ASSERT(!controllerObject->hasProperty(exec, controller));

    mediaControlsHostJSWrapperObject->putDirect(exec->vm(), controller, controllerValue, JSC::DontDelete | JSC::DontEnum | JSC::ReadOnly);

    setPageScaleFactorProperty(exec, controllerValue, page->pageScaleFactor());

    if (exec->hadException())
        exec->clearException();
}

void HTMLMediaElement::setMediaControlsDependOnPageScaleFactor(bool dependsOnPageScale)
{
    LOG(Media, "MediaElement::setMediaControlsDependPageScaleFactor(%p) = %s", this, boolString(dependsOnPageScale));

    Settings* settings = document().settings();
    if (settings && settings->mediaControlsScaleWithPageZoom()) {
        LOG(Media, "MediaElement::setMediaControlsDependPageScaleFactor(%p) forced to false by Settings value", this);
        m_mediaControlsDependOnPageScaleFactor = false;
        return;
    }

    if (m_mediaControlsDependOnPageScaleFactor == dependsOnPageScale)
        return;

    m_mediaControlsDependOnPageScaleFactor = dependsOnPageScale;

    if (m_mediaControlsDependOnPageScaleFactor)
        document().registerForPageScaleFactorChangedCallbacks(this);
    else
        document().unregisterForPageScaleFactorChangedCallbacks(this);
}

void HTMLMediaElement::pageScaleFactorChanged()
{
    Page* page = document().page();
    if (!page)
        return;

    LOG(Media, "HTMLMediaElement::pageScaleFactorChanged(%p) = %f", this, page->pageScaleFactor());
    DOMWrapperWorld& world = ensureIsolatedWorld();
    ScriptController& scriptController = document().frame()->script();
    JSDOMGlobalObject* globalObject = JSC::jsCast<JSDOMGlobalObject*>(scriptController.globalObject(world));
    JSC::ExecState* exec = globalObject->globalExec();
    JSC::JSLockHolder lock(exec);

    JSC::JSValue controllerValue = controllerJSValue(*exec, *globalObject, *this);

    setPageScaleFactorProperty(exec, controllerValue, page->pageScaleFactor());
}
#endif // ENABLE(MEDIA_CONTROLS_SCRIPT)

unsigned long long HTMLMediaElement::fileSize() const
{
    if (m_player)
        return m_player->fileSize();
    
    return 0;
}

PlatformMediaSession::MediaType HTMLMediaElement::mediaType() const
{
    if (m_player && m_readyState >= HAVE_METADATA)
        return hasVideo() ? PlatformMediaSession::Video : PlatformMediaSession::Audio;

    return presentationType();
}

PlatformMediaSession::MediaType HTMLMediaElement::presentationType() const
{
    if (hasTagName(HTMLNames::videoTag))
        return PlatformMediaSession::Video;

    return PlatformMediaSession::Audio;
}

PlatformMediaSession::DisplayType HTMLMediaElement::displayType() const
{
    if (m_videoFullscreenMode == VideoFullscreenModeStandard)
        return PlatformMediaSession::Fullscreen;
    if (m_videoFullscreenMode & VideoFullscreenModePictureInPicture)
        return PlatformMediaSession::Optimized;
    if (m_videoFullscreenMode == VideoFullscreenModeNone)
        return PlatformMediaSession::Normal;

    ASSERT_NOT_REACHED();
    return PlatformMediaSession::Normal;
}

#if ENABLE(MEDIA_SOURCE)
size_t HTMLMediaElement::maximumSourceBufferSize(const SourceBuffer& buffer) const
{
    return m_mediaSession->maximumMediaSourceBufferSize(buffer);
}
#endif

void HTMLMediaElement::suspendPlayback()
{
    LOG(Media, "HTMLMediaElement::suspendPlayback(%p) - paused = %s", this, boolString(paused()));
    if (!paused())
        pause();
}

void HTMLMediaElement::resumeAutoplaying()
{
    LOG(Media, "HTMLMediaElement::resumeAutoplaying(%p) - paused = %s", this, boolString(paused()));
    m_autoplaying = true;

    if (m_readyState == HAVE_ENOUGH_DATA && elementCanTransitionFromAutoplayToPlay(*this))
        play();
}

void HTMLMediaElement::mayResumePlayback(bool shouldResume)
{
    LOG(Media, "HTMLMediaElement::mayResumePlayback(%p) - paused = %s", this, boolString(paused()));
    if (paused() && shouldResume)
        play();
}
    
String HTMLMediaElement::mediaSessionTitle() const
{
    if (fastHasAttribute(titleAttr))
        return fastGetAttribute(titleAttr);
    
    return m_currentSrc;
}

void HTMLMediaElement::didReceiveRemoteControlCommand(PlatformMediaSession::RemoteControlCommandType command, const PlatformMediaSession::RemoteCommandArgument* argument)
{
    LOG(Media, "HTMLMediaElement::didReceiveRemoteControlCommand(%p) - %i", this, static_cast<int>(command));

    switch (command) {
    case PlatformMediaSession::PlayCommand:
        play();
        break;
    case PlatformMediaSession::StopCommand:
    case PlatformMediaSession::PauseCommand:
        pause();
        break;
    case PlatformMediaSession::TogglePlayPauseCommand:
        canPlay() ? play() : pause();
        break;
    case PlatformMediaSession::BeginSeekingBackwardCommand:
        beginScanning(Backward);
        break;
    case PlatformMediaSession::BeginSeekingForwardCommand:
        beginScanning(Forward);
        break;
    case PlatformMediaSession::EndSeekingBackwardCommand:
    case PlatformMediaSession::EndSeekingForwardCommand:
        endScanning();
        break;
    case PlatformMediaSession::SeekToPlaybackPositionCommand:
        ASSERT(argument);
        if (argument)
            handleSeekToPlaybackPosition(argument->asDouble);
        break;
    default:
        { } // Do nothing
    }
}

bool HTMLMediaElement::supportsSeeking() const 
{
    return !isLiveStream();
}

bool HTMLMediaElement::shouldOverrideBackgroundPlaybackRestriction(PlatformMediaSession::InterruptionType type) const
{
    if (type != PlatformMediaSession::EnteringBackground)
        return false;

#if ENABLE(WIRELESS_PLAYBACK_TARGET)
    if (m_player && m_player->isCurrentPlaybackTargetWireless())
        return true;
#endif
    if (m_videoFullscreenMode & VideoFullscreenModePictureInPicture)
        return true;
#if PLATFORM(IOS)
    if (m_videoFullscreenMode == VideoFullscreenModeStandard && wkIsOptimizedFullscreenSupported() && isPlaying())
        return true;
#endif
    return false;
}

#if ENABLE(WIRELESS_PLAYBACK_TARGET)
void HTMLMediaElement::updateMediaState(UpdateMediaState updateState)
{
    if (updateState == UpdateMediaState::Asynchronously) {
        scheduleDelayedAction(CheckMediaState);
        return;
    }

    MediaProducer::MediaStateFlags state = mediaState();
    if (m_mediaState == state)
        return;

    m_mediaState = state;
    m_mediaSession->mediaStateDidChange(*this, m_mediaState);
}
#endif

MediaProducer::MediaStateFlags HTMLMediaElement::mediaState() const
{
    MediaStateFlags state = IsNotPlaying;

    bool hasActiveVideo = isVideo() && hasVideo();
    bool hasAudio = this->hasAudio();
#if ENABLE(WIRELESS_PLAYBACK_TARGET)
    if (m_player && m_player->isCurrentPlaybackTargetWireless())
        state |= IsPlayingToExternalDevice;

    if (m_hasPlaybackTargetAvailabilityListeners) {
        state |= HasPlaybackTargetAvailabilityListener;
        if (!m_mediaSession->wirelessVideoPlaybackDisabled(*this))
            state |= RequiresPlaybackTargetMonitoring;
    }

    bool requireUserGesture = m_mediaSession->hasBehaviorRestriction(MediaElementSession::RequireUserGestureToAutoplayToExternalDevice);
    if (m_readyState >= HAVE_METADATA && !requireUserGesture && !m_failedToPlayToWirelessTarget)
        state |= ExternalDeviceAutoPlayCandidate;

    if (hasActiveVideo || hasAudio)
        state |= HasAudioOrVideo;

    if (hasActiveVideo && endedPlayback())
        state |= DidPlayToEnd;
#endif

    if (!isPlaying())
        return state;

    if (hasAudio && !muted())
        state |= IsPlayingAudio;

    if (hasActiveVideo)
        state |= IsPlayingVideo;

    return state;
}

void HTMLMediaElement::pageMutedStateDidChange()
{
    updateVolume();
}

bool HTMLMediaElement::effectiveMuted() const
{
    return muted() || (document().page() && document().page()->isMuted());
}

bool HTMLMediaElement::doesHaveAttribute(const AtomicString& attribute, AtomicString* value) const
{
    QualifiedName attributeName(nullAtom, attribute, nullAtom);

    AtomicString elementValue = fastGetAttribute(attributeName);
    if (elementValue.isNull())
        return false;
    
    if (Settings* settings = document().settings()) {
        if (attributeName == HTMLNames::x_itunes_inherit_uri_query_componentAttr && !settings->enableInheritURIQueryComponent())
            return false;
    }

    if (value)
        *value = elementValue;
    
    return true;
}

void HTMLMediaElement::setShouldBufferData(bool shouldBuffer)
{
    if (m_player)
        m_player->setShouldBufferData(shouldBuffer);
}

void HTMLMediaElement::purgeBufferedDataIfPossible()
{
#if PLATFORM(IOS)
    // This is called to relieve memory pressure. Turning off buffering causes the media playback
    // daemon to release memory associated with queued-up video frames.
    // We turn it back on right away, but new frames won't get loaded unless playback is resumed.
    setShouldBufferData(false);
    setShouldBufferData(true);
#endif
}

bool HTMLMediaElement::canSaveMediaData() const
{
    if (m_player)
        return m_player->canSaveMediaData();

    return false;
}

#if ENABLE(MEDIA_SESSION)

MediaSession* HTMLMediaElement::session() const
{
    MediaSession* session = m_session.get();
    if (session && session->kindEnum() == MediaSession::Kind::Default)
        return nullptr;
    return session;
}

void HTMLMediaElement::setSession(MediaSession* session)
{
    // 6.1
    // 1. Let m be the media element in question.
    // 2. Let old media session be m's current media session, if it has one, and null otherwise.
    // 3. Let new media session be null.
    // 4. Set m's current media session to null, if it currently has one.
    // 5. Let m's current media session be the new value or the top-level browsing context's media session if the new
    //    value is null.
    // 6. If the new value is null, then set the m's kind IDL attribute to the empty string. Otherwise, set m's kind IDL
    //    attribute to the value of current media session's kind attribute.
    // 7. Let new media session be m's current media session.
    // 8. Update media sessions: If old media session and new media session are the same (whether both null or both the
    //    same media session), then terminate these steps.
    // 9. If m is an active audio-producing participant of old media session, then pause m and remove m from old media
    //    session's active audio-producing participants.
    // 10. If old media session is not null and no longer has one or more active audio-producing participants, then run
    //     the media session release algorithm for old media session.

    if (m_session) {
        if (m_session->isMediaElementActive(*this))
            pause();

        m_session->removeMediaElement(*this);

        if (!m_session->hasActiveMediaElements())
            m_session->releaseSession();
    }

    if (session)
        setSessionInternal(*session);
    else
        setSessionInternal(document().defaultMediaSession());
}

void HTMLMediaElement::setSessionInternal(MediaSession& session)
{
    m_session = &session;
    session.addMediaElement(*this);
    m_kind = session.kind();
}

#endif

void HTMLMediaElement::allowsMediaDocumentInlinePlaybackChanged()
{
    if (potentiallyPlaying() && m_mediaSession->requiresFullscreenForVideoPlayback(*this) && !isFullscreen())
        enterFullscreen();
}

static bool mediaElementIsAllowedToAutoplay(const HTMLMediaElement& element)
{
    const Document& document = element.document();
    if (document.inPageCache())
        return false;
    if (document.activeDOMObjectsAreSuspended())
        return false;

    RenderElement* renderer = element.renderer();
    if (!renderer)
        return false;
    if (renderer->style().visibility() != VISIBLE)
        return false;
    if (renderer->view().frameView().isOffscreen())
        return false;
    if (renderer->visibleInViewportState() != RenderElement::VisibleInViewport)
        return false;
    return true;
}

void HTMLMediaElement::isVisibleInViewportChanged()
{
    m_visibilityChangeTaskQueue.enqueueTask([this] {
        updateShouldAutoplay();
    });
}

void HTMLMediaElement::updateShouldAutoplay()
{
    if (!autoplay())
        return;

    if (!m_mediaSession->hasBehaviorRestriction(MediaElementSession::InvisibleAutoplayNotPermitted))
        return;

    bool canAutoplay = mediaElementIsAllowedToAutoplay(*this);
    if (canAutoplay
        && m_mediaSession->state() == PlatformMediaSession::Interrupted
        && m_mediaSession->interruptionType() == PlatformMediaSession::InvisibleAutoplay)
        m_mediaSession->endInterruption(PlatformMediaSession::MayResumePlaying);
    else if (!canAutoplay
        && m_mediaSession->state() != PlatformMediaSession::Interrupted)
        m_mediaSession->beginInterruption(PlatformMediaSession::InvisibleAutoplay);
}

}

#endif
