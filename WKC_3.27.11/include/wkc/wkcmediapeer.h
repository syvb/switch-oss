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

#ifndef _WKC_MEDIA_PEER_H_
#define _WKC_MEDIA_PEER_H_

#include <wkc/wkcbase.h>
#include <wkc/wkcmediaprocs.h>
#include <wkc/wkcaudioprocs.h>

#include <inttypes.h>

// Ugh!: for testing. move to wkcconfig.h!
// 110525 ACCESS Co.,Ltd.

#if 0
#define HAVE_SUPPORT_MEDIASTREAM_NULL 1
#define HAVE_SUPPORT_CONTAINER_NULL 1
#define HAVE_SUPPORT_AUDIO_NULL 1
#define HAVE_SUPPORT_VIDEO_NULL 1
#endif

//#define HAVE_SUPPORT_MEDIASTREAM_FILE 1
//#define HAVE_SUPPORT_MEDIASTREAM_HTTP 1

//#define HAVE_SUPPORT_CONTAINER_RIFFWAV 1
//#define HAVE_SUPPORT_AUDIO_WAV 1

//#define HAVE_SUPPORT_CONTAINER_MP3 1
//#define HAVE_SUPPORT_AUDIO_MP3 1

#ifdef WKC_USE_WEBM
# define HAVE_SUPPORT_CONTAINER_WEBM 1
// # define HAVE_SUPPORT_CONTAINER_OGG 1
// # define HAVE_SUPPORT_AUDIO_VORBIS 1
# define HAVE_SUPPORT_VIDEO_VP8 1
#endif

#ifdef WKC_USE_GSTREAMER
# define HAVE_SUPPORT_MEDIASOURCE_GSTREAMER 1
# define HAVE_SUPPORT_MEDIASTREAM_GSTREAMER 1
# define HAVE_SUPPORT_CONTAINER_GSTREAMER 1
# define HAVE_SUPPORT_AUDIO_GSTREAMER 1
# define HAVE_SUPPORT_VIDEO_GSTREAMER 1
# undef HAVE_SUPPORT_CONTAINER_RIFFWAV
# undef HAVE_SUPPORT_AUDIO_WAV
#endif

#ifdef WKC_USE_NFMP
# define HAVE_SUPPORT_MEDIASTREAM_NFMP 1
# define HAVE_SUPPORT_CONTAINER_NFMP 1
# define HAVE_SUPPORT_AUDIO_NFMP 1
# define HAVE_SUPPORT_VIDEO_NFMP 1
#endif

/**
   @file
   @brief media (audio / video) playback and FFT related peers.
*/

/*@{*/

WKC_BEGIN_C_LINKAGE

enum {
    WKC_MEDIA_SUPPORT_NOTSUPPORTED = 0,
    WKC_MEDIA_SUPPORT_SUPPORTED,
    WKC_MEDIA_SUPPORT_MAYBESUPPORTED,
    WKC_MEDIA_SUPPORTS
};

enum {
    WKC_MEDIA_ERROR_OK = 0,
    WKC_MEDIA_ERROR_GENERIC = -1,
    WKC_MEDIA_ERROR_INVALIDVALUE = -2,
    WKC_MEDIA_ERROR_NOTFOUND = -3,
    WKC_MEDIA_ERROR_NOTSUPPORTED = -4,
    WKC_MEDIA_ERROR_NO_MEMORY = -5,
    WKC_MEDIA_ERROR_BUSY = -6,
    WKC_MEDIA_ERROR_NOTREADY = -7,
    WKC_MEDIA_ERROR_EOF = -8,
    WKC_MEDIA_ERROR_WOULDBLOCK = -9,
    WKC_MEDIA_ERRORS = -100
};
enum {
    WKC_MEDIA_NETWORKSTATE_EMPTY = 0,
    WKC_MEDIA_NETWORKSTATE_IDLE,
    WKC_MEDIA_NETWORKSTATE_LOADING,
    WKC_MEDIA_NETWORKSTATE_LOADED,
    WKC_MEDIA_NETWORKSTATE_FORMATERROR,
    WKC_MEDIA_NETWORKSTATE_NETWORKERROR,
    WKC_MEDIA_NETWORKSTATE_DECODEERROR,
    WKC_MEDIA_NETWORKSTATES
};
enum {
    WKC_MEDIA_READYSTATE_HAVE_NOTHING = 0,
    WKC_MEDIA_READYSTATE_HAVE_METADATA,
    WKC_MEDIA_READYSTATE_HAVE_CURRENTDATA,
    WKC_MEDIA_READYSTATE_HAVE_FUTUREDATA,
    WKC_MEDIA_READYSTATE_HAVE_ENOUGHDATA,
    WKC_MEDIA_READYSTATES
};
enum {
    WKC_MEDIA_PRELOAD_NONE = 0,
    WKC_MEDIA_PRELOAD_METADATA,
    WKC_MEDIA_PRELOAD_AUTO,
    WKC_MEDIA_PRELOADS
};
enum {
    WKC_MEDIA_MOVIELOADTYPE_UNKNOWN = 0,
    WKC_MEDIA_MOVIELOADTYPE_DOWNLOAD,
    WKC_MEDIA_MOVIELOADTYPE_STOREDSTREAM,
    WKC_MEDIA_MOVIELOADTYPE_LIVESTREAM,
    WKC_MEDIA_MOVIELOADTYPES
};
enum {
    WKC_MEDIA_VIDEOSINKTYPE_BITMAP = 0,
    WKC_MEDIA_VIDEOSINKTYPE_WINDOW,
    WKC_MEDIA_VIDEOSINKTYPE_HOLEDWINDOW,
    WKC_MEDIA_VIDEOSINKTYPE_LAYER,
    WKC_MEDIA_VIDEOSINKTYPES
};
enum {
    WKC_MEDIA_AUDIOSINKTYPE_BINARYSTREAM = 0,
    WKC_MEDIA_AUDIOSINKTYPE_DIRECT,
    WKC_MEDIA_AUDIOSINKTYPES
};
enum {
    WKC_MEDIA_ENDIAN_LITTLEENDIAN = 0,
    WKC_MEDIA_ENDIAN_BIGENDIAN,
    WKC_MEDIA_ENDIAN_MIDDLEENDIAN,
    WKC_MEDIA_ENDIANS
};

enum {
    WKC_MEDIA_PLAYERSTATE_NETWORKSTATECHANGED = 0,
    WKC_MEDIA_PLAYERSTATE_READYSTATECHANGED,
    WKC_MEDIA_PLAYERSTATE_VOLUMECHANGED,
    WKC_MEDIA_PLAYERSTATE_TIMECHANGED,
    WKC_MEDIA_PLAYERSTATE_SIZECHANGED,
    WKC_MEDIA_PLAYERSTATE_RATECHANGED,
    WKC_MEDIA_PLAYERSTATE_DURATIONCHANGED,
    WKC_MEDIA_PLAYERSTATE_PLAYBACKSTATECHANGED,
    WKC_MEDIA_PLAYERSTATE_SEEKBEGIN,
    WKC_MEDIA_PLAYERSTATE_SEEKEND,
    WKC_MEDIA_PLAYERSTATE_RESETUSERGESTURERESTRICTION,
    WKC_MEDIA_PLAYERSTATES
};

enum {
    WKC_MEDIA_UISTRING_LOADING = 0,
    WKC_MEDIA_UISTRING_BROADCAST,
    WKC_MEDIA_UISTRINGS
};

enum {
    WKC_MEDIA_KEYERR_UNKNOWN = 1,
    WKC_MEDIA_KEYERR_CLIENT,
    WKC_MEDIA_KEYERR_SERVICE,
    WKC_MEDIA_KEYERR_OUTPUT,
    WKC_MEDIA_KEYERR_HARDWARECHANGE,
    WKC_MEDIA_KEYERR_DOMAIN
};

enum {
    WKC_MEDIA_KEYEXCEPTION_NOERROR = 0,
    WKC_MEDIA_KEYEXCEPTION_INVALIDPLAYERSTATE,
    WKC_MEDIA_KEYEXCEPTION_NOTSUPPORTED,
    WKC_MEDIA_KEYEXCEPTION_INVALIDACCESS,
    WKC_MEDIA_KEYEXCEPTION
};

enum {
    WKC_MEDIA_ADD_SB_OK = 0,
    WKC_MEDIA_ADD_SB_NOTSUPPORTED = 1,
    WKC_MEDIA_ADD_SB_REACHEDIDLIMIT = 2,
};

enum {
    WKC_MEDIA_MS_EOS_NOERROR = 0,
    WKC_MEDIA_MS_EOS_NETWORKERROR = 1,
    WKC_MEDIA_MS_EOS_DECODEERROR = 2,
};

enum {
    WKC_MEDIA_TYPE_NORMAL = 0,
    WKC_MEDIA_TYPE_MEDIASOURCE,
    WKC_MEDIA_TYPE_MEDIASTREAM,
    WKC_MEDIA_TYPES
};

enum {
    WKC_MEDIA_SB_TRACKKIND_AUDIO,
    WKC_MEDIA_SB_TRACKKIND_VIDEO,
};

enum {
    WKC_MEDIA_SB_APPENDRESULT_APPEND_SUCCEEDED = 0,
    WKC_MEDIA_SB_APPENDRESULT_READ_STREAM_FAILED = 1,
    WKC_MEDIA_SB_APPENDRESULT_PARSING_FAILED = 2,
};

