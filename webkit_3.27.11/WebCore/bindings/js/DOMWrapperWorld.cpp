/*
 *  Copyright (C) 1999-2001 Harri Porten (porten@kde.org)
 *  Copyright (C) 2004, 2005, 2006, 2007, 2008, 2009 Apple Inc. All rights reserved.
 *  Copyright (C) 2007 Samuel Weinig <sam@webkit.org>
 *
 *  This library is free software; you can redistribute it and/or
 *  modify it under the terms of the GNU Lesser General Public
 *  License as published by the Free Software Foundation; either
 *  version 2 of the License, or (at your option) any later version.
 *
 *  This library is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 *  Lesser General Public License for more details.
 *
 *  You should have received a copy of the GNU Lesser General Public
 *  License along with this library; if not, write to the Free Software
 *  Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301  USA
 */

#include "config.h"
#include "DOMWrapperWorld.h"

#include "CommonVM.h"
#include "JSDOMWindow.h"
#include "WebCoreJSClientData.h"
#include "WindowProxy.h"
#include <wtf/MainThread.h>


namespace WebCore {
using namespace JSC;

DOMWrapperWorld::DOMWrapperWorld(JSC::VM& vm, Type type, const String& name)
    : m_vm(vm)
    , m_name(name)
    , m_type(type)
{
    VM::ClientData* clientData = m_vm.clientData;
    ASSERT(clientData);
    static_cast<JSVMClientData*>(clientData)->rememberWorld(*this);
}

DOMWrapperWorld::~DOMWrapperWorld()
{
    VM::ClientData* clientData = m_vm.clientData;
    ASSERT(clientData);
    static_cast<JSVMClientData*>(clientData)->forgetWorld(*this);

    // These items are created lazily.
    while (!m_jsWindowProxies.isEmpty())
        (*m_jsWindowProxies.begin())->destroyJSWindowProxy(*this);
}

void DOMWrapperWorld::clearWrappers()
{
    m_wrappers.clear();

    // These items are created lazily.
    while (!m_jsWindowProxies.isEmpty())
        (*m_jsWindowProxies.begin())->destroyJSWindowProxy(*this);
}

DOMWrapperWorld& normalWorld(JSC::VM& vm)
{
    VM::ClientData* clientData = vm.clientData;
    ASSERT(clientData);
    return static_cast<JSVMClientData*>(clientData)->normalWorld();
}

DOMWrapperWorld& mainThreadNormalWorld()
{
    ASSERT(isMainThread());
#if !PLATFORM(WKC)
    static DOMWrapperWorld& cachedNormalWorld = normalWorld(commonVM());
    return cachedNormalWorld;
#else
    WKC_DEFINE_STATIC_TYPE(DOMWrapperWorld*, cachedNormalWorld, 0);
    if (!cachedNormalWorld)
        cachedNormalWorld = &normalWorld(commonVM());
    return *cachedNormalWorld;
#endif
}

} // namespace WebCore
