/*
 * Copyright (C) 2011 Google Inc. All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions are
 * met:
 *
 * 1. Redistributions of source code must retain the above copyright
 * notice, this list of conditions and the following disclaimer.
 *
 * 2. Redistributions in binary form must reproduce the above
 * copyright notice, this list of conditions and the following disclaimer
 * in the documentation and/or other materials provided with the
 * distribution.
 *
 * THIS SOFTWARE IS PROVIDED BY GOOGLE INC. AND ITS CONTRIBUTORS
 * "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT
 * LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR
 * A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL GOOGLE INC.
 * OR ITS CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL,
 * SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT
 * LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE,
 * DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY
 * THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
 * (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
 * OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */

// This is based on code from Chromium: WebKit/chromium/src/PageOverlay(List).cpp

#include "config.h"

#include "WKCOverlayIf.h"
#include "WKCOverlayPrivate.h"
#include "WKCWebViewPrivate.h"

#include "Chrome.h"
#include "ChromeClient.h"
#include "GraphicsContext.h"
#include "Page.h"

# include "helpers/privates/WKCGraphicsLayerPrivate.h"
namespace WebCore {
WKC::GraphicsLayerPrivate* GraphicsLayerWKC_wkc(const WebCore::GraphicsLayer* layer);
}

namespace WKC {

class WKCOverlayGraphicsLayerClient : public WebCore::GraphicsLayerClient, public RefCounted<WKCOverlayGraphicsLayerClient> {
public:
    static WTF::Ref<WKCOverlayGraphicsLayerClient> create(WKCWebViewPrivate* view, WKCOverlayIf* overlay)
    {
        return WTF::adoptRef(*new WKCOverlayGraphicsLayerClient(view, overlay));
    }

    virtual ~WKCOverlayGraphicsLayerClient() { }

    virtual void notifyFlushRequired(const WebCore::GraphicsLayer*) override
    {
        WebCore::Page* page = m_view->core();
        if (page)
            page->chrome().client().triggerRenderingUpdate();
    }

    virtual void paintContents(const WebCore::GraphicsLayer*, WebCore::GraphicsContext& ctx, const WebCore::FloatRect& /* inClip */, WebCore::GraphicsLayerPaintBehavior) override
    {
        ctx.save();
        m_overlay->paintOverlay(ctx);
        ctx.restore();
    }

    virtual float deviceScaleFactor() const override {
        if (m_view->core())
            return m_view->core()->deviceScaleFactor();
        return 1;
    }

    virtual float pageScaleFactor() const override {
        if (m_view->core())
            return m_view->core()->pageScaleFactor();
        return 1;
    }

private:
    WKCOverlayGraphicsLayerClient(WKCWebViewPrivate* view, WKCOverlayIf* overlay)
        : m_view(view)
        , m_overlay(overlay)
    {
    }

