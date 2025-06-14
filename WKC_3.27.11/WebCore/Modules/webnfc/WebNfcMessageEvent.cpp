/*
 *  WebNfcMessageEvent.cpp
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
 * You should have received a copy of the GNU Library General Public
 * License along with this library; if not, write to the
 * Free Software Foundation, Inc., 51 Franklin St, Fifth Floor,
 * Boston, MA  02110-1301, USA.
 */

#include "config.h"
#include "WebNfcMessageEvent.h"

#if ENABLE(WKC_WEB_NFC)

#include "WebNfcMessage.h"

namespace WebCore {

WebNfcMessageEvent::WebNfcMessageEvent()
    : m_message(0)
{
}

WebNfcMessageEvent::WebNfcMessageEvent(const AtomString& eventType, RefPtr<WebNfcMessage> message)
    : Event(eventType, false, false) // Can't bubble, not cancelable
    , m_message(message)
{
}

WebNfcMessageEvent::~WebNfcMessageEvent()
{
}

Ref<WebNfcMessageEvent> WebNfcMessageEvent::create()
{
    return adoptRef(*new WebNfcMessageEvent());
}

Ref<WebNfcMessageEvent> WebNfcMessageEvent::create(const AtomString& eventType, RefPtr<WebNfcMessage> message)
{
    return adoptRef(*new WebNfcMessageEvent(eventType, message));
}

} // namspace WebCore

#endif // ENABLE(WKC_WEB_NFC)
