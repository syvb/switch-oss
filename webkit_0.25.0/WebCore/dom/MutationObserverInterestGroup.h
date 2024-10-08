/*
 * Copyright (C) 2011 Google Inc. All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions are
 * met:
 *
 *     * Redistributions of source code must retain the above copyright
 * notice, this list of conditions and the following disclaimer.
 *     * Redistributions in binary form must reproduce the above
 * copyright notice, this list of conditions and the following disclaimer
 * in the documentation and/or other materials provided with the
 * distribution.
 *     * Neither the name of Google Inc. nor the names of its
 * contributors may be used to endorse or promote products derived from
 * this software without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS
 * "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT
 * LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR
 * A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT
 * OWNER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL,
 * SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT
 * LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE,
 * DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY
 * THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
 * (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
 * OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */

#ifndef MutationObserverInterestGroup_h
#define MutationObserverInterestGroup_h

#include "Document.h"
#include "MutationObserver.h"
#include "QualifiedName.h"
#include <memory>
#include <wtf/HashMap.h>

namespace WebCore {

class MutationObserverInterestGroup {
#if PLATFORM(WKC)
    WTF_MAKE_FAST_ALLOCATED;
#endif
public:
    MutationObserverInterestGroup(HashMap<MutationObserver*, MutationRecordDeliveryOptions>& observers, MutationRecordDeliveryOptions oldValueFlag);

    static std::unique_ptr<MutationObserverInterestGroup> createForChildListMutation(Node& target)
    {
        if (!target.document().hasMutationObserversOfType(MutationObserver::ChildList))
            return nullptr;

        MutationRecordDeliveryOptions oldValueFlag = 0;
        return createIfNeeded(target, MutationObserver::ChildList, oldValueFlag);
    }

    static std::unique_ptr<MutationObserverInterestGroup> createForCharacterDataMutation(Node& target)
    {
        if (!target.document().hasMutationObserversOfType(MutationObserver::CharacterData))
            return nullptr;

        return createIfNeeded(target, MutationObserver::CharacterData, MutationObserver::CharacterDataOldValue);
    }

    static std::unique_ptr<MutationObserverInterestGroup> createForAttributesMutation(Node& target, const QualifiedName& attributeName)
    {
        if (!target.document().hasMutationObserversOfType(MutationObserver::Attributes))
            return nullptr;

        return createIfNeeded(target, MutationObserver::Attributes, MutationObserver::AttributeOldValue, &attributeName);
    }

    bool isOldValueRequested();
    void enqueueMutationRecord(PassRefPtr<MutationRecord>);

private:
    static std::unique_ptr<MutationObserverInterestGroup> createIfNeeded(Node& target, MutationObserver::MutationType, MutationRecordDeliveryOptions oldValueFlag, const QualifiedName* attributeName = nullptr);

    bool hasOldValue(MutationRecordDeliveryOptions options) { return options & m_oldValueFlag; }

    HashMap<MutationObserver*, MutationRecordDeliveryOptions> m_observers;
    MutationRecordDeliveryOptions m_oldValueFlag;
};

}

#endif // MutationObserverInterestGroup_h