/**
@typedef void(*WKCMPNotifyPlayerStateProc)(void*, int in_state)
@brief Type of callback function that gives notification of media player state
@details
Call this function when a state change notification of the media player is necessary. Values specified for in_state are defined as follows:
@li WKC_MEDIA_PLAYERSTATE_NETWORKSTATECHANGED When network state changes
@li WKC_MEDIA_PLAYERSTATE_READYSTATECHANGED When content hold state changes
@li WKC_MEDIA_PLAYERSTATE_VOLUMECHANGED When volume changes
@li WKC_MEDIA_PLAYERSTATE_TIMECHANGED When playback position changes
@li WKC_MEDIA_PLAYERSTATE_SIZECHANGED When player size changes
@li WKC_MEDIA_PLAYERSTATE_RATECHANGED When playback speed changes
@li WKC_MEDIA_PLAYERSTATE_DURATIONCHANGED When content duration changes
*/
typedef void(*WKCMPNotifyPlayerStateProc)(void*, int in_state);
/**
@typedef void(*WKCMPNotifyRequestInvalidateProc)(void*, int in_x, int in_y, int in_w, int in_h)
@brief Type of callback function that gives notification of areas that need to be redrawn
@details
Call this function to request redrawing.
*/
typedef void(*WKCMPNotifyRequestInvalidateProc)(void*, int in_x, int in_y, int in_w, int in_h);
/**
@typedef void(*WKCMPNotifyAudioDataAvailableProc)(void*, float in_timing, int in_availdatalen)
@brief Type of callback function that gives notification of the length of audio data that can be processed
@details
Call this function to give notification of the length of audio data that can be processed.
Specify the position from the start position (elapsed seconds) for in_timing, and the total size for in_availdatalen.
*/
typedef void(*WKCMPNotifyAudioDataAvailableProc)(void*, float in_timing, unsigned int in_availdatalen);
/**
@typedef void(*WKCMPNotifyKeyNeededProc)(void *self, const char *in_keySystem, const char *in_sessionID, const unsigned char *in_initData, unsigned in_initDataLen);
@brief Type of callback function that gives notification of key is needed (Encrypted Media Extensions 1.0b relevant)
@param self Media player instance
@param in_keySystem Key system identifier of the event
@param in_sessionID Session ID of the event, can be null. Please refer to sec 5.1, sec 5.2, EME 1.0b
@param in_initData CDM (Content Decryption Module) dependent container data
@param in_initDataLen initData length
@details

Refer to http://dvcs.w3.org/hg/html-media/raw-file/eme-v0.1b/encrypted-media/encrypted-media.html#events
*/
    typedef void(*WKCMPNotifyKeyNeededProc)(void *self,
                                            const char *in_keySystem,
                                            const char *in_sessionID,
                                            const unsigned char *in_initData,
                                            unsigned in_initDataLen);

/**
@typedef void(*WKCMPNotifyKeyRequestProc)(void *self, const char *in_keySystem, const char *in_sessionID, const unsigned char *in_binaryBuff, unsigned in_len, const char *in_defaultURL);
@brief Type of callback function that gives notification of generateKeyRequest method was success and bring back results (Encrypted Media Extensions 1.0b relevant)
@param self Media player instance
@param in_keySystem Key system identifier of the event
@param in_sessionID Session ID of the event
@param in_binaryBuff Message from CDM (Content Decryption Module)
@param in_defaultURL the default URL to send the key request to as provided by the media data.
@param in_len Message length
@details

Refer to http://dvcs.w3.org/hg/html-media/raw-file/eme-v0.1b/encrypted-media/encrypted-media.html#events
*/
    typedef void(*WKCMPNotifyKeyRequestProc)(void *self,
                                             const char *in_keySystem,
                                             const char *in_sessionID,
                                             const unsigned char *in_binaryBuff,
                                             unsigned in_len,
                                             const char *in_defaultURL);
/**
@typedef void(*WKCMPNotifyKeyAddedProc)(void *self, const char *in_keySystem, const char *in_sessionID);
@brief Type of callback function that gives notification of addkey method was success (Encrypted Media Extensions 1.0b relevant)
@param self Media player instance
@param in_keySystem Key system identifier of the event
@param in_sessionID Session ID of the event

Refer to http://dvcs.w3.org/hg/html-media/raw-file/eme-v0.1b/encrypted-media/encrypted-media.html#events
*/
    typedef void(*WKCMPNotifyKeyAddedProc)(void *self, const char *in_keySystem, const char *in_sessionID);
/**
@typedef void(*WKCMPNotifyKeyErrorProc)(void *self, const char *in_keySystem, const char *in_sessionID, int in_keyErrorCode, unsigned short in_systemCode);
@brief Type of callback function that gives notification of errors during EME related methods in peer (Encrypted Media Extensions 1.0b relevant)
@param self Media player instance
@param in_keySystem Key system identifier of the event
@param in_sessionID Session ID of the event
@param in_keyErrorCode MediaKeyError code (See below)
@param in_systemCode  key system related status code
@details
Valid in_keyErrorCode
@li WKC_MEDIA_KEYERR_UNKNOWN
@li WKC_MEDIA_KEYERR_CLIENT
@li WKC_MEDIA_KEYERR_SERVICE
@li WKC_MEDIA_KEYERR_OUTPUT
@li WKC_MEDIA_KEYERR_HARDWARECHANGE
@li WKC_MEDIA_KEYERR_DOMAIN

Refer to http://dvcs.w3.org/hg/html-media/raw-file/eme-v0.1b/encrypted-media/encrypted-media.html#events
*/
    typedef void(*WKCMPNotifyKeyErrorProc)(void *self,
                                           const char *in_keySystem,
                                           const char *in_sessionID,
                                           int in_keyErrorCode,
                                           unsigned short in_systemCode);

typedef bool(*WKCMPChromeVisibleProc)(void*);
typedef void* (*WKCMPCreateHTMLMediaElementProc)(void*);
typedef void (*WKCMPDestroyHTMLMediaElementProc)(void*, void*);

/**
@typedef void(*WKCMPNotifyStreamReceiveResponseProc)(void* in_instance, int in_id, const void* in_response)
@brief Type of function that notifies of stream response
@details
It must be called at receiving stream response.
*/
typedef void(*WKCMPNotifyStreamReceiveResponseProc)(void* in_instance, int in_id, const void* in_response);
/**
@typedef void(*WKCMPNotifyStreamReceiveDataProc)(void* in_instance, int in_id, const unsigned char* in_data, int in_len)
@brief Type of function that notifies of stream data reception state
@details
It must be called while receiving stream data.
*/
typedef void(*WKCMPNotifyStreamReceiveDataProc)(void* in_instance, int in_id, const unsigned char* in_data, int in_len);
/**
@typedef void(*WKCMPNotifyStreamErrorProc)(void* in_instance, int in_id, int in_error)
@brief Type of function that gives notification when an error occurs in stream
@details
Call it when an error occurs in stream.
*/
typedef void(*WKCMPNotifyStreamErrorProc)(void* in_instance, int in_id, int in_error);
/**
@typedef void(*WKCMPNotifyStreamFinishProc)(void* in_instance, int in_id)
@brief Type of function that gives notification when stream finished
@details
Call it when stream finished.
*/
typedef void(*WKCMPNotifyStreamFinishProc)(void* in_instance, int in_id);

/** @brief Structure that stores header information */
struct WKCMediaPlayerStreamHeaderInfo_ {
    /** @brief Member that holds header name */
    const char* fName;
    /** @brief Member that holds header value */
    const char* fValue;
};
/** @brief Type definition of WKCMediaPlayerStreamProxyInfo */
typedef struct WKCMediaPlayerStreamHeaderInfo_ WKCMediaPlayerStreamHeaderInfo;

/** @brief Structure that stores callback function for notifying media player of stream state */
struct WKCMediaPlayerStreamStateProcs_ {
    /** @brief Variable that stores instance */
    void* fInstance;
    /** @brief Member that holds WKCMPNotifyStreamReceiveResponseProc */
    WKCMPNotifyStreamReceiveResponseProc fReceiveResponseProc;
    /** @brief Member that holds WKCMPNotifyStreamReceiveDataProc */
    WKCMPNotifyStreamReceiveDataProc fReceiveDataProc;
    /** @brief Member that holds WKCMPNotifyStreamErrorProc */
    WKCMPNotifyStreamErrorProc fErrorProc;
    /** @brief Member that holds WKCMPNotifyStreamFinishProc */
    WKCMPNotifyStreamFinishProc fFinishProc;
};
/** @brief Type definition of WKCMediaPlayerStreamStateProcs */
typedef struct WKCMediaPlayerStreamStateProcs_ WKCMediaPlayerStreamStateProcs;

/**
@typedef int(*WKCMPOpenURIProc)(void* in_instance, const char* in_uri, const char* in_method, const WKCMediaPlayerStreamHeaderInfo* in_headers, int in_len, const WKCMediaPlayerStreamStateProcs* in_procs)
@brief Type of callback function that opens URI stream
@details
Call this function when opening the URI stream.
*/
typedef int(*WKCMPOpenURIProc)(void* in_instance, const char* in_uri, const char* in_method, const WKCMediaPlayerStreamHeaderInfo* in_headers, int in_len, const WKCMediaPlayerStreamStateProcs* in_procs);
/**
@typedef int(*WKCMPCloseProc)(void* in_instance, int in_id)
@brief Type of callback function that closes stream
@details
Call this function when closing stream.
*/
typedef int(*WKCMPCloseProc)(void* in_instance, int in_id);
/**
@typedef int(*WKCMPCancelProc)(void* in_instance, int in_id)
@brief Type of callback function that cancels stream
@details
Call this function when canceling stream.
*/
typedef int(*WKCMPCancelProc)(void* in_instance, int in_id);
/**
@typedef int(*WKCMPOnCurlDebugProc)(void* in_instance, void* in_handle, int in_type, const char* in_data, unsigned int in_size)
@brief Called when curl calls back with debug information.
@details
Notify the events to WebInspector inside this function
*/
typedef void(*WKCMPOnCurlDebugProc)(void* in_instance, void* in_handle, int in_type, const char* in_data, unsigned int in_size);