    WKCWebViewPrivate* m_view;
    WKCOverlayIf* m_overlay;
};

// WKCOverlayInternal Implementation

Ref<WKCOverlayInternal> WKCOverlayInternal::create(WKCWebViewPrivate* view, WKCOverlayIf* overlay)
{
    return WTF::adoptRef(*new WKCOverlayInternal(view, overlay));
}

WKCOverlayInternal::WKCOverlayInternal(WKCWebViewPrivate* view, WKCOverlayIf* overlay)
    : m_view(view)
    , m_overlay(overlay)
    , m_layerClient(nullptr)
    , m_layer(nullptr)
    , m_zOrder(0)
    , m_fixedDirectionFlag(EFixedDirectionNone)
    , m_isUpdating(false)
{
}

WKCOverlayInternal::~WKCOverlayInternal()
{
}

void
WKCOverlayInternal::clear()
{
    if (!m_view->rootGraphicsLayer()) {
        invalidateAll();
        return;
    }

    if (m_layer) {
        m_layer->removeFromParent();
        m_layer = nullptr;
        m_layerClient = nullptr;
    }
}

// TODO: Now we always update a whole rect. Can we invalidate a smaller rect?
void
WKCOverlayInternal::update(const WebCore::IntRect&, bool immediate)
{
    if (m_isUpdating)
        return;

    m_isUpdating = true;

    if (!m_view->rootGraphicsLayer()) {
        invalidateAll();
        m_isUpdating = false;
        return;
    }

    if (!m_layer) {
        m_layerClient = WKCOverlayGraphicsLayerClient::create(m_view, m_overlay);
        m_layer = WebCore::GraphicsLayer::create(m_view->core()->chrome().client().graphicsLayerFactory(), *m_layerClient);
        m_layer->setName("WKCWebView overlay content");
        m_layer->setDrawsContent(true);
        m_view->rootGraphicsLayer()->addChild(*m_layer);
    }

    GraphicsLayerWKC_wkc(m_layer.get())->setIsFixedPositioned(m_fixedDirectionFlag != EFixedDirectionNone);
    GraphicsLayerWKC_wkc(m_layer.get())->setPropertyLeftType(m_fixedDirectionFlag & EFixedDirectionLeft ? LengthType::Fixed : LengthType::Undefined);
    GraphicsLayerWKC_wkc(m_layer.get())->setPropertyRightType(m_fixedDirectionFlag & EFixedDirectionRight ? LengthType::Fixed : LengthType::Undefined);
    GraphicsLayerWKC_wkc(m_layer.get())->setPropertyTopType(m_fixedDirectionFlag & EFixedDirectionTop ? LengthType::Fixed : LengthType::Undefined);
    GraphicsLayerWKC_wkc(m_layer.get())->setPropertyBottomType(m_fixedDirectionFlag & EFixedDirectionBottom ? LengthType::Fixed : LengthType::Undefined);
    m_layer->setSize(WebCore::FloatSize(m_view->desktopSize()));

    if (immediate)
        m_layer->setNeedsDisplay();
    else
        GraphicsLayerWKC_wkc(m_layer.get())->addDirtyRect();

    m_isUpdating = false;
}

void
WKCOverlayInternal::invalidateAll()
{
    WebCore::Frame& frame = m_view->core()->mainFrame();
    if (frame.view()) {
        WebCore::IntRect rect = frame.view()->frameRect();
        if (!rect.isEmpty())
            frame.view()->invalidateRect(rect);
    }
}

void
WKCOverlayInternal::paintOffscreen(WebCore::GraphicsContext& ctx)
{
    ctx.save();
    m_overlay->paintOverlay(ctx);
    ctx.restore();
}


// WKCOverlayList Implementation

Ref<WKCOverlayList> WKCOverlayList::create(WKCWebViewPrivate* view)
{
    return WTF::adoptRef(*new WKCOverlayList(view));
}

WKCOverlayList::WKCOverlayList(WKCWebViewPrivate* view)
    : m_view(view)
    , m_isUpdating(false)
{
}

WKCOverlayList::~WKCOverlayList()
{
}

bool
WKCOverlayList::add(WKCOverlayIf* overlay, int zOrder, int fixedDirectionFlag)
{
    bool added = false;
    size_t index = find(overlay);
    if (index == WTF::notFound) {
        RefPtr<WKCOverlayInternal> ol = WKCOverlayInternal::create(m_view, overlay);
        m_list.append(ol);
        index = m_list.size() - 1;
        added = true;
    }

    WKCOverlayInternal* ol = m_list[index].get();
    ol->setZOrder(zOrder);
    ol->setFixedDirection(fixedDirectionFlag);

    // TODO: If we have more than one overlay, sort overlay list according to zOrder and update all the overlays.
    ol->update(WebCore::IntRect(), true);

    return added;
}

bool
WKCOverlayList::remove(WKCOverlayIf* overlay)
{
    size_t index = find(overlay);
    if (index == WTF::notFound)
        return false;

    m_list[index]->clear();
    m_list.remove(index);
    return true;
}

size_t
WKCOverlayList::find(WKCOverlayIf* overlay)
{
    for (size_t i = 0; i < m_list.size(); ++i) {
        if (m_list[i]->overlay() == overlay)
            return i;
    }
    return WTF::notFound;
}

void
WKCOverlayList::update(const WebCore::IntRect& rect, bool immediate)
{
    if (m_isUpdating)
        return;
    m_isUpdating = true;

    for (size_t i = 0; i < m_list.size(); ++i)
        m_list[i]->update(rect, immediate);

    m_isUpdating = false;
}

void
WKCOverlayList::paintOffscreen(WebCore::GraphicsContext& ctx)
{
    if (m_view->rootGraphicsLayer())
        return;

    for (size_t i = 0; i < m_list.size(); ++i)
        m_list[i]->paintOffscreen(ctx);
}

} // namespace
