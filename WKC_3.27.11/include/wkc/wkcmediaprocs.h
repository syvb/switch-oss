/*
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
 *
 */

#ifndef _WKCMEDIAPROCS_H_
#define _WKCMEDIAPROCS_H_

#include <wkc/wkcbase.h>

typedef struct WKCMediaPlayerCallbacks_ WKCMediaPlayerCallbacks;
typedef struct WKCMediaPlayerSourceBufferCallbacks_ WKCMediaPlayerSourceBufferCallbacks;
typedef struct WKCMediaVideoPlaybackQualityMetrics_ WKCMediaVideoPlaybackQualityMetrics;

typedef bool (*wkcMediaPlayerInitializeProc)(void);
typedef void (*wkcMediaPlayerFinalizeProc)(void);
typedef void (*wkcMediaPlayerForceTerminateProc)(void);
typedef int (*wkcMediaPlayerIsSupportedMIMETypeProc)(const char* in_mimetype, const char* in_codec, const char* in_system, const char* in_url);
typedef int (*wkcMediaPlayerSupportedMIMETypesProc)(void);
typedef const char* (*wkcMediaPlayerSupportedMIMETypeProc)(int in_index);
typedef const char* (*wkcMediaPlayerGetUIStringProc)(int in_type);
typedef const char* (*wkcMediaPlayerGetUIStringTimeProc)(float in_time);
typedef void* (*wkcMediaPlayerCreateProc)(const WKCMediaPlayerCallbacks* in_callbacks, void* in_opaque);
typedef void (*wkcMediaPlayerDeleteProc)(void* in_self);
typedef int (*wkcMediaPlayerLoadProc)(void* in_self, const char* in_uri, int in_mediatype);
typedef int (*wkcMediaPlayerCancelLoadProc)(void* in_self);
typedef int (*wkcMediaPlayerPrepareToPlayProc)(void* in_self);
typedef int (*wkcMediaPlayerPlayProc)(void* in_self);
typedef int (*wkcMediaPlayerPauseProc)(void* in_self);
typedef bool (*wkcMediaPlayerIsPausedProc)(void* in_self);
typedef bool (*wkcMediaPlayerHasVideoProc)(void* in_self);
typedef bool (*wkcMediaPlayerHasAudioProc)(void* in_self);
typedef bool (*wkcMediaPlayerSupportsFullScreenProc)(void* in_self);
typedef bool (*wkcMediaPlayerCanSaveMediaDataProc)(void* in_self);
typedef bool (*wkcMediaPlayerHasClosedCaptionsProc)(void* in_self);
typedef int (*wkcMediaPlayerSetClosedCaptionsVisibleProc)(void* in_self, bool in_flag);
typedef bool (*wkcMediaPlayerHasSingleSecurityOriginProc)(void* in_self);
typedef int (*wkcMediaPlayerNaturalSizeProc)(void* in_self, WKCSize* out_size);
typedef int (*wkcMediaPlayerSetSizeProc)(void* in_self, const WKCSize* in_size);
typedef int (*wkcMediaPlayerSetPreloadProc)(void* in_self, const int in_type);
typedef int (*wkcMediaPlayerSetPageIsVisibleProc)(void* in_self, bool in_flag);
typedef double (*wkcMediaPlayerDurationProc)(void* in_self);
typedef float (*wkcMediaPlayerStartTimeProc)(void* in_self);
typedef double (*wkcMediaPlayerCurrentTimeProc)(void* in_self);
typedef float (*wkcMediaPlayerMaxTimeSeekableProc)(void* in_self);
typedef int (*wkcMediaPlayerSeekProc)(void* in_self, float in_time);
typedef double (*wkcMediaPlayerRateProc)(void* in_self);
typedef int (*wkcMediaPlayerSetRateProc)(void* in_self, float in_rate);
typedef int (*wkcMediaPlayerSetPreservesPitchProc)(void* in_self, bool in_flag);
typedef float (*wkcMediaPlayerVolumeProc)(void* in_self);
typedef int (*wkcMediaPlayerSetVolumeProc)(void* in_self, float in_volume);
typedef int (*wkcMediaPlayerNetworkStateProc)(void* in_self);
typedef int (*wkcMediaPlayerReadyStateProc)(void* in_self);
typedef int (*wkcMediaPlayerMovieLoadTypeProc)(void* in_self);
typedef int (*wkcMediaPlayerSetWindowPositionProc)(void* in_self, const WKCRect* in_rect);
typedef int (*wkcMediaPlayerVideoSinkTypeProc)(void* in_self);
typedef int (*wkcMediaPlayerBufferedRangesProc)(void* in_self);
typedef int (*wkcMediaPlayerBufferedRangeProc)(void* in_self, int in_index, float* out_start, float* out_end);
typedef int (*wkcMediaPlayerSetLoopProc)(void* in_self, bool in_loop);
typedef void (*wkcMediaPlayerRemovedFromDocumentProc)(void* in_self);
typedef void (*wkcMediaPlayerWillBecomeInactiveProc)(void* in_self);
typedef void (*wkcMediaPlayerEnterFullscreenProc)(void* in_self);
typedef void (*wkcMediaPlayerExitFullscreenProc)(void* in_self);
typedef bool (*wkcMediaPlayerCanEnterFullscreenProc)(void* in_self);
typedef WKCMediaVideoPlaybackQualityMetrics (*wkcMedaPlayerVideoPlaybackQualityMetricsProc)(void* in_self);
typedef int (*wkcMediaPlayerDecodeAudioDataProc)(const void* in_data, int in_len, void* out_buf, int in_buflen, int* out_samplerate, int* out_bitspersample, int* out_channels, int* out_endian);
typedef int (*wkcMediaPlayerAddSBProc)(void* in_self, const char* in_type, void** out_sourcebuffer);
typedef double (*wkcMediaPlayerGetMSDurationProc)(void* in_self);
typedef int (*wkcMediaPlayerSetMSDurationProc)(void* in_self, double in_duration);
typedef int (*wkcMediaPlayerNotifyMSMarkEndOfStreamProc)(void* in_self, int in_status);
typedef int (*wkcMediaPlayerNotifyMSUnmarkEndOfStreamProc)(void* in_self);
typedef bool (*wkcMediaPlayerMSIsEndedProc)(void* in_self);
typedef int (*wkcMediaPlayerSetReadyStateProc)(void* in_self, int in_state);
typedef int (*wkcMediaPlayerSetActiveSBProc)(void* in_self, void* in_sourcebuffer, bool in_active);
typedef int (*wkcMediaPlayerAppendToSBProc)(void* in_self, void* in_sourcebuffer, const unsigned char* in_data, unsigned in_length, const WKCMediaPlayerSourceBufferCallbacks* in_callbacks, void* in_opaque);
typedef int (*wkcMediaPlayerAbortSBProc)(void* in_self, void* in_sourcebuffer);
typedef bool (*wkcMediaPlayerSetSBTimestampOffsetProc)(void* in_self, void* in_sourcebuffer, double in_timestamp_offset);
typedef int (*wkcMediaPlayerRemoveSBProc)(void* in_self, void* in_sourcebuffer);
typedef int (*wkcMediaPlayerRemoveCodedFramesProc)(void* in_self, void* in_sourcebuffer, double in_start, double in_end);
typedef int (*wkcMediaPlayerEvictCodedFramesSBProc)(void* in_self, void* in_sourcebuffer, double* out_start, double* out_end);
typedef bool (*wkcMediaPlayerIsFullSBProc)(void* in_self, void* in_sourcebuffer);
typedef bool (*wkcMediaPlayerCanSwitchToTypeProc)(void* in_self, void* in_sourcebuffer, const char* in_type);
typedef void (*wkcMediaPlayerSetCurlVerboseProc)(void* in_self, bool in_verbose);

