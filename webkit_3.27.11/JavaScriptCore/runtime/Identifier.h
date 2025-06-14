/*
 *  Copyright (C) 2003-2019 Apple Inc. All rights reserved.
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
 *  You should have received a copy of the GNU Library General Public License
 *  along with this library; see the file COPYING.LIB.  If not, write to
 *  the Free Software Foundation, Inc., 51 Franklin Street, Fifth Floor,
 *  Boston, MA 02110-1301, USA.
 *
 */

#pragma once

#include "ArrayConventions.h"
#include "PrivateName.h"
#include "VM.h"
#include <wtf/text/CString.h>
#include <wtf/text/UniquedStringImpl.h>
#include <wtf/text/WTFString.h>

namespace JSC {

class CallFrame;

ALWAYS_INLINE bool isIndex(uint32_t index)
{
    return index <= MAX_ARRAY_INDEX;
}

template <typename CharType>
ALWAYS_INLINE std::optional<uint32_t> parseIndex(const CharType* characters, unsigned length)
{
    // An empty string is not a number.
    if (!length)
        return std::nullopt;

    // Get the first character, turning it into a digit.
    uint32_t value = characters[0] - '0';
    if (value > 9)
        return std::nullopt;

    // Check for leading zeros. If the first characher is 0, then the
    // length of the string must be one - e.g. "042" is not equal to "42".
    if (!value && length > 1)
        return std::nullopt;

    while (--length) {
        // Multiply value by 10, checking for overflow out of 32 bits.
        if (value > 0xFFFFFFFFU / 10)
            return std::nullopt;
        value *= 10;

        // Get the next character, turning it into a digit.
        uint32_t newValue = *(++characters) - '0';
        if (newValue > 9)
            return std::nullopt;

        // Add in the old value, checking for overflow out of 32 bits.
        newValue += value;
        if (newValue < value)
            return std::nullopt;
        value = newValue;
    }

    if (!isIndex(value))
        return std::nullopt;
    return value;
}

ALWAYS_INLINE std::optional<uint32_t> parseIndex(StringImpl& impl)
{
    if (impl.is8Bit())
        return parseIndex(impl.characters8(), impl.length());
    return parseIndex(impl.characters16(), impl.length());
}

class Identifier {
    friend class Structure;
public:
    Identifier() { }
    enum EmptyIdentifierFlag { EmptyIdentifier };
    Identifier(EmptyIdentifierFlag) : m_string(StringImpl::empty()) { ASSERT(m_string.impl()->isAtom()); }

    const String& string() const { return m_string; }
    UniquedStringImpl* impl() const { return static_cast<UniquedStringImpl*>(m_string.impl()); }

    int length() const { return m_string.length(); }

    CString ascii() const { return m_string.ascii(); }
    CString utf8() const { return m_string.utf8(); }

    // There's 2 functions to construct Identifier from string, (1) fromString and (2) fromUid.
    // They have different meanings in keeping or discarding symbol-ness of strings.
    // (1): fromString
    // Just construct Identifier from string. String held by Identifier is always atomized.
    // Symbol-ness of StringImpl*, which represents that the string is inteded to be used for ES6 Symbols, is discarded.
    // So a constructed Identifier never represents a symbol.
    // (2): fromUid
    // `StringImpl* uid` represents ether String or Symbol property.
    // fromUid keeps symbol-ness of provided StringImpl* while fromString discards it.
    // Use fromUid when constructing Identifier from StringImpl* which may represent symbols.

    // Only to be used with string literals.
    template<unsigned charactersCount>
    static Identifier fromString(VM&, const char (&characters)[charactersCount]);
    static Identifier fromString(VM&, ASCIILiteral);
    static Identifier fromString(VM&, const LChar*, int length);
    static Identifier fromString(VM&, const UChar*, int length);
    static Identifier fromString(VM&, const String&);
    static Identifier fromString(VM&, AtomStringImpl*);
    static Identifier fromString(VM&, Ref<AtomStringImpl>&&);
    static Identifier fromString(VM&, const AtomString&);
    static Identifier fromString(VM& vm, SymbolImpl*);
    static Identifier fromString(VM&, const char*);
    static Identifier fromString(VM& vm, const Vector<LChar>& characters) { return fromString(vm, characters.data(), characters.size()); }

    static Identifier fromUid(VM&, UniquedStringImpl* uid);
    static Identifier fromUid(const PrivateName&);
    static Identifier fromUid(SymbolImpl&);

