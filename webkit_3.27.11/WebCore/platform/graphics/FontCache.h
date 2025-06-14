/*
 * Copyright (C) 2006-2021 Apple Inc. All rights reserved.
 * Copyright (C) 2007-2008 Torch Mobile, Inc.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 *
 * 1.  Redistributions of source code must retain the above copyright
 *     notice, this list of conditions and the following disclaimer.
 * 2.  Redistributions in binary form must reproduce the above copyright
 *     notice, this list of conditions and the following disclaimer in the
 *     documentation and/or other materials provided with the distribution.
 * 3.  Neither the name of Apple Inc. ("Apple") nor the names of
 *     its contributors may be used to endorse or promote products derived
 *     from this software without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY APPLE AND ITS CONTRIBUTORS "AS IS" AND ANY
 * EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED
 * WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE
 * DISCLAIMED. IN NO EVENT SHALL APPLE OR ITS CONTRIBUTORS BE LIABLE FOR ANY
 * DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES
 * (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES;
 * LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND
 * ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
 * (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF
 * THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */

#pragma once

#include "FontCascadeFonts.h"
#include "FontCreationContext.h"
#include "FontDescription.h"
#include "FontPlatformData.h"
#include "FontSelector.h"
#include "FontTaggedSettings.h"
#include "Timer.h"
#include <array>
#include <limits.h>
#include <wtf/FastMalloc.h>
#include <wtf/Forward.h>
#include <wtf/HashFunctions.h>
#include <wtf/HashTraits.h>
#include <wtf/ListHashSet.h>
#include <wtf/PointerComparison.h>
#include <wtf/RefPtr.h>
#include <wtf/UniqueRef.h>
#include <wtf/Vector.h>
#include <wtf/WorkQueue.h>
#include <wtf/text/AtomStringHash.h>
#include <wtf/text/WTFString.h>

#if PLATFORM(COCOA)
#include "FontCacheCoreText.h"
#endif

#if PLATFORM(IOS_FAMILY)
#include <wtf/Lock.h>
#include <wtf/RecursiveLockAdapter.h>
#endif

#if OS(WINDOWS)
#include <windows.h>
#include <objidl.h>
#include <mlang.h>
#endif

namespace WebCore {

class Font;
class FontCascade;
class OpenTypeVerticalData;

enum class IsForPlatformFont : bool;

#if PLATFORM(WIN) && USE(IMLANG_FONT_LINK2)
using IMLangFontLinkType = IMLangFontLink2;
#endif

#if PLATFORM(WIN) && !USE(IMLANG_FONT_LINK2)
using IMLangFontLinkType = IMLangFontLink;
#endif

struct FontDescriptionKeyRareData : public RefCounted<FontDescriptionKeyRareData> {
    WTF_MAKE_FAST_ALLOCATED;
public:
    static Ref<FontDescriptionKeyRareData> create(FontFeatureSettings&& featureSettings, FontVariationSettings&& variationSettings, FontPalette&& fontPalette)
    {
        return adoptRef(*new FontDescriptionKeyRareData(WTFMove(featureSettings), WTFMove(variationSettings), WTFMove(fontPalette)));
    }

    const FontFeatureSettings& featureSettings() const
    {
        return m_featureSettings;
    }

    const FontVariationSettings& variationSettings() const
    {
        return m_variationSettings;
    }

    const FontPalette& fontPalette() const
    {
        return m_fontPalette;
    }

    bool operator==(const FontDescriptionKeyRareData& other) const
    {
        return m_featureSettings == other.m_featureSettings
            && m_variationSettings == other.m_variationSettings
            && m_fontPalette == other.m_fontPalette;
    }

private:
    FontDescriptionKeyRareData(FontFeatureSettings&& featureSettings, FontVariationSettings&& variationSettings, FontPalette&& fontPalette)
        : m_featureSettings(WTFMove(featureSettings))
        , m_variationSettings(WTFMove(variationSettings))
        , m_fontPalette(WTFMove(fontPalette))
    {
    }

    FontFeatureSettings m_featureSettings;
    FontVariationSettings m_variationSettings;
    FontPalette m_fontPalette;
};

inline void add(Hasher& hasher, const FontDescriptionKeyRareData& key)
{
    add(hasher, key.featureSettings(), key.variationSettings(), key.fontPalette());
}

// This key contains the FontDescription fields other than family that matter when fetching FontDatas (platform fonts).
struct FontDescriptionKey {
    FontDescriptionKey() = default;

