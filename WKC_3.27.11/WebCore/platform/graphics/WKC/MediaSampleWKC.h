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

#pragma once

#include "MediaSample.h"

namespace WebCore {

class MediaSampleWKC : public MediaSample {
public:
    static Ref<MediaSampleWKC> create(int id, int64_t presentationTime, int64_t decodeTime, int64_t duration, size_t size, bool isSync) { return adoptRef(*new MediaSampleWKC(id, presentationTime, decodeTime, duration, size, isSync)); }
    virtual ~MediaSampleWKC() { }

    virtual MediaTime presentationTime() const override final { return m_presentationTime; }
    virtual MediaTime decodeTime() const override final { return m_decodeTime; }
    virtual MediaTime duration() const override final { return m_duration; }

    virtual AtomString trackID() const override final { return m_trackID; }
    virtual void setTrackID(const String& id) override final { m_trackID = id; }

    virtual size_t sizeInBytes() const override final { return m_size; }
    virtual FloatSize presentationSize() const override final { return { 0, 0 }; }

    virtual void offsetTimestampsBy(const MediaTime& timestampOffset) override final
    {
        m_presentationTime += timestampOffset;
        m_decodeTime += timestampOffset;
    }
    virtual void setTimestamps(const MediaTime& presentationTimestamp, const MediaTime& decodeTimestamp) override final
    {
        m_presentationTime = presentationTimestamp;
        m_decodeTime = decodeTimestamp;
    }
    virtual bool isDivisable() const override final { return false; }

    virtual std::pair<RefPtr<MediaSample>, RefPtr<MediaSample>> divide(const MediaTime& presentationTime, UseEndTime = UseEndTime::DoNotUse) override final { return { nullptr, nullptr }; }
    virtual Ref<MediaSample> createNonDisplayingCopy() const override final
    {
        auto copy = MediaSampleWKC::create(std::atoi(m_trackID.string().utf8().data()), m_presentationTime.toDouble(), m_decodeTime.toDouble(), m_duration.toDouble(), m_size, isSync());
        return WTFMove(copy);
    }

    virtual SampleFlags flags() const override final { return m_flags; }
    virtual PlatformSample platformSample() override final { PlatformSample sample = { PlatformSample::None, }; return sample; }

    virtual std::optional<ByteRange> byteRange() const override final { return std::nullopt; }

    virtual void dump(PrintStream&) const override final { }

    virtual void didDrop() override final { m_isDropped = true; }
    bool isDropped() const { return m_isDropped; }

protected:
    MediaSampleWKC(int id, int64_t presentationTime, int64_t decodeTime, int64_t duration, size_t size, bool isSync)
        : m_presentationTime(presentationTime, 1000000)
        , m_decodeTime(decodeTime, 1000000)
        , m_duration(duration, 1000000)
        , m_trackID(makeString(id))
        , m_size(size)
        , m_flags(isSync ? IsSync : None)
        , m_isDropped(false)
    {
    }

private:
    MediaTime m_presentationTime;
    MediaTime m_decodeTime;
    MediaTime m_duration;
    AtomString m_trackID;
    size_t m_size;
    SampleFlags m_flags;
    bool m_isDropped;
};

}