/** @brief Structure that stores callback function for notifying media player */
struct WKCMediaPlayerCallbacks_ {
    /** @brief Member that holds WKCMPNotifyPlayerStateProc */
    WKCMPNotifyPlayerStateProc fNotifyPlayerState;
    /** @brief Member that holds WKCMPNotifyRequestInvalidateProc */
    WKCMPNotifyRequestInvalidateProc fNotifyRequestInvalidate;
    /** @brief Member that holds WKCMPNotifyAudioDataAvailableProc */
    WKCMPNotifyAudioDataAvailableProc fNotifyAudioDataAvailable;
    /** @brief Member that holds WKCMPNotifyKeyNeededProc (Encrypted Media Extensions 1.0b relevant) */
    WKCMPNotifyKeyNeededProc  fNotifyKeyNeeded;
    /** @brief Member that holds WKCMPNotifyKeyRequestProc (Encrypted Media Extensions 1.0b relevant) */
    WKCMPNotifyKeyRequestProc fNotifyKeyRequest;
    /** @brief Member that holds WKCMPNotifyKeyAddedProc (Encrypted Media Extensions 1.0b relevant) */
    WKCMPNotifyKeyAddedProc   fNotifyKeyAdded;
    /** @brief Member that holds WKCMPNotifyKeyErrorProc (Encrypted Media Extensions 1.0b relevant) */
    WKCMPNotifyKeyErrorProc   fNotifyKeyError;
    WKCMPChromeVisibleProc fChromeVisible;
    WKCMPCreateHTMLMediaElementProc fCreateHTMLMediaElement;
    WKCMPDestroyHTMLMediaElementProc fDestroyHTMLMediaElement;
    /** @brief Member that holds WKCMPOpenURIProc */
    WKCMPOpenURIProc fOpenURIProc;
    /** @brief Member that holds WKCMPCloseProc */
    WKCMPCloseProc fCloseProc;
    /** @brief Member that holds WKCMPCancelProc */
    WKCMPCancelProc fCancelProc;
    /** @brief Member that holds fOnCurlDebugProc */
    WKCMPOnCurlDebugProc fOnCurlDebugProc;
};
/** @brief Type definition of WKCMediaPlayerCallbacks */
typedef struct WKCMediaPlayerCallbacks_ WKCMediaPlayerCallbacks;

WKC_PEER_API void wkcMediaPlayerCallbackSetPeer(const WKCMediaPlayerProcs* in_procs);

/**
@brief Initializes media player
@retval !=false Succeeded
@retval ==false Failed
@details
Write the necessary processes for initializing the media player.
*/
WKC_PEER_API bool wkcMediaPlayerInitializePeer(void);
/**
@brief Finalizes media player
@details
Write the necessary processes for finalizing the media player.
*/
WKC_PEER_API void wkcMediaPlayerFinalizePeer(void);
/**
@brief Forcibly terminates media player
@details
Write the necessary processes for forcibly terminating the media player.
*/
WKC_PEER_API void wkcMediaPlayerForceTerminatePeer(void);
/**
@brief Determines whether MIME type is supported (Extended for Encrypted Media Extensions 1.0b)
@param in_mimetype MIME type string
@param in_codec Codec string
@param in_system Key system identify for EME, can be null if there is no need to check a key system (Encrypted Media Extensions 1.0b relevant)
@param in_url Play content URL (Encrypted Media Extensions 1.0b relevant)
@details
For EME
For MIME type represented by in_mimetype, in_codec, key system and URL, either of the following must be returned as a return value.
@retval WKC_MEDIA_SUPPORT_NOTSUPPORTED Unsupported
@retval WKC_MEDIA_SUPPORT_SUPPORTED Supported
@retval WKC_MEDIA_SUPPORT_MAYBESUPPORTED Supported (not guaranteed)
*/
WKC_PEER_API int wkcMediaPlayerIsSupportedMIMETypePeer(const char* in_mimetype, const char* in_codec, const char* in_system, const char* in_url);
/**
@brief Gets number of supported MIME types
@return Number of supported MIME types
@details
The number of supported MIME types must be returned as a return value.
*/
WKC_PEER_API int wkcMediaPlayerSupportedMIMETypesPeer(void);
/**
@brief Gets string of supported MIME type
@param in_index MIME type index
@return MIME string
@details
A smaller amount of value than the value returned by wkcMediaPlayerSupportedMIMETypesPeer() is passed to in_index.@n
The MIME type string that supports individual in_index must be returned as a return value.
*/
WKC_PEER_API const char* wkcMediaPlayerSupportedMIMETypePeer(int in_index);
/**
@brief Generate session ID for EME (Encrypted Media Extensions 1.0b relevant). Session ID was accquired by wkcMediaPlayerNotifyKeyRequest callback
@param in_self Media player instance
@param in_system Key system identifier
@param in_data CDM (Content Decryption Module) dependent container data
@param in_dataLen in_data length
@retval ==WKC_MEDIA_KEYEXCEPTION_NOERROR Suceeded
@retval ==WKC_MEDIA_KEYEXCEPTION_INVALIDPLAYERSTATE Throw INVALID_STATE_ERR
@retval ==WKC_MEDIA_KEYEXCEPTION_NOTSUPPORTED Throw NOT_SUPPORTED_ERR
@details
Refer to http://dvcs.w3.org/hg/html-media/raw-file/eme-v0.1b/encrypted-media/encrypted-media.html#dom-generatekeyrequest
*/
WKC_PEER_API int wkcMediaPlayerGenerateKeyRequestPeer(void* in_self, const char* in_system, const unsigned char* in_data, unsigned in_dataLen);
/**
@brief Callback function that gives notification of media player state (Encrypted Media Extensions 1.0b relevant)
@param in_self Media player instance
@param in_system Key system identifier
@param in_key Key data
@param in_keyLen key data length
@param in_data CDM (Content Decryption Module) dependent container data
@param in_dataLen in_data length
@param in_id Session ID
@retval ==WKC_MEDIA_KEYEXCEPTION_NOERROR Suceeded
@retval ==WKC_MEDIA_KEYEXCEPTION_INVALIDPLAYERSTATE Throw INVALID_STATE_ERR
@retval ==WKC_MEDIA_KEYEXCEPTION_NOTSUPPORTED Throw NOT_SUPPORTED_ERR
@retval ==WKC_MEDIA_KEYEXCEPTION_INVALIDACCESS Throw INVALID_ACCESS_ERR
@details
Refer to http://dvcs.w3.org/hg/html-media/raw-file/eme-v0.1b/encrypted-media/encrypted-media.html#dom-addkey
*/
WKC_PEER_API int wkcMediaPlayerAddKeyPeer(void* in_self, const char* in_system, const unsigned char* in_key, unsigned in_keyLen, const unsigned char* in_data, unsigned in_dataLen, const char* in_id);
/**
@brief Cancel accquire key process, and return the media element state so that generateKeyRequest() may be called again (Encrypted Media Extensions 1.0b relevant)
@param in_self Media player instance
@param in_system Key system identifier
@param in_id Session ID
@retval ==WKC_MEDIA_KEYEXCEPTION_NOERROR Suceeded
@retval ==WKC_MEDIA_KEYEXCEPTION_INVALIDPLAYERSTATE Throw INVALID_STATE_ERR
@retval ==WKC_MEDIA_KEYEXCEPTION_NOTSUPPORTED Throw NOT_SUPPORTED_ERR
@retval ==WKC_MEDIA_KEYEXCEPTION_INVALIDACCESS Throw INVALID_ACCESS_ERR
@details
Refer to http://dvcs.w3.org/hg/html-media/raw-file/eme-v0.1b/encrypted-media/encrypted-media.html#dom-cancelkeyrequest
*/
WKC_PEER_API int wkcMediaPlayerCancelKeyRequestPeer(void* in_self, const char* in_system, const char* in_id);
/**
@brief Gets strings used for displaying UI
@param in_type String type
@return strings used for displaying UI
@details
The UI string represented by in_type must be returned as a return value. The following value is passed to in_type:
@li WKC_MEDIA_UISTRING_LOADING Loading
@li WKC_MEDIA_UISTRING_BROADCAST Broadcast
@attention The Peer implementer must manage the memory that stores strings.
*/
WKC_PEER_API const char* wkcMediaPlayerGetUIStringPeer(int in_type);
/**
@brief Gets string used for displaying time
@param in_time Time (sec)
@return string used for displaying time
@details
String that represents time passed by in_time must be returned as a return value.
@attention The Peer implementer must manage the memory that stores strings.
*/
WKC_PEER_API const char* wkcMediaPlayerGetUIStringTimePeer(float in_time);

/**
@brief Generates media player
@param in_callbacks Callback for notification
@param in_scallbacks Callback for notifying of stream event
@param in_opaque Callback argument
@return Media player instance
@details
Write the generating process of the media player instance.@n
The media player instance specified for return value is passed as in_self of individual wkcMediaPlayerXXXXPeer().@n
For notifying the media player, in_opaque must be specified when calling the function stored in in_callbacks.
@attention The Peer implementer must not return NULL even if media player instance creation failed. The Peer should return some instances as dummy media player instances in these case.
*/
WKC_PEER_API void* wkcMediaPlayerCreatePeer(const WKCMediaPlayerCallbacks* in_callbacks, void* in_opaque);
/**
@brief Discards media player
@param in_self Media player instance
@details
Write the discard process of the media player instance.
*/
WKC_PEER_API void wkcMediaPlayerDeletePeer(void* in_self);

/**
@brief Loads content
@param in_self Media player instance
@param in_uri Content URI
@param in_mediatype Media type
@li WKC_MEDIA_TYPE_NORMAL Normal content
@li WKC_MEDIA_TYPE_MEDIASOURCE Media source
@li WKC_MEDIA_TYPE_MEDIASTREAM Media stream
@retval WKC_MEDIA_ERROR_OK Succeeded
@retval WKC_MEDIA_ERROR_GENERIC Failed
@retval WKC_MEDIA_ERROR_NOTREADY Preparation not complete
@details
Write the load process of content.
@attention
If MediaPlayer loads "window.URL.createObjectURL(MediaSource object)", in_mediatype == WKC_MEDIA_TYPE_MEDIASOURCE.@n
If MediaPlayer loads "window.URL.createObjectURL(LocalMediaStream object)", in_mediatype == WKC_MEDIA_TYPE_MEDIASTREAM.@n
Otherwise, in_mediatype == WKC_MEDIA_TYPE_NORMAL.@n
Currently, there are no differences in operation by return value.
*/
WKC_PEER_API int wkcMediaPlayerLoadPeer(void* in_self, const char* in_uri, int in_mediatype);
/**
@brief Cancels loading content
@param in_self Media player instance
@retval WKC_MEDIA_ERROR_OK Succeeded
@retval WKC_MEDIA_ERROR_GENERIC Failed
@retval WKC_MEDIA_ERROR_NOTREADY Preparation not complete
@details
Write the process for canceling the loading of content.
@attention
Currently, there are no differences in operation by return value.
*/
WKC_PEER_API int wkcMediaPlayerCancelLoadPeer(void* in_self);
/**
@brief Prepares content playback
@param in_self Media player instance
@retval WKC_MEDIA_ERROR_OK Succeeded
@retval WKC_MEDIA_ERROR_GENERIC Failed
@retval WKC_MEDIA_ERROR_NOTREADY Preparation not complete
@details
Write the process before content playback.
@attention
Currently, there are no differences in operation by return value.
*/
WKC_PEER_API int wkcMediaPlayerPrepareToPlayPeer(void* in_self);
/**
@brief Plays content
@param in_self Media player instance
@retval WKC_MEDIA_ERROR_OK Succeeded
@retval WKC_MEDIA_ERROR_GENERIC Failed
@retval WKC_MEDIA_ERROR_NOTREADY Preparation not complete
@details
Write the playback process of content.
@attention
Currently, there are no differences in operation by return value.
*/
WKC_PEER_API int wkcMediaPlayerPlayPeer(void* in_self);
/**
@brief Pauses content
@param in_self Media player instance
@retval WKC_MEDIA_ERROR_OK Succeeded
@retval WKC_MEDIA_ERROR_GENERIC Failed
@retval WKC_MEDIA_ERROR_NOTREADY Preparation not complete
@details
Write the pause process of content.
@attention
Currently, there are no differences in operation by return value.
*/
WKC_PEER_API int wkcMediaPlayerPausePeer(void* in_self);
/**
@brief Determines whether content is paused
@param in_self Media player instance
@retval !=false Paused
@retval ==false Not paused
@details
Whether content is paused must be returned as a return value.
*/
WKC_PEER_API bool wkcMediaPlayerIsPausedPeer(void* in_self);