struct WKCMediaPlayerProcs_ {
    wkcMediaPlayerInitializeProc fInitializeProc;
    wkcMediaPlayerFinalizeProc fFinalizeProc;
    wkcMediaPlayerForceTerminateProc fForceTerminateProc;
    wkcMediaPlayerIsSupportedMIMETypeProc fIsSupportedMIMETypeProc;
    wkcMediaPlayerSupportedMIMETypesProc fSupportedMIMETypesProc;
    wkcMediaPlayerSupportedMIMETypeProc fSupportedMIMETypeProc;
    wkcMediaPlayerGetUIStringProc fGetUIStringProc;
    wkcMediaPlayerGetUIStringTimeProc fGetUIStringTimeProc;
    wkcMediaPlayerCreateProc fCreateProc;
    wkcMediaPlayerDeleteProc fDeleteProc;
    wkcMediaPlayerLoadProc fLoadProc;
    wkcMediaPlayerCancelLoadProc fCancelLoadProc;
    wkcMediaPlayerPrepareToPlayProc fPrepareToPlayProc;
    wkcMediaPlayerPlayProc fPlayProc;
    wkcMediaPlayerPauseProc fPauseProc;
    wkcMediaPlayerIsPausedProc fIsPausedProc;
    wkcMediaPlayerHasVideoProc fHasVideoProc;
    wkcMediaPlayerHasAudioProc fHasAudioProc;
    wkcMediaPlayerSupportsFullScreenProc fSupportsFullScreenProc;
    wkcMediaPlayerCanSaveMediaDataProc fCanSaveMediaDataProc;
    wkcMediaPlayerHasClosedCaptionsProc fHasClosedCaptionsProc;
    wkcMediaPlayerSetClosedCaptionsVisibleProc fSetClosedCaptionsVisibleProc;
    wkcMediaPlayerHasSingleSecurityOriginProc fHasSingleSecurityOriginProc;
    wkcMediaPlayerNaturalSizeProc fNaturalSizeProc;
    wkcMediaPlayerSetSizeProc fSetSizeProc;
    wkcMediaPlayerSetPreloadProc fSetPreloadProc;
    wkcMediaPlayerSetPageIsVisibleProc fSetPageIsVisibleProc;
    wkcMediaPlayerDurationProc fDurationProc;
    wkcMediaPlayerStartTimeProc fStartTimeProc;
    wkcMediaPlayerCurrentTimeProc fCurrentTimeProc;
    wkcMediaPlayerMaxTimeSeekableProc fMaxTimeSeekableProc;
    wkcMediaPlayerSeekProc fSeekProc;
    wkcMediaPlayerRateProc fRateProc;
    wkcMediaPlayerSetRateProc fSetRateProc;
    wkcMediaPlayerSetPreservesPitchProc fSetPreservesPitchProc;
    wkcMediaPlayerVolumeProc fVolumeProc;
    wkcMediaPlayerSetVolumeProc fSetVolumeProc;
    wkcMediaPlayerNetworkStateProc fNetworkStateProc;
    wkcMediaPlayerReadyStateProc fReadyStateProc;
    wkcMediaPlayerMovieLoadTypeProc fMovieLoadTypeProc;
    wkcMediaPlayerSetWindowPositionProc fSetWindowPositionProc;
    wkcMediaPlayerVideoSinkTypeProc fVideoSinkTypeProc;
    wkcMediaPlayerBufferedRangesProc fBufferedRangesProc;
    wkcMediaPlayerBufferedRangeProc fBufferedRangeProc;
    wkcMediaPlayerSetLoopProc fSetLoopProc;
    wkcMediaPlayerRemovedFromDocumentProc fRemovedFromDocumentProc;
    wkcMediaPlayerWillBecomeInactiveProc fWillBecomeInactiveProc;
    wkcMediaPlayerEnterFullscreenProc fEnterFullscreenProc;
    wkcMediaPlayerExitFullscreenProc fExitFullscreenProc;
    wkcMediaPlayerCanEnterFullscreenProc fCanEnterFullscreenProc;
    wkcMedaPlayerVideoPlaybackQualityMetricsProc fVideoPlaybackQualityMetricsProc;
    wkcMediaPlayerDecodeAudioDataProc fDecodeAudioDataProc;
    wkcMediaPlayerAddSBProc fAddSBProc;
    wkcMediaPlayerGetMSDurationProc fGetMSDurationProc;
    wkcMediaPlayerSetMSDurationProc fSetMSDurationProc;
    wkcMediaPlayerNotifyMSMarkEndOfStreamProc fNotifyMSMarkEndOfStreamProc;
    wkcMediaPlayerNotifyMSUnmarkEndOfStreamProc fNotifyMSUnmarkEndOfStreamProc;
    wkcMediaPlayerMSIsEndedProc fMSIsEndedProc;
    wkcMediaPlayerSetReadyStateProc fSetReadyStateProc;
    wkcMediaPlayerSetActiveSBProc fSetActiveSBProc;
    wkcMediaPlayerAppendToSBProc fAppendToSBProc;
    wkcMediaPlayerAbortSBProc fAbortSBProc;
    wkcMediaPlayerSetSBTimestampOffsetProc fSetSBTimestampOffsetProc;
    wkcMediaPlayerRemoveSBProc fRemoveSBProc;
    wkcMediaPlayerRemoveCodedFramesProc fRemoveCodedFramesProc;
    wkcMediaPlayerEvictCodedFramesSBProc fEvictCodedFramesSBProc;
    wkcMediaPlayerIsFullSBProc fIsFullSBProc;
    wkcMediaPlayerCanSwitchToTypeProc fCanSwitchToTypeProc;
    wkcMediaPlayerSetCurlVerboseProc fSetCurlVerboseProc;
};
typedef struct WKCMediaPlayerProcs_ WKCMediaPlayerProcs;

#endif // _WKCMEDIAPROCS_H_
