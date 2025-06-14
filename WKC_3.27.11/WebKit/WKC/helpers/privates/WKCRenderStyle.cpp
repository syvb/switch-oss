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
 * You should have received a copy of the GNU Library General Public
 * License along with this library; if not, write to the
 * Free Software Foundation, Inc., 51 Franklin St, Fifth Floor,
 * Boston, MA  02110-1301, USA.
 */

#include "config.h"

#include "helpers/WKCRenderStyle.h"
#include "helpers/WKCColor.h"
#include "helpers/privates/WKCRenderStylePrivate.h"
#include "helpers/privates/WKCBorderDataPrivate.h"

#include "RenderStyle.h"

namespace WKC {

RenderStylePrivate::RenderStylePrivate(WebCore::RenderStyle* parent)
    : m_webcore(parent)
    , m_wkc(*this)
    , m_borderData(nullptr)
{
}

RenderStylePrivate::~RenderStylePrivate()
{
    delete m_borderData;
}

#if ENABLE(TOUCH_EVENTS)
Color
RenderStylePrivate::tapHighlightColor() const
{
    WebCore::Color color = m_webcore->tapHighlightColor();
    return Color((ColorPrivate*)&color);
}
#endif

bool
RenderStylePrivate::outlineStyleIsAuto() const
{
    auto isAuto = m_webcore->outlineStyleIsAuto();
    return (isAuto == WebCore::OutlineIsAuto::On);
}

const BorderData&
RenderStylePrivate::border()
{
    const WebCore::BorderData& borderData = webcore()->border();

    delete m_borderData;
    m_borderData = new BorderDataPrivate(&borderData);

    return m_borderData->wkc();
}

Color
RenderStylePrivate::backgroundColor() const
{
    WebCore::Color color = m_webcore->backgroundColor();
    return Color((ColorPrivate*)&color);
}

////////////////////////////////////////////////////////////////////////////////

RenderStyle::RenderStyle(RenderStylePrivate& parent)
    : m_private(parent)
{
}

RenderStyle::~RenderStyle()
{
}

Color
RenderStyle::tapHighlightColor() const
{
#if ENABLE(TOUCH_EVENTS)
    return m_private.tapHighlightColor();
#else
    return Color();
#endif
}

bool
RenderStyle::outlineStyleIsAuto() const
{
    return m_private.outlineStyleIsAuto();
}

const BorderData&
RenderStyle::border() const
{
    return m_private.border();
}

Color
RenderStyle::backgroundColor() const
{
    return m_private.backgroundColor();
}

} // namespace
