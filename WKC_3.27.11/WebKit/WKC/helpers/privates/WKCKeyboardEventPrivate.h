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

#ifndef _WKC_HELPERS_PRIVATE_KEYBOARDEVENT_H_
#define _WKC_HELPERS_PRIVATE_KEYBOARDEVENT_H_

#include <wkc/wkcbase.h>

#include "helpers/WKCKeyboardEvent.h"

#include "helpers/privates/WKCEventPrivate.h"

#include "WKCPlatformEvents.h"

namespace WebCore {
class KeyboardEvent;
} // namespace

namespace WKC {

class KeyboardEvent;

class KeyboardEventWrap : public KeyboardEvent {
WTF_MAKE_FAST_ALLOCATED;
public:
    KeyboardEventWrap(KeyboardEventPrivate& parent) : KeyboardEvent(parent) {}
    ~KeyboardEventWrap() {}
};

class KeyboardEventPrivate : public EventPrivate {
WTF_MAKE_FAST_ALLOCATED;
public:
    KeyboardEventPrivate(WebCore::KeyboardEvent*);
    virtual ~KeyboardEventPrivate();

    KeyboardEvent& wkc() { return m_wkc; }

    WKCKeyEvent keyEvent() const { return m_keyEvent; }

private:
    KeyboardEventWrap m_wkc;

    WKCKeyEvent m_keyEvent;
};

} // namespace

#endif // _WKC_HELPERS_PRIVATE_KEYBOARDEVENT_H_