/**
@brief Determines whether video player is usable
@param in_self Media player instance
@retval !=false Video player is usable
@retval ==false No video player is usable
@details
Whether video player is usable must be returned as a return value.
*/
WKC_PEER_API bool wkcMediaPlayerHasVideoPeer(void* in_self);
/**
@brief Determines whether audio player is usable
@param in_self Media player instance
@retval !=false Audio player is usable
@retval ==false No audio player is usable
@details
Whether audio player is usable must be returned as a return value.
*/
WKC_PEER_API bool wkcMediaPlayerHasAudioPeer(void* in_self);
/**
@brief Determines whether media player is displayable in full screen
@param in_self Media player instance
@retval !=false Full screen display available
@retval ==false Full screen display not available
@details
Whether the media player is displayable in full screen must be returned as a return value.
*/
WKC_PEER_API bool wkcMediaPlayerSupportsFullScreenPeer(void* in_self);
/**
@brief Determines whether file can be saved
@param in_self Media player instance
@retval !=false File can be saved
@retval ==false File cannot be saved
@details
Whether a file can be saved must be returned as a return value.
*/
WKC_PEER_API bool wkcMediaPlayerCanSaveMediaDataPeer(void* in_self);
/**
@brief Determines whether closed caption is supported
@param in_self Media player instance
@retval !=false Closed caption is supported
@retval ==false Closed caption is not supported
@details
Whether close caption is supported must be returned as a return value.
*/
WKC_PEER_API bool wkcMediaPlayerHasClosedCaptionsPeer(void* in_self);
/**
@brief Toggles closed caption display
@param in_self Media player instance
@param in_flag Display setting value
@li !=false Display
@li ==false Do not display
@retval WKC_MEDIA_ERROR_OK Success
@retval Other Fail
@details
Write the toggle process of closed caption display in accordance with in_flag.
*/
WKC_PEER_API int wkcMediaPlayerSetClosedCaptionsVisiblePeer(void* in_self, bool in_flag);
/**
@brief Determines whether content is generated from single source
@param in_self Media player instance
@retval !=false Content resource is generated from single source
@retval ==false Content resource is not generated from single source
@details
The determination of whether resource included in content is generated from single source must be returned as a return value.
*/
WKC_PEER_API bool wkcMediaPlayerHasSingleSecurityOriginPeer(void* in_self);

/**
@brief Gets total size of content
@param in_self Media player instance
@return Size of content (bytes)
@details
The total size of content must be returned as a return value.
*/
WKC_PEER_API unsigned int wkcMediaPlayerTotalBytesPeer(void* in_self);

/**
@brief Gets natural size
@param in_self Media player instance
@param out_size Size
@retval WKC_MEDIA_ERROR_OK Succeeded
@retval WKC_MEDIA_ERROR_GENERIC Failed
@retval WKC_MEDIA_ERROR_NOTREADY Preparation not complete
@details
The natural size of the player must be set for out_size.
*/
WKC_PEER_API int wkcMediaPlayerNaturalSizePeer(void* in_self, WKCSize* out_size);
/**
@brief Sets size of media player
@param in_self Media player instance
@param in_size Size
@retval WKC_MEDIA_ERROR_OK Succeeded
@retval WKC_MEDIA_ERROR_GENERIC Failed
@retval WKC_MEDIA_ERROR_NOTSUPPORTED Unsupported
@retval WKC_MEDIA_ERROR_NOTREADY Preparation not complete
@details
The player must be set with the size specified by in_size.
*/
WKC_PEER_API int wkcMediaPlayerSetSizePeer(void* in_self, const WKCSize* in_size);
/**
@brief Sets preload of media player
@param in_self Media player instance
@param in_type Preload type
@retval WKC_MEDIA_ERROR_OK Succeeded
@retval WKC_MEDIA_ERROR_GENERIC Failed
@retval WKC_MEDIA_ERROR_NOTSUPPORTED Unsupported
@retval WKC_MEDIA_ERROR_NOTREADY Preparation not complete
@details
The player can use the preload specified by in_type.
*/
WKC_PEER_API int wkcMediaPlayerSetPreloadPeer(void* in_self, int in_type);
/**
@brief Sets media player visibility
@param in_self Media player instance
@param in_flag Visibility setting
@retval WKC_MEDIA_ERROR_OK Succeeded
@retval WKC_MEDIA_ERROR_GENERIC Failed
@retval WKC_MEDIA_ERROR_NOTREADY Preparation not complete
@details
Set the player to visible if it is in_flag != false, and to invisible if it is in_flag == false.
@attention
Currently, there are no differences in operation by return value.
*/
WKC_PEER_API int wkcMediaPlayerSetPageIsVisiblePeer(void* in_self, bool in_flag);

/**
@brief Gets content duration
@param in_self Media player instance
@return Content duration (seconds)
@details
The content duration (the duration attribute value) must be returned as a return value.
@attention
This function is obsolete. wkcMediaPlayerDurationDoublePeer() would be used.
*/
WKC_PEER_API float wkcMediaPlayerDurationPeer(void* in_self);
/**
@brief Gets content duration
@param in_self Media player instance
@return Content duration (seconds)
@details
The content duration (the duration attribute value) must be returned as a return value.
*/
WKC_PEER_API double wkcMediaPlayerDurationDoublePeer(void* in_self);
/**
@brief Gets content playback start position
@param in_self Media player instance
@return Content playback start position (elapsed seconds from beginning)
@details
The time of the content playback start position from the beginning must be returned as a return value.
*/
WKC_PEER_API float wkcMediaPlayerStartTimePeer(void* in_self);
/**
@brief Gets current content position
@param in_self Media player instance
@return Current position of content (elapsed seconds from beginning)
@details
The time of the current content playback position from the beginning must be returned as a return value.
@attention
This function is obsolete. wkcMediaPlayerCurrentTimeDoublePeer() would be used.
*/
WKC_PEER_API float wkcMediaPlayerCurrentTimePeer(void* in_self);
/**
@brief Gets current content position
@param in_self Media player instance
@return Current position of content (elapsed seconds from beginning)
@details
The time of the current content playback position from the beginning must be returned as a return value.
*/
WKC_PEER_API double wkcMediaPlayerCurrentTimeDoublePeer(void* in_self);
/**
@brief Gets the maximum seekable position of content
@param in_self Media player instance
@return Maximum seekable position
@details
The maximum seekable position of content must be returned as a return value.
*/
WKC_PEER_API float wkcMediaPlayerMaxTimeSeekablePeer(void* in_self);
/**
@brief Seeks content
@param in_self Media player instance
@param in_time Seek time
@retval WKC_MEDIA_ERROR_OK Succeeded
@retval WKC_MEDIA_ERROR_GENERIC Failed
@retval WKC_MEDIA_ERROR_NOTSUPPORTED Unsupported
@retval WKC_MEDIA_ERROR_NOTREADY Preparation not complete
@details
Write the seeking operation process for the position represented by in_time from the beginning.
@attention
Currently, there are no differences in operation by return value.
*/
WKC_PEER_API int wkcMediaPlayerSeekPeer(void* in_self, float in_time);
/**
@brief Determines whether content is being seeked
@param in_self Media player instance
@retval !=false Seeking
@retval ==false Not seeking
@details
Write the process for determining whether content is being seeked.
*/
WKC_PEER_API bool wkcMediaPlayerIsSeekingPeer(void* in_self);

/**
@brief Gets content playback speed factor
@param in_self Media player instance
@retval content playback speed factor
@details
The content playback speed factor (the playbackRate attribute value) must be returned as a return value.
*/
WKC_PEER_API double wkcMediaPlayerRatePeer(void* in_self);

/**
@brief Sets content playback speed factor
@param in_self Media player instance
@param in_rate Playback rate
@retval WKC_MEDIA_ERROR_OK Succeeded
@retval WKC_MEDIA_ERROR_GENERIC Failed
@retval WKC_MEDIA_ERROR_NOTSUPPORTED Unsupported
@retval WKC_MEDIA_ERROR_NOTREADY Preparation not complete
@details
Write the process for setting the playback speed factor specified by in_rate.
@attention
Currently, there are no differences in operation by return value.
*/
WKC_PEER_API int wkcMediaPlayerSetRatePeer(void* in_self, float in_rate);
/**
@brief Sets audio playback pitch
@param in_self Media player instance
@param in_flag Sets audio playback pitch
@li in_flag!=false Saves normal pitch when playback speed factor changes
@li in_flag==false Does not save normal pitch when playback speed factor changes
@retval WKC_MEDIA_ERROR_OK Succeeded
@retval WKC_MEDIA_ERROR_GENERIC Failed
@retval WKC_MEDIA_ERROR_NOTSUPPORTED Unsupported
@retval WKC_MEDIA_ERROR_NOTREADY Preparation not complete
@details
Set the audio playback pitch represented by in_flag.
@attention
Currently, there are no differences in operation by return value.
*/
WKC_PEER_API int wkcMediaPlayerSetPreservesPitchPeer(void* in_self, bool in_flag);