    static Identifier createLCharFromUChar(VM& vm, const UChar* s, int length) { return Identifier(vm, add8(vm, s, length)); }

    JS_EXPORT_PRIVATE static Identifier from(VM&, unsigned y);
    JS_EXPORT_PRIVATE static Identifier from(VM&, int y);
    JS_EXPORT_PRIVATE static Identifier from(VM&, double y);
    ALWAYS_INLINE static Identifier from(VM& vm, uint64_t y)
    {
        if (static_cast<uint32_t>(y) == y)
            return from(vm, static_cast<uint32_t>(y));
        ASSERT(static_cast<uint64_t>(static_cast<double>(y)) == y);
        return from(vm, static_cast<double>(y));
    }

    bool isNull() const { return m_string.isNull(); }
    bool isEmpty() const { return m_string.isEmpty(); }
    bool isSymbol() const { return !isNull() && impl()->isSymbol(); }
    bool isPrivateName() const { return isSymbol() && static_cast<const SymbolImpl*>(impl())->isPrivate(); }

    friend bool operator==(const Identifier&, const Identifier&);
    friend bool operator!=(const Identifier&, const Identifier&);

    friend bool operator==(const Identifier&, const LChar*);
    friend bool operator==(const Identifier&, const char*);
    friend bool operator!=(const Identifier&, const LChar*);
    friend bool operator!=(const Identifier&, const char*);

    static bool equal(const StringImpl*, const LChar*);
    static inline bool equal(const StringImpl*a, const char*b) { return Identifier::equal(a, reinterpret_cast<const LChar*>(b)); };
    static bool equal(const StringImpl*, const LChar*, unsigned length);
    static bool equal(const StringImpl*, const UChar*, unsigned length);
    static bool equal(const StringImpl* a, const StringImpl* b) { return ::equal(a, b); }

    // Only to be used with string literals.
    JS_EXPORT_PRIVATE static Ref<AtomStringImpl> add(VM&, const char*);

    void dump(PrintStream&) const;

private:
    String m_string;

    // Only to be used with string literals.
#if !PLATFORM(WKC)
    template<unsigned charactersCount>
    Identifier(VM& vm, const char (&characters)[charactersCount]) : m_string(add(vm, characters)) { ASSERT(m_string.impl()->isAtom()); }

    Identifier(VM& vm, const LChar* s, int length) : m_string(add(vm, s, length)) { ASSERT(m_string.impl()->isAtom()); }
    Identifier(VM& vm, const UChar* s, int length) : m_string(add(vm, s, length)) { ASSERT(m_string.impl()->isAtom()); }
    Identifier(VM&, AtomStringImpl*);
    Identifier(VM&, const AtomString&);
    Identifier(VM& vm, const String& string) : m_string(add(vm, string.impl())) { ASSERT(m_string.impl()->isAtom()); }
    Identifier(VM& vm, StringImpl* rep) : m_string(add(vm, rep)) { ASSERT(m_string.impl()->isAtom()); }
#else
    template<unsigned charactersCount>
    Identifier(VM& vm, const char(&characters)[charactersCount]) : m_string(add(vm, characters)) { }

    Identifier(VM& vm, const LChar* s, int length) : m_string(add(vm, s, length)) { }
    Identifier(VM& vm, const UChar* s, int length) : m_string(add(vm, s, length)) { }
    Identifier(VM&, AtomStringImpl*);
    Identifier(VM&, const AtomString&);
    Identifier(VM& vm, const String& string) : m_string(add(vm, string.impl())) { }
    Identifier(VM& vm, StringImpl* rep) : m_string(add(vm, rep)) { }
#endif

    Identifier(VM&, Ref<AtomStringImpl>&& impl)
        : m_string(WTFMove(impl))
    { }

    Identifier(SymbolImpl& uid)
        : m_string(&uid)
    { }

    template <typename CharType>
    ALWAYS_INLINE static uint32_t toUInt32FromCharacters(const CharType* characters, unsigned length, bool& ok);

    static bool equal(const Identifier& a, const Identifier& b) { return a.m_string.impl() == b.m_string.impl(); }
    static bool equal(const Identifier& a, const LChar* b) { return equal(a.m_string.impl(), b); }

    template <typename T> static Ref<AtomStringImpl> add(VM&, const T*, int length);
    static Ref<AtomStringImpl> add8(VM&, const UChar*, int length);
    template <typename T> ALWAYS_INLINE static constexpr bool canUseSingleCharacterString(T);

