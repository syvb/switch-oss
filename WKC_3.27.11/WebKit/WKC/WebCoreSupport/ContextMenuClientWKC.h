/*
 * Copyright (C) 2006 Zack Rusin <zack@kde.org>
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
 * THIS SOFTWARE IS PROVIDED BY APPLE COMPUTER, INC. ``AS IS'' AND ANY
 * EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR
 * PURPOSE ARE DISCLAIMED.  IN NO EVENT SHALL APPLE COMPUTER, INC. OR
 * CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL,
 * EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO,
 * PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR
 * PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY
 * OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
 * (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
 * OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */

#ifndef ContextMenuClientWKC_h
#define ContextMenuClientWKC_h

#include "ContextMenuClient.h"

namespace WKC {
class ContextMenuClientIf;
class WKCWebViewPrivate;

class ContextMenuClientWKC : public WebCore::ContextMenuClient
{
WTF_MAKE_FAST_ALLOCATED;
public:
    static ContextMenuClientWKC* create(WKCWebViewPrivate* view);
    ~ContextMenuClientWKC();

    virtual void contextMenuDestroyed();

    virtual WebCore::PlatformMenuDescription getCustomMenuFromDefaultItems(WebCore::ContextMenu*);
    virtual void contextMenuItemSelected(WebCore::ContextMenuItem*, const WebCore::ContextMenu*);

    virtual void downloadURL(const WebCore::URL& url);
    virtual void searchWithGoogle(const WebCore::Frame*);
    virtual void lookUpInDictionary(WebCore::Frame*);
    virtual void speak(const WTF::String&);
    virtual void stopSpeaking();
    virtual bool isSpeaking();

    virtual WebCore::ContextMenuItem shareMenuItem(const WebCore::HitTestResult&);

private:
    ContextMenuClientWKC(WKCWebViewPrivate* view);
    bool construct();

private:
    WKCWebViewPrivate* m_view;
    WKC::ContextMenuClientIf* m_appClient;
};

} // namespace

#endif // ContextMenuClientWKC_h