/**
@brief Gets volume
@param in_self Media player instance
@retval volume
@details
The volume (the volume attribute value) must be returned as a return value.
*/
WKC_PEER_API float wkcMediaPlayerVolumePeer(void* in_self);

/**
@brief Sets volume
@param in_self Media player instance
@param in_volume Volume (0 <= in_volume <= 1.0)
@retval WKC_MEDIA_ERROR_OK Succeeded
@retval WKC_MEDIA_ERROR_GENERIC Failed
@retval WKC_MEDIA_ERROR_NOTSUPPORTED Unsupported
@retval WKC_MEDIA_ERROR_NOTREADY Preparation not complete
@details
Write the process for setting the volume specified by in_volume.
@attention
Currently, there are no differences in operation by return value.
*/
WKC_PEER_API int wkcMediaPlayerSetVolumePeer(void* in_self, float in_volume);

/**
@brief Gets media player network state
@param in_self Media player instance
@retval WKC_MEDIA_NETWORKSTATE_EMPTY Not initialized
@retval WKC_MEDIA_NETWORKSTATE_IDLE Idle state
@retval WKC_MEDIA_NETWORKSTATE_LOADING Loading content
@retval WKC_MEDIA_NETWORKSTATE_LOADED Loaded content
@retval WKC_MEDIA_NETWORKSTATE_FORMATERROR Invalid content format
@retval WKC_MEDIA_NETWORKSTATE_NETWORKERROR Network state error
@retval WKC_MEDIA_NETWORKSTATE_DECODEERROR Failed to decode
@details
The appropriate network state of the media player must be returned as a return value.
*/
WKC_PEER_API int wkcMediaPlayerNetworkStatePeer(void* in_self);
/**
@brief Gets content hold state of media player
@param in_self Media player instance
@retval WKC_MEDIA_READYSTATE_HAVE_NOTHING Not held
@retval WKC_MEDIA_READYSTATE_HAVE_METADATA Meta data held
@retval WKC_MEDIA_READYSTATE_HAVE_CURRENTDATA Playback content until current position held
@retval WKC_MEDIA_READYSTATE_HAVE_FUTUREDATA Playback content from current position until some future position held
@retval WKC_MEDIA_READYSTATE_HAVE_ENOUGHDATA Assumed playback content from current position until last held
@details
The appropriate content hold state of the media player must be returned as a return value.@n
*/
WKC_PEER_API int wkcMediaPlayerReadyStatePeer(void* in_self);
/**
@brief Gets playback type of video content
@param in_self Media player instance
@retval WKC_MEDIA_MOVIELOADTYPE_UNKNOWN Unknown
@retval WKC_MEDIA_MOVIELOADTYPE_DOWNLOAD Download
@retval WKC_MEDIA_MOVIELOADTYPE_STOREDSTREAM Pseudo streaming
@retval WKC_MEDIA_MOVIELOADTYPE_LIVESTREAM Streaming
@details
The playback type of content must be returned as a return value.
*/
WKC_PEER_API int wkcMediaPlayerMovieLoadTypePeer(void* in_self);

/**
@brief Gives notification of forced redrawing
@param in_self Media player instance
@retval WKC_MEDIA_ERROR_OK Succeeded
@retval WKC_MEDIA_ERROR_GENERIC Failed
@retval WKC_MEDIA_ERROR_NOTREADY Preparation not complete
@details
Write the process for redrawing the whole media player regardless of the state.@n
* Currently not used so it is okay to implement an empty function.
*/
WKC_PEER_API int wkcMediaPlayerForceRepaintPeer(void* in_self);
/**
@brief Sets media player window position
@param in_self Media player instance
@param in_rect Window position
@retval WKC_MEDIA_ERROR_OK Succeeded
@retval WKC_MEDIA_ERROR_GENERIC Failed
@retval WKC_MEDIA_ERROR_NOTREADY Preparation not complete
@details
Write the process for setting the media player to the position specified by in_rect.@n
* Currently not used so it is okay to implement an empty function.
*/
WKC_PEER_API int wkcMediaPlayerSetWindowPositionPeer(void* in_self, const WKCRect* in_rect);
/**
@brief Gets video drawing area type
@param in_self Media player instance
@retval WKC_MEDIA_VIDEOSINKTYPE_BITMAP Bitmap buffer stream
@retval WKC_MEDIA_VIDEOSINKTYPE_WINDOW Inside separate window
@retval WKC_MEDIA_VIDEOSINKTYPE_HOLEDWINDOW Inside holed parent window
@details
The drawing area type of video content must be returned as a return value.@n
Drawing video is performed using wkcMediaPlayerLockImagePeer() on the browser engine side only when WKC_MEDIA_VIDEOSINKTYPE_BITMAP is returned. In other cases, the media player must draw video appropriately.
*/
WKC_PEER_API int wkcMediaPlayerVideoSinkTypePeer(void* in_self);
/**
@brief Gets video layer for accelerated compositing if supported
@param in_self Media player instance
@retval layer
@details
The layer should be the same type of instance created by wkcLayerNewPeer().
*/
WKC_PEER_API void* wkcMediaPlayerVideoLayerPeer(void* in_self);
/**
@brief Notifies video rendering state for accelerated-composited layer changed
@param in_self Media player instance
@details
Notifies video rendering state for accelerated-composited layer changed.
*/
WKC_PEER_API void wkcMediaPlayerNotifyVideoLayerRenderingStateChangedPeer(void* in_self);
/**
@brief Sets image format
@param in_self Media player instance
@param in_fmt Format
@retval WKC_MEDIA_ERROR_OK Succeeded
@retval WKC_MEDIA_ERROR_GENERIC Failed
@retval WKC_MEDIA_ERROR_NOTSUPPORTED Unsupported
@retval WKC_MEDIA_ERROR_NOTREADY Preparation not complete
@details
* Currently not used so it is okay to implement an empty function.
*/
WKC_PEER_API int wkcMediaPlayerSetPreferredImageFormatPeer(void* in_self, int in_fmt);

/**
@brief Gets number of buffered contents
@param in_self Media player instance
@return Number of content items
@details
The number of buffered content items must be returned as a return value.
*/
WKC_PEER_API int wkcMediaPlayerBufferedRangesPeer(void* in_self);
/**
@brief Gets buffered position of content
@param in_self Media player instance
@param in_index Index
@param out_start Start position
@param out_end End position
@retval WKC_MEDIA_ERROR_OK Succeeded
@retval WKC_MEDIA_ERROR_GENERIC Failed
@retval WKC_MEDIA_ERROR_NOTSUPPORTED Unsupported
@retval WKC_MEDIA_ERROR_NOTREADY Preparation not complete
@details
Set the buffered content positions in out_start and out_end.@n
The value corresponding to the value returned by wkcMediaPlayerBufferedRangesPeer() is passed to in_index.
*/
WKC_PEER_API int wkcMediaPlayerBufferedRangePeer(void* in_self, int in_index, float* out_start, float* out_end);

/**
@brief Locks drawing buffer
@param in_self Media player instance
@param out_fmt Format
@param out_rowbytes Number of bytes in horizontal direction of drawing buffer
@param out_mask Mask image
@param out_maskrowbytes Number of bytes in horizontal direction of mask image buffer
@param out_size Drawing buffer size
@return Pointer to drawing buffer
@details
Write the processes for locking the drawing buffer for reading data and returning a pointer to the buffer as a return value.@n
The out_fmt format must be specified using the following symbols:
@li WKC_IMAGETYPE_RGBA8888	0x00000000
@li WKC_IMAGETYPE_RGAB5515	0x00000001
@li WKC_IMAGETYPE_RGAB5515MASK	0x00000002
@li WKC_IMAGETYPE_ARGB1232	0x00000003
@li WKC_IMAGETYPE_TEXTURE	0x00000004
@li WKC_IMAGETYPE_TYPEMASK	0x0000ffff
@li WKC_IMAGETYPE_FLAG_HASALPHA	0x00010000
@li WKC_IMAGETYPE_FLAG_HASTRUEALPHA	0x00020000
@li WKC_IMAGETYPE_FLAG_FORSKIN	0x00100000
@li WKC_IMAGETYPE_FLAG_STEREO_M	0x00000000
@li WKC_IMAGETYPE_FLAG_STEREO_L	0x01000000
@li WKC_IMAGETYPE_FLAG_STEREO_R	0x02000000
@li WKC_IMAGETYPE_FLAG_STEREOMASK	0x0f000000
*/
WKC_PEER_API void* wkcMediaPlayerLockImagePeer(void* in_self, int* out_fmt, int* out_rowbytes, void** out_mask, int* out_maskrowbytes, WKCSize* out_size);
/**
@brief Unlocks drawing buffer
@param in_self Media player instance
@param image Pointer to drawing buffer
@details
Write the process for unlocking the drawing buffer specified by image.
*/
WKC_PEER_API void wkcMediaPlayerUnlockImagePeer(void* in_self, void* image);

/**
@brief Set loop
@param in_self Media player instance
@param in_loop Loop setting
@details
Write the process for setting the loop specified by in_loop.
*/
WKC_PEER_API int wkcMediaPlayerSetLoopPeer(void* in_self, bool in_loop);

/**
@brief Do something for when the media element is removed from a document.
@param in_self Media player instance
@details
Write a process for when the media element is removed from a document.
@attention
This call doesn't mean the media element was deleted.
The media element can be attached again to a document later by javascript.
*/
WKC_PEER_API void wkcMediaPlayerRemovedFromDocumentPeer(void* in_self);

WKC_PEER_API void wkcMediaPlayerWillBecomeInactivePeer(void* in_self);

WKC_PEER_API void wkcMediaPlayerEnterFullscreenPeer(void* in_self);

WKC_PEER_API void wkcMediaPlayerExitFullscreenPeer(void* in_self);

WKC_PEER_API bool wkcMediaPlayerCanEnterFullscreenPeer(void* in_self);

/** @brief Structure that stores information about VideoPlaybackQualityMetrics */
struct WKCMediaVideoPlaybackQualityMetrics_ {
    /** @brief total video frames */
    uint32_t fTotalVideoFrames;
    /** @brief dropped video frames */
    uint32_t fDroppedVideoFrames;
};
/** @brief Type definition of WKCMediaPlayerVideoPlaybackQualityMetrics */
typedef struct WKCMediaVideoPlaybackQualityMetrics_ WKCMediaVideoPlaybackQualityMetrics;
WKC_PEER_API WKCMediaVideoPlaybackQualityMetrics wkcMediaPlayerVideoPlaybackQualityMetricsPeer(void* in_self);

