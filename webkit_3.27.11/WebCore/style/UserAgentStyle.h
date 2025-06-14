/*
 * Copyright (C) 1999 Lars Knoll (knoll@kde.org)
 * Copyright (C) 2003, 2004, 2005, 2006, 2007, 2008, 2009, 2010, 2011 Apple Inc. All rights reserved.
 * Copyright (C) 2013 Google Inc. All rights reserved.
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
 *
 */

#pragma once

namespace WebCore {

class Element;
class StyleSheetContents;

namespace Style {

class RuleSet;

class UserAgentStyle {
public:
#if !PLATFORM(WKC)
    static RuleSet* defaultStyle;
    static RuleSet* defaultQuirksStyle;
    static RuleSet* defaultPrintStyle;
    static unsigned defaultStyleVersion;

    static StyleSheetContents* defaultStyleSheet;
    static StyleSheetContents* quirksStyleSheet;
    static StyleSheetContents* dialogStyleSheet;
    static StyleSheetContents* svgStyleSheet;
    static StyleSheetContents* mathMLStyleSheet;
    static StyleSheetContents* mediaControlsStyleSheet;
    static StyleSheetContents* fullscreenStyleSheet;
    static StyleSheetContents* plugInsStyleSheet;
#if ENABLE(SERVICE_CONTROLS)
    static StyleSheetContents* imageControlsStyleSheet;
#endif
#if ENABLE(DATALIST_ELEMENT)
    static StyleSheetContents* dataListStyleSheet;
#endif
#if ENABLE(INPUT_TYPE_COLOR)
    static StyleSheetContents* colorInputStyleSheet;
#endif
#if ENABLE(IOS_FORM_CONTROL_REFRESH)
    static StyleSheetContents* legacyFormControlsIOSStyleSheet;
#endif
#if ENABLE(ALTERNATE_FORM_CONTROL_DESIGN)
    static StyleSheetContents* alternateFormControlDesignStyleSheet;
#endif

    static StyleSheetContents* mediaQueryStyleSheet;
#else
    WKC_DEFINE_GLOBAL_CLASS_OBJ_ENTRY_ZERO(RuleSet*, defaultStyle);
    WKC_DEFINE_GLOBAL_CLASS_OBJ_ENTRY_ZERO(RuleSet*, defaultQuirksStyle);
    WKC_DEFINE_GLOBAL_CLASS_OBJ_ENTRY_ZERO(RuleSet*, defaultPrintStyle);
    WKC_DEFINE_GLOBAL_CLASS_OBJ_ENTRY_ZERO(unsigned, defaultStyleVersion);

    WKC_DEFINE_GLOBAL_CLASS_OBJ_ENTRY_ZERO(StyleSheetContents*, simpleDefaultStyleSheet);
    WKC_DEFINE_GLOBAL_CLASS_OBJ_ENTRY_ZERO(StyleSheetContents*, defaultStyleSheet);
    WKC_DEFINE_GLOBAL_CLASS_OBJ_ENTRY_ZERO(StyleSheetContents*, quirksStyleSheet);
    WKC_DEFINE_GLOBAL_CLASS_OBJ_ENTRY_ZERO(StyleSheetContents*, dialogStyleSheet);
    WKC_DEFINE_GLOBAL_CLASS_OBJ_ENTRY_ZERO(StyleSheetContents*, svgStyleSheet);
    WKC_DEFINE_GLOBAL_CLASS_OBJ_ENTRY_ZERO(StyleSheetContents*, mathMLStyleSheet);
    WKC_DEFINE_GLOBAL_CLASS_OBJ_ENTRY_ZERO(StyleSheetContents*, mediaControlsStyleSheet);
    WKC_DEFINE_GLOBAL_CLASS_OBJ_ENTRY_ZERO(StyleSheetContents*, fullscreenStyleSheet);
    WKC_DEFINE_GLOBAL_CLASS_OBJ_ENTRY_ZERO(StyleSheetContents*, plugInsStyleSheet);
    WKC_DEFINE_GLOBAL_CLASS_OBJ_ENTRY_ZERO(StyleSheetContents*, imageControlsStyleSheet);
#if ENABLE(DATALIST_ELEMENT)
    WKC_DEFINE_GLOBAL_CLASS_OBJ_ENTRY_ZERO(StyleSheetContents*, dataListStyleSheet);
#endif
#if ENABLE(INPUT_TYPE_COLOR)
    WKC_DEFINE_GLOBAL_CLASS_OBJ_ENTRY_ZERO(StyleSheetContents*, colorInputStyleSheet);
#endif

    WKC_DEFINE_GLOBAL_CLASS_OBJ_ENTRY_ZERO(StyleSheetContents*, mediaQueryStyleSheet);
#endif

    static void initDefaultStyleSheet();
    static void ensureDefaultStyleSheetsForElement(const Element&);

private:
    static void addToDefaultStyle(StyleSheetContents&);
};

} // namespace Style
} // namespace WebCore