    FontDescriptionKey(const FontDescription& description)
        : m_size(description.computedPixelSize())
        , m_fontSelectionRequest(description.fontSelectionRequest())
        , m_flags(makeFlagsKey(description))
        , m_locale(description.specifiedLocale())
    {
        auto featureSettings = description.featureSettings();
        auto variationSettings = description.variationSettings();
        auto fontPalette = description.fontPalette();
        if (!featureSettings.isEmpty() || !variationSettings.isEmpty() || fontPalette.type != FontPalette::Type::Normal)
            m_rareData = FontDescriptionKeyRareData::create(WTFMove(featureSettings), WTFMove(variationSettings), WTFMove(fontPalette));
    }

    explicit FontDescriptionKey(WTF::HashTableDeletedValueType)
        : m_isDeletedValue(true)
    { }

    bool operator==(const FontDescriptionKey& other) const
    {
        return m_isDeletedValue == other.m_isDeletedValue
            && m_size == other.m_size
            && m_fontSelectionRequest == other.m_fontSelectionRequest
            && m_flags == other.m_flags
            && m_locale == other.m_locale
            && arePointingToEqualData(m_rareData, other.m_rareData);
    }

    bool operator!=(const FontDescriptionKey& other) const
    {
        return !(*this == other);
    }

    bool isHashTableDeletedValue() const { return m_isDeletedValue; }

    friend void add(Hasher&, const FontDescriptionKey&);

private:
    static std::array<unsigned, 2> makeFlagsKey(const FontDescription& description)
    {
        unsigned first = static_cast<unsigned>(description.script()) << 15
            | static_cast<unsigned>(description.shouldDisableLigaturesForSpacing()) << 14
            | static_cast<unsigned>(description.shouldAllowUserInstalledFonts()) << 13
            | static_cast<unsigned>(description.fontStyleAxis() == FontStyleAxis::slnt) << 12
            | static_cast<unsigned>(description.opticalSizing()) << 11
            | static_cast<unsigned>(description.textRenderingMode()) << 9
            | static_cast<unsigned>(description.fontSynthesis()) << 6
            | static_cast<unsigned>(description.widthVariant()) << 4
            | static_cast<unsigned>(description.nonCJKGlyphOrientation()) << 3
            | static_cast<unsigned>(description.orientation()) << 2
            | static_cast<unsigned>(description.renderingMode());
        unsigned second = static_cast<unsigned>(description.variantEastAsianRuby()) << 27
            | static_cast<unsigned>(description.variantEastAsianWidth()) << 25
            | static_cast<unsigned>(description.variantEastAsianVariant()) << 22
            | static_cast<unsigned>(description.variantAlternates()) << 21
            | static_cast<unsigned>(description.variantNumericSlashedZero()) << 20
            | static_cast<unsigned>(description.variantNumericOrdinal()) << 19
            | static_cast<unsigned>(description.variantNumericFraction()) << 17
            | static_cast<unsigned>(description.variantNumericSpacing()) << 15
            | static_cast<unsigned>(description.variantNumericFigure()) << 13
            | static_cast<unsigned>(description.variantCaps()) << 10
            | static_cast<unsigned>(description.variantPosition()) << 8
            | static_cast<unsigned>(description.variantContextualAlternates()) << 6
            | static_cast<unsigned>(description.variantHistoricalLigatures()) << 4
            | static_cast<unsigned>(description.variantDiscretionaryLigatures()) << 2
            | static_cast<unsigned>(description.variantCommonLigatures());
        return {{ first, second }};
    }

    bool m_isDeletedValue { false };
    unsigned m_size { 0 };
    FontSelectionRequest m_fontSelectionRequest;
    std::array<unsigned, 2> m_flags {{ 0, 0 }};
    AtomString m_locale;
    RefPtr<FontDescriptionKeyRareData> m_rareData;
};

inline void add(Hasher& hasher, const FontDescriptionKey& key)
{
    add(hasher, key.m_size, key.m_fontSelectionRequest, key.m_flags, key.m_locale);
    if (key.m_rareData)
        add(hasher, *key.m_rareData);
}

} // namespace WebCore

namespace WTF {

template<> struct DefaultHash<WebCore::FontDescriptionKey> {
    static unsigned hash(const WebCore::FontDescriptionKey& key) { return computeHash(key); }
    static bool equal(const WebCore::FontDescriptionKey& a, const WebCore::FontDescriptionKey& b) { return a == b; }
    static constexpr bool safeToCompareToEmptyOrDeleted = true;
};

template<> struct HashTraits<WebCore::FontDescriptionKey> : SimpleClassHashTraits<WebCore::FontDescriptionKey> {
};

}