/**
@brief Gets output format of audio data
@param in_self Device descriptor
@retval WKC_MEDIA_AUDIOSINKTYPE_DIRECT Direct output
@retval WKC_MEDIA_AUDIOSINKTYPE_BINARYSTREAM Binary data
@retval WKC_MEDIA_ERROR_NOTREADY Preparation not complete
@details
Write the process for returning output format.
*/
WKC_PEER_API int wkcMediaPlayerAudioSinkTypePeer(void* in_self);
/**
@brief Gets audio channel
@param in_self Device descriptor
@retval WKC_MEDIA_ERROR_NOTREADY Preparation not complete
@retval Audio channel
@details
Write the process for returning audio channel.
*/
WKC_PEER_API int wkcMediaPlayerAudioChannelsPeer(void* in_self);
/**
@brief Gets audio rate
@param in_self Device descriptor
@retval WKC_MEDIA_ERROR_NOTREADY Preparation not complete
@retval Audio rate
@details
Write the process for returning audio rate.
*/
WKC_PEER_API int wkcMediaPlayerAudioRatePeer(void* in_self);
/**
@brief Gets sampling rate
@param in_self Device descriptor
@param out_endian Endian
@retval WKC_MEDIA_ERROR_NOTREADY Preparation not complete
@retval Sampling rate
@details
Write the process for returning sampling rate.
*/
WKC_PEER_API int wkcMediaPlayerAudioBitsPerSamplePeer(void* in_self, int* out_endian);
/**
@brief Locks audio buffer
@param in_self Device descriptor
@param out_len Length of buffer
@retval Pointer to buffer
@details
Write the process for returning a pointer to the locked buffer.
*/
WKC_PEER_API void* wkcMediaPlayerLockAudioPeer(void* in_self, unsigned int* out_len);
/**
@brief Unlocks audio buffer
@param in_self Device descriptor
@param in_consumed Usage
@retval None
@details
Write the process for releasing the buffer locked by wkcMediaPlayerLockAudioPeer.
*/
WKC_PEER_API void wkcMediaPlayerUnlockAudioPeer(void* in_self, unsigned int in_consumed);

// for webAudio
/**
@brief Sets audio resource path
@param in_path Path of the storage of audio resources
@retval None
@details
Sets audio resource path.
*/
WKC_PEER_API void wkcMediaPlayerSetAudioResourcesPathPeer(const char* in_path);
/**
@brief Reads an audio resource
@param in_name Resource name
@param out_buf Buffer for read resource. If NULL, do nothing.
@param in_buflen Max length of out_buf
@retval Size of the resource
@details
For webAudio. Reads the audio resource specified in in_name.@n
At first, this peer will be called with out_buf==NULL, and this peer should return size of the specified resource.@n
Then, core will allocate buffer and call this function again. This peer should store the resource to out_buf.
*/
WKC_PEER_API int wkcMediaPlayerLoadPlatformAudioResourcePeer(const char* in_name, void* out_buf, int in_buflen);
/**
@brief Decodes encoded audio data.
@param in_data Encoded audio data
@param in_len Length of in_data
@param out_buf buffer for decoded data. If NULL, do nothing.
@param in_buflen Max length of out_buf
@param out_samplerate Sample rate of the decoded data
@param out_bitspersample Sample bits of the decoded data
@param out_channels channels of the decoded data
@param out_endian Endianness of the decoded data
@retval Size of decoded data
@details
For webAudio. Decodes encoded audio data on memory.@n
The calling sequences is same as wkcMediaPlayerLoadPlatformAudioResourcePeer.
*/
WKC_PEER_API int wkcMediaPlayerDecodeAudioDataPeer(const void* in_data, int in_len, void* out_buf, int in_buflen, int* out_samplerate, int* out_bitspersample, int* out_channels, int* out_endian);


/**
@brief Gets style sheet for media player controller
@retval "== NULL" Failed, or there is no the Style Sheet
@retval "!= NULL" Style Sheet String
@details
The style sheet string for the media player controller must be returned as a return value.
*/
WKC_PEER_API const char* wkcMediaPlayerControlsGetStyleSheetPeer(void);
/**
@brief Gets JavaScript for media player controller
@retval "== NULL" Failed, or there is no the Style Sheet
@retval "!= NULL" Style Sheet String
@details
The JavaScript string for the media player controller must be returned as a return value.
*/
WKC_PEER_API const char* wkcMediaPlayerControlsGetScriptPeer(void);

/**
@brief Registers style sheet for media player controller
@param in_css Style sheet
@details
Registers style sheet for media player controller
*/
WKC_PEER_API void wkcMediaPlayerControlsRegisterStyleSheetPeer(const char* in_css);
/**
@brief Registers JavaScript for media player controller
@param in_js JavaScript
@details
Registers JavaScript for media player controller
*/
WKC_PEER_API void wkcMediaPlayerControlsRegisterScriptPeer(const char* in_js);

WKC_PEER_API void wkcAudioCallbackSetPeer(const WKCAudioProcs* in_procs);
/**
@brief Initializes audio device
@retval !=false Succeeded
@retval ==false Failed
@details
Write the necessary processes for initializing the audio device.
*/
WKC_PEER_API bool wkcAudioInitializePeer(void);
/**
@brief Finalizes audio device
@details
Write the necessary processes for finalizing the audio device.
*/
WKC_PEER_API void wkcAudioFinalizePeer(void);
/**
@brief Forcibly terminates audio device
@details
Write the necessary processes for forcibly terminating the audio device.
*/
WKC_PEER_API void wkcAudioForceTerminatePeer(void);

/**
@brief Opens audio device
@param in_samplerate Sampling rate
@param in_bitspersample Number of bits per sample
@param in_channels Number of channels
@param in_endian Byte endian
@li WKC_MEDIA_ENDIAN_LITTLEENDIAN Little endian
@li WKC_MEDIA_ENDIAN_BIGENDIAN Big endian
@li WKC_MEDIA_ENDIAN_MIDDLEENDIAN Middle endian
@return Device descriptor
@details
Write the process for opening the audio device with the conditions specified by the argument.@n
The return value is passed to in_self using wkcAudioXXXXPeer().
*/
WKC_PEER_API void* wkcAudioOpenPeer(int in_samplerate, int in_bitspersample, int in_channels, int in_endian);
/**
@brief Closes audio device
@param in_self Device descriptor
@details
Write the process for closing the audio device.
*/
WKC_PEER_API void wkcAudioClosePeer(void* in_self);
/**
@brief Sets audio device volume
@param in_self Device descriptor
@param in_volume Volume (0.0<=in_volume<=1.0)
@retval WKC_MEDIA_ERROR_OK Succeeded
@retval WKC_MEDIA_ERROR_GENERIC Failed
@details
Set the audio device volume to the value specified by in_volume.
*/
WKC_PEER_API int wkcAudioSetVolumePeer(void* in_self, float in_volume);
/**
@brief Gets audio device volume
@param in_self Device descriptor
@return Audio device volume
@details
The audio device volume must be returned as a return value within the range from 0.0 to 1.0.@n
Return a negative value if the process fails.
*/
WKC_PEER_API float wkcAudioGetVolumePeer(void* in_self);
/**
@brief Outputs to audio device
@param in_self Device descriptor
@param in_data Stream
@param in_len Stream length
@retval Size of consumed data
@details
Write the process for outputting the stream specified by in_data and in_len to the device.
*/
WKC_PEER_API unsigned int wkcAudioWritePeer(void* in_self, void* in_data, unsigned int in_len);
/**
@brief Outputs to audio device
@param in_self Device descriptor
@param in_data Pointer to array of channel data
@param in_channels Number of channels
@param in_frame_count Count of frames in each channel data
@param in_max_abs_value Pointer to array of the maximum absolute value in each channel data
@retval !=false Succeeded
@retval ==false Failed
@details
Write the process for outputting the stream specified by in_data to the device.
If the return value is not false, wkcAudioWriteRawPeer will be called again with the next data after some sleep.
If the return value is false, wkcAudioWriteRawPeer will be called again with the same data after some sleep.
*/
WKC_PEER_API bool wkcAudioWriteRawPeer(void* in_self, float** in_data, unsigned int in_channels, unsigned int in_frame_count, float* in_max_abs_value);
/**
@brief Returns preferred sample rate of audio device
@retval Preferred sample rate of audio device
@details
Returns preferred sample rate of audio device.
*/
WKC_PEER_API int wkcAudioPreferredSampleRatePeer(void);

/**
@brief Initializes FFT functionality
@details
Initializes FFT functionality.
*/
WKC_PEER_API void wkcAudioFFTInitializePeer(void);
/**
@brief Creates FFT conversion functionality instance
@param in_size Size of FFT conversion
@retval Instance
@details
Creates FFT conversion functionality instance.
*/
WKC_PEER_API void* wkcAudioFFTCreatePeer(int in_size);
/**
@brief Deletes FFT conversion functionality instance
@param in_self Instance
@details
Deletes FFT conversion functionality instance.
*/
WKC_PEER_API void wkcAudioFFTDeletePeer(void* in_self);
/**
@brief Do FFT conversion
@param in_self Instance
@param in_data Input data
@param out_real Real part of FFT converted data
@param out_imag Imaginary part of FFT converted data
@details
Do FFT conversion.
*/
WKC_PEER_API void wkcAudioFFTDoFFTPeer(void* in_self, const float* in_data, float* out_real, float* out_imag);
/**
@brief Do inverse FFT conversion
@param in_self Instance
@param out_data Output data
@param in_real Real part of input data
@param in_imag Imaginary part of input data
@details
Do inverse FFT conversion.
*/
WKC_PEER_API void wkcAudioFFTDoInverseFFTPeer(void* in_self, float* out_data, const float* in_real, const float* in_imag);

/** @brief Structure that stores information about media segment */
struct WKCMediaTrack_ {
    /** @brief track ID*/
    int fID;
    /** @brief codec */
    const char* fCodec;
    /** @brief kind@n
        WKC_MEDIA_SB_TRACKKIND_AUDIO kind of media is audio@n
        WKC_MEDIA_SB_TRACKKIND_VIDEO kind of media is video */
    int fKind;
};
/** @brief Type definition of WKCMediaTrack */
typedef struct WKCMediaTrack_ WKCMediaTrack;

