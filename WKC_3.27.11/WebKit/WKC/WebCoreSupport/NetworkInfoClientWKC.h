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

#ifndef NetworkInfoClientWKC_h
#define NetworkInfoClientWKC_h

#include "NetworkInfoClient.h"

#if ENABLE(NETWORK_INFO)

namespace WKC {

class WKCWebViewPrivate;

class NetworkInfoClientWKC : public WebCore::NetworkInfoClient {
public:
    static NetworkInfoClientWKC* create(WKCWebViewPrivate*);
    virtual ~NetworkInfoClientWKC();

    virtual double bandwidth() const;
    virtual bool metered() const;
    virtual void startUpdating();
    virtual void stopUpdating();
    virtual void networkInfoControllerDestroyed();

private:
    NetworkInfoClientWKC(WKCWebViewPrivate*);
    bool construct();

private:
    WKCWebViewPrivate* m_view;
};

} // namespace

#endif // ENABLE(NETWORK_INFO)

#endif // NetworkInfoClientWKC_h