    static Ref<AtomStringImpl> add(VM&, StringImpl*);

#ifndef NDEBUG
    JS_EXPORT_PRIVATE static void checkCurrentAtomStringTable(VM&);
#else
    JS_EXPORT_PRIVATE NO_RETURN_DUE_TO_CRASH static void checkCurrentAtomStringTable(VM&);
#endif
};

template <> ALWAYS_INLINE constexpr bool Identifier::canUseSingleCharacterString(LChar)
{
    static_assert(maxSingleCharacterString == 0xff);
    return true;
}

template <> ALWAYS_INLINE constexpr bool Identifier::canUseSingleCharacterString(UChar c)
{
    return (c <= maxSingleCharacterString);
}

template <typename T>
Ref<AtomStringImpl> Identifier::add(VM& vm, const T* s, int length)
{
    if (length == 1) {
        T c = s[0];
        if (canUseSingleCharacterString(c))
            return vm.smallStrings.singleCharacterStringRep(c);
    }
    if (!length)
        return *static_cast<AtomStringImpl*>(StringImpl::empty());

    return *AtomStringImpl::add(s, length);
}

inline bool operator==(const Identifier& a, const Identifier& b)
{
    return Identifier::equal(a, b);
}

inline bool operator!=(const Identifier& a, const Identifier& b)
{
    return !Identifier::equal(a, b);
}

inline bool operator==(const Identifier& a, const LChar* b)
{
    return Identifier::equal(a, b);
}

inline bool operator==(const Identifier& a, const char* b)
{
    return Identifier::equal(a, reinterpret_cast<const LChar*>(b));
}

inline bool operator!=(const Identifier& a, const LChar* b)
{
    return !Identifier::equal(a, b);
}

inline bool operator!=(const Identifier& a, const char* b)
{
    return !Identifier::equal(a, reinterpret_cast<const LChar*>(b));
}

inline bool Identifier::equal(const StringImpl* r, const LChar* s)
{
    return WTF::equal(r, s);
}

inline bool Identifier::equal(const StringImpl* r, const LChar* s, unsigned length)
{
    return WTF::equal(r, s, length);
}

inline bool Identifier::equal(const StringImpl* r, const UChar* s, unsigned length)
{
    return WTF::equal(r, s, length);
}

ALWAYS_INLINE std::optional<uint32_t> parseIndex(const Identifier& identifier)
{
    auto uid = identifier.impl();
    if (!uid)
        return std::nullopt;
    if (uid->isSymbol())
        return std::nullopt;
    return parseIndex(*uid);
}

JSValue identifierToJSValue(VM&, const Identifier&);
// This will stringify private symbols. When leaking JSValues to
// non-internal code, make sure to use this function and not the above one.
JSValue identifierToSafePublicJSValue(VM&, const Identifier&);

// FIXME: It may be better for this to just be a typedef for PtrHash, since PtrHash may be cheaper to
// compute than loading the StringImpl's hash from memory. That change would also reduce the likelihood of
// crashes in code that somehow dangled a StringImpl.
// https://bugs.webkit.org/show_bug.cgi?id=150137
struct IdentifierRepHash : PtrHash<RefPtr<UniquedStringImpl>> {
    static unsigned hash(const RefPtr<UniquedStringImpl>& key) { return key->existingSymbolAwareHash(); }
    static unsigned hash(UniquedStringImpl* key) { return key->existingSymbolAwareHash(); }
    static constexpr bool hasHashInValue = true;
};

struct IdentifierMapIndexHashTraits : HashTraits<int> {
    static int emptyValue() { return std::numeric_limits<int>::max(); }
    static constexpr bool emptyValueIsZero = false;
};

typedef HashSet<RefPtr<UniquedStringImpl>, IdentifierRepHash> IdentifierSet;
typedef HashMap<RefPtr<UniquedStringImpl>, int, IdentifierRepHash, HashTraits<RefPtr<UniquedStringImpl>>, IdentifierMapIndexHashTraits> IdentifierMap;
typedef HashMap<UniquedStringImpl*, int, IdentifierRepHash, HashTraits<UniquedStringImpl*>, IdentifierMapIndexHashTraits> BorrowedIdentifierMap;

} // namespace JSC

namespace WTF {

template <> struct VectorTraits<JSC::Identifier> : SimpleClassVectorTraits { };

} // namespace WTF