/**
@typedef void (*WKCSBDidReceiveInitializationSegmentProc)(void* in_opaque, const WKCMediaTrack* in_tracks, int in_tracks_len, double in_duration)
@brief Notify new initialization segment was append (Media Source Extensions Draft relevant)
@param in_opaque Callback argument passed to wkcMediaPlayerAddSBPeer()
@param in_tracks pointer to array of WKCMediaTrack
@param in_tracks_len length of array of WKCMediaTrack
@param in_duration duration information contained in initialization segment
@details
Notify new initialization segment was append in wkcMediaPlayerAppendToSBPeer().
*/
typedef void (*WKCSBDidReceiveInitializationSegmentProc)(void* in_opaque, const WKCMediaTrack* in_tracks, int in_tracks_len, double in_duration);

/** @brief Structure that stores information about media segment */
struct WKCMediaSample_ {
    /** @brief track ID*/
    int fID;
    /** @brief presentation timestamp of media segment@n
        Refer to http://www.w3.org/TR/media-source/#presentation-timestamp */
    long long fPresentationTime;
    /** @brief decode timestamp of media segment@n
        Refer to http://www.w3.org/TR/media-source/#decode-timestamp */
    long long fDecodeTime;
    /** @brief coded frame duration@n
        Refer to http://www.w3.org/TR/media-source/#coded-frame-duration */
    long long fDuration;
    /** @brief size in bytes*/
    long long fSize;
    /** @brief random access point
        Refer to https://www.w3.org/TR/media-source/#random-access-point */
    bool fIsSync;
    /** @brief dropped flag */
    bool fIsDropped;
};
/** @brief Type definition of WKCMediaSample */
typedef struct WKCMediaSample_ WKCMediaSample;

/**
@typedef void (*WKCSBDidReceiveSampleProc)(void* in_opaque, WKCMediaSample* inout_sample)
@brief Notify new media segment was append (Media Source Extensions Draft relevant)
@param in_opaque Callback argument passed to wkcMediaPlayerAddSBPeer()
@param in_sample pointer to WKCMediaSample
@details
Notify new media segment was append in wkcMediaPlayerAppendToSBPeer().
*/
typedef void (*WKCSBDidReceiveSampleProc)(void* in_opaque, WKCMediaSample* inout_sample);
/**
@typedef void (*WKCSBAppendCompleteProc)(void* in_opaque, int in_appendresult)
@brief Notify finished parsing segment (Media Source Extensions Draft relevant)
@param in_opaque Callback argument passed to wkcMediaPlayerAddSBPeer()
@param in_appendresult result of append segment@n
WKC_MEDIA_SB_APPENDRESULT_APPEND_SUCCEEDED succeded to parsing segment@n
WKC_MEDIA_SB_APPENDRESULT_READ_STREAM_FAILED don't use@n
WKC_MEDIA_SB_APPENDRESULT_PARSING_FAILED failed to parsing segment@n
@details
Notify parsing segment is finished. It should be call in wkcMediaPlayerAppendToSBPeer().
*/
typedef void (*WKCSBAppendCompleteProc)(void* in_opaque, int in_appendresult);

/**
@typedef void (*WKCSBRemoveCodedFramesProc)(void* in_opaque, double in_start, double in_end)
@brief Remove specified range
@param in_opaque Callback argument passed to wkcMediaPlayerAddSBPeer()
@param in_start start of range removed
@param in_end end of range removed
@details
Remove coded frames in specified range.
 */
typedef void (*WKCSBRemoveCodedFramesProc)(void* in_opaque, double in_start, double in_end);

/** @brief Structure that stores callback function for notifying SourceBuffer  */
struct WKCMediaPlayerSourceBufferCallbacks_ {
    /** @brief Member that holds WKCSBDidReceiveInitializationSegmentProc */
    WKCSBDidReceiveInitializationSegmentProc fDidReceiveInitializationSegment;
    /** @brief Member that holds WKCSBDidReceiveSampleProc */
    WKCSBDidReceiveSampleProc fDidReceiveSample;
    /** @brief Member that holds WKCSBAppendCompleteProc */
    WKCSBAppendCompleteProc fAppendComplete;
    /** @brief Member that holds WKCSBRemoveCodedFramesProc */
    WKCSBRemoveCodedFramesProc fRemoveCodedFrames;
};
/** @brief Type definition of WKCMediaPlayerSourceBufferCallbacks */
typedef struct WKCMediaPlayerSourceBufferCallbacks_ WKCMediaPlayerSourceBufferCallbacks;
/**
@brief Add new SourceBuffer to MediaSource (Media Source Extensions Draft relevant)
@param in_self Media player instance
@param in_type MIMEType of new SourceBuffer
@param out_sourcebuffer Source buffer instance
@retval WKC_MEDIA_ADD_SB_OK Suceeded
@retval WKC_MEDIA_ADD_SB_NOTSUPPORTED Failed, NOT_SUPPORTED_ERR will be thrown
@retval WKC_MEDIA_ADD_SB_REACHEDIDLIMIT Faile, QUOTA_EXCEEDED_ERR will be thrown
@details
Called from MediaSource.addSourceBuffer().@n
The source buffer instance returned via out_sourcebuffer is passed as in_sourcebuffer of individual wkcMediaPlayerXXXXSBPeer().@n
Refer to http://www.w3.org/TR/media-source/#widl-MediaSource-addSourceBuffer-SourceBuffer-DOMString-type
*/
WKC_PEER_API int wkcMediaPlayerAddSBPeer(void* in_self, const char* in_type, void** out_sourcebuffer);
/**
@brief Get duration of MediaSource (Media Source Extensions Draft relevant)
@param in_self Media player instance
@return duration in seconds
@details
Called on getting MediaSource.duration.@n
Refer to http://www.w3.org/TR/media-source/#widl-MediaSource-duration
*/
WKC_PEER_API double wkcMediaPlayerGetMSDurationPeer(void* in_self);
/**
@brief Set duration of MediaSource (Media Source Extensions Draft relevant)
@param in_self Media player instance
@param in_duration duration in seconds
@retval WKC_MEDIA_ERROR_OK Succeeded
@retval WKC_MEDIA_ERROR_GENERIC Failed
@details
Called on setting MediaSource.duration.@n
Refer to http://www.w3.org/TR/media-source/#widl-MediaSource-duration
@attention
Currently, there are no differences in operation by return value.
*/
WKC_PEER_API int wkcMediaPlayerSetMSDurationPeer(void* in_self, double in_duration);
/**
@brief Notify end of stream to media player (Media Source Extensions Draft relevant)
@param in_self Media player instance
@param in_status one of WKC_MEDIA_MS_EOS_NOERROR, WKC_MEDIA_MS_EOS_NETWORKERROR or WKC_MEDIA_MS_EOS_DECODEERROR
@retval WKC_MEDIA_ERROR_OK Succeeded
@retval WKC_MEDIA_ERROR_GENERIC Failed
@details
Call from MediaSource.endOfStream()@n
WKC_MEDIA_MS_EOS_NOERROR is passed via in_status when MediaSource.endOfStream() called with null, an empty string or called without argument.@n
WKC_MEDIA_MS_EOS_NETWORKERROR is passed via in_status when MediaSource.endOfStream() called with string "network"@n
WKC_MEDIA_MS_EOS_DECODEERROR is passed via in_status when MediaSource.endOfStream() called with string "decode"@n
Refer to http://www.w3.org/TR/media-source/#widl-MediaSource-endOfStream-void-EndOfStreamError-error
@attention
Currently, there are no differences in operation by return value.
*/
WKC_PEER_API int wkcMediaPlayerNotifyMSMarkEndOfStreamPeer(void* in_self, int in_status);
/**
@@brief Notify opening source for "ended" state MediaSource to media player (Media Source Extensions Draft relevant)
@param in_self Media player instance
@retval WKC_MEDIA_ERROR_OK Succeeded
@retval WKC_MEDIA_ERROR_GENERIC Failed
@details
Call from SourceBuffer.appendBuffer(), SourceBuffer.remove(), SourceBuffer.setTimestampOffset()@n
Refer to http://www.w3.org/TR/media-source/#widl-SourceBuffer-appendBuffer-void-ArrayBuffer-data, @n
http://www.w3.org/TR/media-source/#widl-SourceBuffer-remove-void-double-start-unrestricted-double-end
@attention
Currently, there are no differences in operation by return value.
*/
WKC_PEER_API int wkcMediaPlayerNotifyMSUnmarkEndOfStreamPeer(void* in_self);
/**
@brief Check MediaSource is Ended
@param in_self Media player instance
@retval != false MediaSource is ended
@retval == false MediaSource is not ended
@details
Called for checking MediaSource ended flag.@n
Refer to https://www.w3.org/TR/media-source/#dom-readystate-ended
*/
WKC_PEER_API bool wkcMediaPlayerMSIsEndedPeer(void* in_self);
WKC_PEER_API int wkcMediaPlayerSetReadyStatePeer(void* in_self, int in_state);
/**
@brief Set Active to SourceBurrer
@param in_self Media player instance
@param in_sourcebuffer Source buffer instance
@param in_active active flag
@retval WKC_MEDIA_ERROR_OK Succeeded
@retval WKC_MEDIA_ERROR_GENERIC Failed
@details
Set Active to SourceBurrer.
@attention
Currently, there are no differences in operation by return value.
*/
WKC_PEER_API int wkcMediaPlayerSetActiveSBPeer(void* in_self, void* in_sourcebuffer, bool in_active);
/**
@brief Append data to SourceBuffer (Media Source Extensions Draft relevant)
@param in_self Media player instance
@param in_sourcebuffer Source buffer instance
@param in_data Pointer to first byte of data
@param in_length Length of data
@param in_callbacks Callback for notification
@param in_opaque Callback argument
@retval WKC_MEDIA_ERROR_OK Succeeded
@retval WKC_MEDIA_ERROR_GENERIC Failed
@details
Called from SourceBuffer.appendBuffer().@n
Refer to http://www.w3.org/TR/media-source/#widl-SourceBuffer-appendBuffer-void-ArrayBuffer-data
Should call WKCSBDidReceiveInitializationSegmentProc on parsing initialization segment@n
Should call WKCSBDidReceiveSampleProc on parsing sample segment@n
Should call WKCSBAppendCompleteProc on finished parsing segment
@attention
Currently, there are no differences in operation by return value.
*/
WKC_PEER_API int wkcMediaPlayerAppendToSBPeer(void* in_self, void* in_sourcebuffer, const unsigned char* in_data, unsigned in_length, const WKCMediaPlayerSourceBufferCallbacks* in_callbacks, void* in_opaque);
/**
@brief Abort parsing fragment (Media Source Extensions Draft relevant)
@param in_self Media player instance
@param in_sourcebuffer Source buffer instance
@retval WKC_MEDIA_ERROR_OK Succeeded
@retval WKC_MEDIA_ERROR_GENERIC Failed
@details
Called from SourceBuffer.abort().@n
Abort parsing the current segment.@n
Refer to http://www.w3.org/TR/media-source/#widl-SourceBuffer-abort-void
@attention
Currently, there are no differences in operation by return value.
*/
WKC_PEER_API int wkcMediaPlayerAbortSBPeer(void* in_self, void* in_sourcebuffer);
/**
@brief Set offset applied to timestamps (Media Source Extensions 0.6 relevant)
@param in_self Media player instance
@param in_sourcebuffer Source buffer instance
@param in_timestamp_offset offset applied to timestamps in seconds
@retval !=false Succeeded
@retval ==false Failed, INVALID_STATE_ERR will be thrown
@details
Called on setting SourceBuffer.timstampOffset.@n
Set offset applied to timestamps.@n
Refer to https://dvcs.w3.org/hg/html-media/raw-file/7bab66368f2c/media-source/media-source.html#dom-abort
@attention
Currently, there are no differences in operation by return value.
*/
WKC_PEER_API bool wkcMediaPlayerSetSBTimestampOffsetPeer(void* in_self, void* in_sourcebuffer, double in_timestamp_offset);
/**
@brief Remove SourceBuffer from MediaSource (Media Source Extensions Draft relevant)
@param in_self Media player instance
@param in_sourcebuffer Source buffer instance
@retval WKC_MEDIA_ERROR_OK Succeeded
@retval WKC_MEDIA_ERROR_GENERIC Failed
@details
Called from MediaSource.removeSourceBuffer() or MediaSource was removed.@n
Discard source buffer instance.@n
Refer to http://www.w3.org/TR/media-source/#widl-MediaSource-removeSourceBuffer-void-SourceBuffer-sourceBuffer
@attention
Currently, there are no differences in operation by return value.
*/
WKC_PEER_API int wkcMediaPlayerRemoveSBPeer(void* in_self, void* in_sourcebuffer);
/**
@brief Remove specified range
@param in_self Media player instance
@param in_sourcebuffer Source buffer instance
@param in_start start of range removed
@param in_end end of range removed
@retval WKC_MEDIA_ERROR_OK Succeeded
@retval WKC_MEDIA_ERROR_GENERIC Failed
@details
Called from SourceBuffer.remove.@n
Remove coded frames in specified range.@n
Refer to https://www.w3.org/TR/media-source/#sourcebuffer-coded-frame-removal
*/
WKC_PEER_API int wkcMediaPlayerRemoveCodedFramesPeer(void* in_self, void* in_sourcebuffer, double in_start, double in_end);
/**
@brief Request to determine "Coded Frame Eviction Algorithm" (Media Source Extensions Draft relevant)
@param in_self Media player instance
@param in_sourcebuffer Source buffer instance
@param out_start start of the eviction range
@param out_end end of the eviction range
@retval WKC_MEDIA_ERROR_OK Succeeded
@retval WKC_MEDIA_ERROR_GENERIC Failed
@details
Run coded frame eviction algorithm.@n
Refer to http://www.w3.org/TR/media-source/#sourcebuffer-coded-frame-eviction
@attention
Currently, there are no differences in operation by return value.
*/
WKC_PEER_API int wkcMediaPlayerEvictCodedFramesSBPeer(void* in_self, void* in_sourcebuffer, double* out_start, double* out_end);
/**
@brief Check SourceBuffer is full (Media Source Extensions Draft relevant)
@param in_self Media player instance
@param in_sourcebuffer Source buffer instance
@retval != false SourceBuffer is full
@retval == false SourceBuffer is not full
@details
Called for checking buffer full flag.@n
Refer to http://www.w3.org/TR/media-source/#sourcebuffer-buffer-full-flag
*/
WKC_PEER_API bool wkcMediaPlayerIsFullSBPeer(void* in_self, void* in_sourcebuffer);
/**
@brief Change the MIME type (Media Source Extensions Draft relevant)
@param in_self Media player instance
@param in_sourcebuffer Source buffer instance
@param in_type MIMEType
@retval != false Supported
@retval == false Unsupported
@details
Change the MIME type of the source buffer.@n
Refer to http://www.w3.org/TR/media-source/#dom-sourcebuffer-changetype
*/
WKC_PEER_API bool wkcCanSwitchToTypePeer(void* in_self, void* in_sourcebuffer, const char* in_type);
/**
@brief Determines whether key system is supported (Encrypted Media Extensions WD relevant)
@param in_keysystem Key system identifier@n
Refer to http://www.w3.org/TR/encrypted-media/#key-system
@param in_mimetype MIME type string or empty string
@retval !=false Supported
@retval ==false Unsupported
@details
Called for checking whether key system is supported.
@attention
Just return in_keysystem is supported if in_mimetype == ""
*/
WKC_PEER_API bool wkcMediaPlayerIsSupportedKeySystemPeer(const char* in_keysystem, const char* in_mimetype);

