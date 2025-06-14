/*
 *
 *  This library is free software; you can redistribute it and/or
 *  modify it under the terms of the GNU Library General Public
 *  License as published by the Free Software Foundation; either
 *  version 2 of the License, or (at your option) any later version.
 *
 *  This library is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 *  Library General Public License for more details.
 *
 *  You should have received a copy of the GNU Library General Public
 *  License along with this library; if not, write to the
 *  Free Software Foundation, Inc., 51 Franklin St, Fifth Floor,
 *  Boston, MA  02110-1301, USA.
 */

#include "config.h"

#if ENABLE(MEDIA_SOURCE)

#include <wtf/Vector.h>
#include <wtf/text/CString.h>

#include "MediaPlayerPrivateWKC.h"
#include "MediaSourcePrivateWKC.h"
#include "SourceBufferPrivateWKC.h"

#include "ContentType.h"
#include "MediaSample.h"

#include "NotImplemented.h"

#include <wkc/wkcmediapeer.h>

namespace WebCore {

MediaSourcePrivateWKC::MediaSourcePrivateWKC(MediaPlayerPrivateWKC* in_player, void* in_peer)
    : m_player(in_player)
    , m_peer(in_peer)
{
}

MediaSourcePrivateWKC::~MediaSourcePrivateWKC()
{
}

MediaSourcePrivateWKC*
MediaSourcePrivateWKC::create(MediaPlayerPrivateWKC* in_player, void* in_peer)
{
    MediaSourcePrivateWKC* self = new MediaSourcePrivateWKC(in_player, in_peer);
    if (!self) {
        return 0;
    }
    if (!self->construct()) {
        delete self;
        return 0;
    }
    return self;
}

bool
MediaSourcePrivateWKC::construct()
{
    return true;
}

MediaSourcePrivate::AddStatus
MediaSourcePrivateWKC::addSourceBuffer(const ContentType& type, bool webMParserEnabled, RefPtr<SourceBufferPrivate>& sourceBufferPrivate)
{
    CString cstr = type.containerType().utf8();
    const char* _type = cstr.data();

    void* sourcebuffer = 0;

    int ret = wkcMediaPlayerAddSBPeer(m_peer, _type, &sourcebuffer);

    MediaSourcePrivate::AddStatus status;
    switch (ret) {
    case WKC_MEDIA_ADD_SB_OK:
        status = MediaSourcePrivate::AddStatus::Ok;
        break;
    case WKC_MEDIA_ADD_SB_NOTSUPPORTED:
        status = MediaSourcePrivate::AddStatus::NotSupported;
        break;
    case WKC_MEDIA_ADD_SB_REACHEDIDLIMIT:
        status = MediaSourcePrivate::AddStatus::ReachedIdLimit;
        break;
    default:
        // Never reach here
        status = MediaSourcePrivate::AddStatus::NotSupported;
    }

    sourceBufferPrivate = adoptRef(SourceBufferPrivateWKC::create(m_peer, sourcebuffer, this));
    return status;
}

MediaTime
MediaSourcePrivateWKC::duration()
{
    return MediaTime::createWithDouble(wkcMediaPlayerGetMSDurationPeer(m_peer));
}

void
MediaSourcePrivateWKC::setDuration(const MediaTime& duration)
{
    (void)wkcMediaPlayerSetMSDurationPeer(m_peer, duration.toDouble());
}

void
MediaSourcePrivateWKC::markEndOfStream(EndOfStreamStatus status)
{
    int _status;
    switch(status) {
    case MediaSourcePrivate::EosNoError:
        _status = WKC_MEDIA_MS_EOS_NOERROR;
        break;
    case MediaSourcePrivate::EosNetworkError:
        _status = WKC_MEDIA_MS_EOS_NETWORKERROR;
        break;
    case MediaSourcePrivate::EosDecodeError:
        _status = WKC_MEDIA_MS_EOS_DECODEERROR;
        break;
    default:
        // Never reach here
        _status = WKC_MEDIA_MS_EOS_DECODEERROR;
        break;
    }

    (void)wkcMediaPlayerNotifyMSMarkEndOfStreamPeer(m_peer, _status);
}

void
MediaSourcePrivateWKC::unmarkEndOfStream()
{
    wkcMediaPlayerNotifyMSUnmarkEndOfStreamPeer(m_peer);
}

bool
MediaSourcePrivateWKC::isEnded() const
{
    return wkcMediaPlayerMSIsEndedPeer(m_peer);
}

MediaPlayer::ReadyState
MediaSourcePrivateWKC::readyState() const
{
    return m_player->readyState();
}

void
MediaSourcePrivateWKC::setReadyState(MediaPlayer::ReadyState in_state)
{
    int state;
    switch (in_state) {
    case MediaPlayer::ReadyState::HaveMetadata:
        state = WKC_MEDIA_READYSTATE_HAVE_METADATA;
        break;
    case MediaPlayer::ReadyState::HaveCurrentData:
        state = WKC_MEDIA_READYSTATE_HAVE_CURRENTDATA;
        break;
    case MediaPlayer::ReadyState::HaveFutureData:
        state = WKC_MEDIA_READYSTATE_HAVE_FUTUREDATA;
        break;
    case MediaPlayer::ReadyState::HaveEnoughData:
        state = WKC_MEDIA_READYSTATE_HAVE_ENOUGHDATA;
        break;
    case MediaPlayer::ReadyState::HaveNothing:
    default:
        state = WKC_MEDIA_READYSTATE_HAVE_NOTHING;
        break;
    }
    wkcMediaPlayerSetReadyStatePeer(m_peer, state);
}

void
MediaSourcePrivateWKC::waitForSeekCompleted()
{
}

void
MediaSourcePrivateWKC::seekCompleted()
{
}

} // namespace WebCore

#endif