namespace WebCore {

// This class holds the name of a font family, and defines hashing and == of this name to
// use the rules for font family names instead of using straight string comparison.
class FontFamilyName {
public:
    FontFamilyName();
    FontFamilyName(const AtomString&);
    const AtomString& string() const;
    friend void add(Hasher&, const FontFamilyName&);

private:
    AtomString m_name;
};

bool operator==(const FontFamilyName&, const FontFamilyName&);
bool operator!=(const FontFamilyName&, const FontFamilyName&);

struct FontCascadeCacheKey {
    FontDescriptionKey fontDescriptionKey; // Shared with the lower level FontCache (caching Font objects)
    Vector<FontFamilyName, 3> families;
    unsigned fontSelectorId;
    unsigned fontSelectorVersion;
};

bool operator==(const FontCascadeCacheKey&, const FontCascadeCacheKey&);

struct FontCascadeCacheEntry {
    WTF_MAKE_STRUCT_FAST_ALLOCATED;

    FontCascadeCacheKey key;
    Ref<FontCascadeFonts> fonts;
};

struct FontCascadeCacheKeyHash {
    static unsigned hash(const FontCascadeCacheKey&);
    static bool equal(const FontCascadeCacheKey& a, const FontCascadeCacheKey& b) { return a == b; }
    static constexpr bool safeToCompareToEmptyOrDeleted = false;
};

struct FontCascadeCacheKeyHashTraits : HashTraits<FontCascadeCacheKey> {
    static FontCascadeCacheKey emptyValue() { return { }; }
    static void constructDeletedValue(FontCascadeCacheKey& slot) { new (NotNull, &slot.fontDescriptionKey) FontDescriptionKey(WTF::HashTableDeletedValue); }
    static bool isDeletedValue(const FontCascadeCacheKey& key) { return key.fontDescriptionKey.isHashTableDeletedValue(); }
};

using FontCascadeCache = HashMap<FontCascadeCacheKey, std::unique_ptr<FontCascadeCacheEntry>, FontCascadeCacheKeyHash, FontCascadeCacheKeyHashTraits>;

class FontCache {
    WTF_MAKE_NONCOPYABLE(FontCache); WTF_MAKE_FAST_ALLOCATED;
public:
    WEBCORE_EXPORT static FontCache& forCurrentThread();
    static FontCache* forCurrentThreadIfNotDestroyed();

    FontCache();
    ~FontCache();

    // These methods are implemented by the platform.
    enum class PreferColoredFont : bool { No, Yes };
    RefPtr<Font> systemFallbackForCharacters(const FontDescription&, const Font* originalFontData, IsForPlatformFont, PreferColoredFont, const UChar* characters, unsigned length);
    Vector<String> systemFontFamilies();
    void platformInit();

    static bool isSystemFontForbiddenForEditing(const String&);

#if PLATFORM(COCOA)
    WEBCORE_EXPORT static void setFontAllowlist(const Vector<String>&);
#endif

#if PLATFORM(WIN)
    IMLangFontLinkType* getFontLinkInterface();
    static void comInitialize();
    static void comUninitialize();
    static IMultiLanguage* getMultiLanguageInterface();
#endif

    // This function exists so CSSFontSelector can have a unified notion of preinstalled fonts and @font-face.
    // It comes into play when you create an @font-face which shares a family name as a preinstalled font.
    Vector<FontSelectionCapabilities> getFontSelectionCapabilitiesInFamily(const AtomString&, AllowUserInstalledFonts);

    WEBCORE_EXPORT RefPtr<Font> fontForFamily(const FontDescription&, const String&, const FontCreationContext& = { }, bool checkingAlternateName = false);
    WEBCORE_EXPORT Ref<Font> lastResortFallbackFont(const FontDescription&);
    WEBCORE_EXPORT Ref<Font> fontForPlatformData(const FontPlatformData&);
    RefPtr<Font> similarFont(const FontDescription&, const String& family);

    void addClient(FontSelector&);
    void removeClient(FontSelector&);

    unsigned short generation() const { return m_generation; }
    WEBCORE_EXPORT void invalidate();
    static void registerFontCacheInvalidationCallback(Function<void()>&&);
    WEBCORE_EXPORT static void invalidateAllFontCaches();

    WEBCORE_EXPORT size_t fontCount();
    WEBCORE_EXPORT size_t inactiveFontCount();
    WEBCORE_EXPORT void purgeInactiveFontData(unsigned count = UINT_MAX);
    void platformPurgeInactiveFontData();

