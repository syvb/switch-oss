/*
 * WKCWebVersion.h
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
 * You should have received a copy of the GNU Library General Public License
 * along with this library; see the file COPYING.LIB.  If not, write to
 * the Free Software Foundation, Inc., 51 Franklin Street, Fifth Floor,
 * Boston, MA 02110-1301, USA.
 */

#ifndef WKCVersion_h
#define WKCVersion_h

#define WKC_VERSION_MAJOR (6)
#define WKC_VERSION_MINOR (0)
#define WKC_VERSION_MICRO (0)

#define WKC_VERSION_CHECK(major, minor, micro) \
    (((major)*10000) + ((minor)*100) + (micro)) >= ((WKC_VERSION_MAJOR*10000) + (WKC_VERSION_MINOR*100) + (WKC_VERSION_MICRO))

#define WKC_CUSTOMER_RELEASE_VERSION "3.27.11"

#define WKC_WEBKIT_VERSION "613.0"

#endif // WKCVersion_h
