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

#ifndef _WKC_HELPERS_PRIVATE_DEVICEORIENTATIONCONTROLLER_H_
#define _WKC_HELPERS_PRIVATE_DEVICEORIENTATIONCONTROLLER_H_

#include "helpers/WKCDeviceOrientationController.h"

namespace WebCore {
class DeviceOrientationController;
} // namespace

namespace WKC {

class DeviceOrientationControllerWrap : public DeviceOrientationController {
WTF_MAKE_FAST_ALLOCATED;
public:
    DeviceOrientationControllerWrap(DeviceOrientationControllerPrivate& parent)
        : DeviceOrientationController(parent) {}
    ~DeviceOrientationControllerWrap() {}
};

class DeviceOrientationControllerPrivate {
WTF_MAKE_FAST_ALLOCATED;
public:
    DeviceOrientationControllerPrivate(WebCore::DeviceOrientationController*);
    ~DeviceOrientationControllerPrivate();

    WebCore::DeviceOrientationController* webcore() const { return m_webcore; }
    DeviceOrientationController& wkc() { return m_wkc; }

    void didChangeDeviceOrientation(DeviceOrientation*);

private:
    WebCore::DeviceOrientationController* m_webcore;
    DeviceOrientationControllerWrap m_wkc;

    DeviceOrientation* m_orientation;
};
} // namespace

#endif // _WKC_HELPERS_PRIVATE_DEVICEORIENTATIONCONTROLLER_H_