    void updateFontCascade(const FontCascade&, RefPtr<FontSelector>&&);
    void invalidateFontCascadeCache();
    void clearWidthCaches();

#if PLATFORM(WKC)
    void clearGlyphPages();
#endif

#if PLATFORM(WIN)
    RefPtr<Font> fontFromDescriptionAndLogFont(const FontDescription&, const LOGFONT&, String& outFontFamilyName);
#endif

#if ENABLE(OPENTYPE_VERTICAL)
    RefPtr<OpenTypeVerticalData> verticalData(const FontPlatformData&);
#endif

    std::unique_ptr<FontPlatformData> createFontPlatformDataForTesting(const FontDescription&, const AtomString& family);
    
    bool shouldMockBoldSystemFontForAccessibility() const { return m_shouldMockBoldSystemFontForAccessibility; }
    void setShouldMockBoldSystemFontForAccessibility(bool shouldMockBoldSystemFontForAccessibility) { m_shouldMockBoldSystemFontForAccessibility = shouldMockBoldSystemFontForAccessibility; }

    struct PrewarmInformation {
        Vector<String> seenFamilies;
        Vector<String> fontNamesRequiringSystemFallback;

        bool isEmpty() const;
        PrewarmInformation isolatedCopy() const;

        template<class Encoder> void encode(Encoder&) const;
        template<class Decoder> static std::optional<PrewarmInformation> decode(Decoder&);
    };
    PrewarmInformation collectPrewarmInformation() const;
    void prewarm(const PrewarmInformation&);
    static void prewarmGlobally();

private:
    WEBCORE_EXPORT void purgeInactiveFontDataIfNeeded();
    void pruneUnreferencedEntriesFromFontCascadeCache();
    void pruneSystemFallbackFonts();
    Ref<FontCascadeFonts> retrieveOrAddCachedFonts(const FontCascadeDescription&, RefPtr<FontSelector>&&);

    FontPlatformData* cachedFontPlatformData(const FontDescription&, const String& family, const FontCreationContext& = { }, bool checkingAlternateName = false);

    // These functions are implemented by each platform (unclear which functions this comment applies to).
    WEBCORE_EXPORT std::unique_ptr<FontPlatformData> createFontPlatformData(const FontDescription&, const AtomString& family, const FontCreationContext&);
    
    static std::optional<ASCIILiteral> alternateFamilyName(const String&);
    static std::optional<ASCIILiteral> platformAlternateFamilyName(const String&);

#if PLATFORM(MAC)
    bool shouldAutoActivateFontIfNeeded(const AtomString& family);
#endif

    Timer m_purgeTimer;
    
    bool m_shouldMockBoldSystemFontForAccessibility { false };

    HashSet<FontSelector*> m_clients;
    struct FontDataCaches;
    UniqueRef<FontDataCaches> m_fontDataCaches;
    FontCascadeCache m_fontCascadeCache;

    unsigned short m_generation { 0 };

#if PLATFORM(IOS_FAMILY)
    RecursiveLock m_fontLock;
#endif

#if PLATFORM(MAC)
    HashSet<AtomString> m_knownFamilies;
#endif

#if PLATFORM(COCOA)
    ListHashSet<String> m_seenFamiliesForPrewarming;
    ListHashSet<String> m_fontNamesRequiringSystemFallbackForPrewarming;
    RefPtr<WorkQueue> m_prewarmQueue;

    friend class ComplexTextController;
#endif

    friend class Font;
};

inline std::unique_ptr<FontPlatformData> FontCache::createFontPlatformDataForTesting(const FontDescription& fontDescription, const AtomString& family)
{
    return createFontPlatformData(fontDescription, family, { });
}

#if !PLATFORM(COCOA) && !USE(FREETYPE) && !PLATFORM(WKC)

inline void FontCache::platformPurgeInactiveFontData()
{
}

#endif

inline bool FontCache::PrewarmInformation::isEmpty() const
{
    return seenFamilies.isEmpty() && fontNamesRequiringSystemFallback.isEmpty();
}

inline FontCache::PrewarmInformation FontCache::PrewarmInformation::isolatedCopy() const
{
    return { seenFamilies.isolatedCopy(), fontNamesRequiringSystemFallback.isolatedCopy() };
}

template<class Encoder>
void FontCache::PrewarmInformation::encode(Encoder& encoder) const
{
    encoder << seenFamilies;
    encoder << fontNamesRequiringSystemFallback;
}

template<class Decoder>
std::optional<FontCache::PrewarmInformation> FontCache::PrewarmInformation::decode(Decoder& decoder)
{
    PrewarmInformation prewarmInformation;
    if (!decoder.decode(prewarmInformation.seenFamilies))
        return { };
    if (!decoder.decode(prewarmInformation.fontNamesRequiringSystemFallback))
        return { };

    return prewarmInformation;
}

}
