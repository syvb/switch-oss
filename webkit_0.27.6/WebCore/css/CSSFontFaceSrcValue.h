/*
 * Copyright (C) 2007, 2008 Apple Inc. All rights reserved.
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

#ifndef CSSFontFaceSrcValue_h
#define CSSFontFaceSrcValue_h

#include "CSSValue.h"
#include "CachedResourceHandle.h"
#include <wtf/PassRefPtr.h>
#include <wtf/text/WTFString.h>

namespace WebCore {

class CachedFont;
class Document;
class SVGFontFaceElement;

class CSSFontFaceSrcValue : public CSSValue {
public:
    static Ref<CSSFontFaceSrcValue> create(const String& resource)
    {
        return adoptRef(*new CSSFontFaceSrcValue(resource, false));
    }
    static Ref<CSSFontFaceSrcValue> createLocal(const String& resource)
    {
        return adoptRef(*new CSSFontFaceSrcValue(resource, true));
    }

    const String& resource() const { return m_resource; }
    const String& format() const { return m_format; }
    bool isLocal() const { return m_isLocal; }

    void setFormat(const String& format) { m_format = format; }

    bool isSupportedFormat() const;

#if ENABLE(SVG_FONTS)
    bool isSVGFontFaceSrc() const;
    bool isSVGFontTarget() const;

    SVGFontFaceElement* svgFontFaceElement() const { return m_svgFontFaceElement; }
    void setSVGFontFaceElement(SVGFontFaceElement* element) { m_svgFontFaceElement = element; }
#endif

    String customCSSText() const;

    void addSubresourceStyleURLs(ListHashSet<URL>&, const StyleSheetContents*) const;

#if PLATFORM(WKC)
    bool traverseSubresources(const WTF::Function<bool (const CachedResource&)>& handler) const;
#else
    bool traverseSubresources(const std::function<bool (const CachedResource&)>& handler) const;
#endif

    CachedFont* cachedFont(Document*, bool isSVG, bool isInitiatingElementInUserAgentShadowTree);

    bool equals(const CSSFontFaceSrcValue&) const;

private:
    CSSFontFaceSrcValue(const String& resource, bool local)
        : CSSValue(FontFaceSrcClass)
        , m_resource(resource)
        , m_isLocal(local)
#if ENABLE(SVG_FONTS)
        , m_svgFontFaceElement(0)
#endif
    {
    }

    String m_resource;
    String m_format;
    bool m_isLocal;

    CachedResourceHandle<CachedFont> m_cachedFont;

#if ENABLE(SVG_FONTS)
    SVGFontFaceElement* m_svgFontFaceElement;
#endif
};

} // namespace WebCore

SPECIALIZE_TYPE_TRAITS_CSS_VALUE(CSSFontFaceSrcValue, isFontFaceSrcValue())

#endif