// media stream source
enum {
    WKC_MEDIA_STREAMSOURCE_TYPE_AUDIO = 0,
    WKC_MEDIA_STREAMSOURCE_TYPE_VIDEO,
    WKC_MEDIA_STREAMSOURCE_TYPES
};

enum {
    WKC_MEDIA_STREAMSOURCE_READYSTATE_LIVE = 0,
    WKC_MEDIA_STREAMSOURCE_READYSTATE_MUTED,
    WKC_MEDIA_STREAMSOURCE_READYSTATE_ENDED,
    WKC_MEDIA_STREAMSOURCE_READYSTATES,
};

/** @brief Structure that stores media stream source information */
struct WKCMediaStreamSourceInfo_ {
    /** @brief Member that holds identification */
    char fID[256];
    /** @brief Member that holds name */
    char fName[256];
    /** @brief Member that holds type */
    int fType;
    /** @brief Member that holds ready state */
    int fReadyState;
    /** @brief Member that holds whether to require consumer */
    bool fRequiresConsumer;
};
/** @brief Type definition of WKCMediaStreamSourceInfo */
typedef struct WKCMediaStreamSourceInfo_ WKCMediaStreamSourceInfo;

/**
@brief Gets number of media stream sources
@return Number of media stream sources
@details
Called from navigator.webkitGetUserMedia().@n
The number of media stream sources must be returned as a return value.
*/
WKC_PEER_API int wkcMediaPlayerQueryMediaStreamSourcesPeer(void);
/**
@brief Gets media stream source infomation
@param in_index Index
@param out_info Media stream source infomation
@retval WKC_MEDIA_ERROR_OK Succeeded
@retval WKC_MEDIA_ERROR_GENERIC Failed
@retval WKC_MEDIA_ERROR_NOTSUPPORTED Unsupported
@retval WKC_MEDIA_ERROR_NOTREADY Preparation not complete
@details
Set the media stream source infomation in out_info.@n
The value corresponding to the value returned by wkcMediaPlayerQueryMediaStreamSourcesPeer() is passed to in_index.
*/
WKC_PEER_API int wkcMediaPlayerQueryMediaStreamSourcePeer(int in_index, WKCMediaStreamSourceInfo* out_info);

/** @brief Structure that stores ice candidate information */
struct WKCMediaStreamIceCandidateInfo_ {
    /** @brief Member that holds MLine index */
    unsigned int fMLineIndex;
    /** @brief Member that holds foundation */
    unsigned int fFoundation;
    /** @brief Member that holds component */
    unsigned int fComponent;
    /** @brief Member that holds is udp or not */
    bool fUDP;
    /** @brief Member that holds priority */
    unsigned int fPriority;
    /** @brief Member that holds host address */
    char fAddress[256];
    /** @brief Member that holds port number */
    int fPort;
    /** @brief Member that holds connection type */
    enum {
        EHOST,
        ESRFLX,
        ERELAY,
    };
    int fType;
};
/** @brief Type definition of WKCMediaStreamSourceInfo */
typedef struct WKCMediaStreamIceCandidateInfo_ WKCMediaStreamIceCandidateInfo;

/**
@brief Gets number of media ice candidates
@return Number of media ice candidates
*/
WKC_PEER_API int wkcMediaPlayerQueryMediaIceCandidatesPeer(void);

/*
@brief Gets media ice candidate infomation
@param in_index Index
@param out_info Media ice candidate infomation
@retval WKC_MEDIA_ERROR_OK Succeeded
@retval WKC_MEDIA_ERROR_GENERIC Failed
@retval WKC_MEDIA_ERROR_NOTSUPPORTED Unsupported
@retval WKC_MEDIA_ERROR_NOTREADY Preparation not complete
@details
Set the media ice candidate infomation in out_info.@n
The value corresponding to the value returned by wkcMediaPlayerQueryMediaIceCandidatesPeer() is passed to in_index.
*/
WKC_PEER_API int wkcMediaPlayerQueryMediaIceCandidatePeer(int in_index, WKCMediaStreamIceCandidateInfo* out_info);

/*
@brief Gets media ice candidate infomation
@retval local SDP string
*/
WKC_PEER_API const char* wkcMediaPlayerQueryMediaLocalSDPPeer(void);

/*
@brief Adds remote media ice candidate information
@param in_candidate Candidate
@param in_sdpmid SDPMid
*/
WKC_PEER_API void wkcMediaPlayerAddMediaIceCandidatePeer(const char* in_candidate, const char* in_sdpmid);

/**
@brief Set verbose mode of CURL
@param in_self Media player instance
@param in_verbose Whether verbose mode is enabled or not
*/
WKC_PEER_API void wkcMediaPlayerSetCurlVerbosePeer(void* in_self, bool in_verbose);

WKC_END_C_LINKAGE

/*@}*/

#endif /* _WKC_MEDIA_PEER_H_ */
