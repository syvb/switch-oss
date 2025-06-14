/*
 * Copyright (C) 2015 Apple Inc. All Rights Reserved.
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

#ifndef MediaSession_h
#define MediaSession_h

#if ENABLE(MEDIA_SESSION)

#include "MediaRemoteControls.h"
#include "MediaSessionMetadata.h"

namespace WebCore {

class Dictionary;
class Document;
class HTMLMediaElement;

class MediaSession final : public RefCounted<MediaSession> {
public:
    enum class State {
        Idle,
        Active,
        Interrupted
    };

    static Ref<MediaSession> create(ScriptExecutionContext& context, const String& kind)
    {
        return adoptRef(*new MediaSession(context, kind));
    }

    explicit MediaSession(Document&);
    MediaSession(ScriptExecutionContext&, const String&);
    ~MediaSession();

    String kind() const;
    MediaRemoteControls* controls(bool& isNull);

    State currentState() const { return m_currentState; }
    bool hasActiveMediaElements() const;

    void setMetadata(const Dictionary&);

    void releaseSession();
    
    // Runs the media session invocation algorithm and returns true on success.
    bool invoke();

    void togglePlayback();
    void skipToNextTrack();
    void skipToPreviousTrack();

private:
    friend class HTMLMediaElement;

    enum class Kind {
        Default,
        Content,
        Transient,
        TransientSolo,
        Ambient
    };

    static Kind parseKind(const String&);
    Kind kindEnum() const { return m_kind; }

    void addMediaElement(HTMLMediaElement&);
    void removeMediaElement(HTMLMediaElement&);

#if PLATFORM(WKC)
    void changeActiveMediaElements(WTF::Function<void(void)>);
#else
    void changeActiveMediaElements(std::function<void(void)>);
#endif
    void addActiveMediaElement(HTMLMediaElement&);
    bool isMediaElementActive(HTMLMediaElement&);

    void releaseInternal();

    State m_currentState { State::Idle };
    HashSet<HTMLMediaElement*> m_participatingElements;
    HashSet<HTMLMediaElement*> m_activeParticipatingElements;
    HashSet<HTMLMediaElement*>* m_iteratedActiveParticipatingElements { nullptr };

    Document& m_document;
    const Kind m_kind { Kind::Default };
    RefPtr<MediaRemoteControls> m_controls;
    MediaSessionMetadata m_metadata;
};

} // namespace WebCore

#endif /* ENABLE(MEDIA_SESSION) */

#endif /* MediaSession_h */
