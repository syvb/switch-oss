/*
 * Copyright (C) 2003, 2006, 2009, 2016 Apple Inc. All rights reserved.
 * Copyright (C) 2006 Rob Buis <buis@kde.org>
 * Copyright (C) 2007-2008 Torch Mobile, Inc.
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

#ifndef Path_h
#define Path_h

#include "FloatRect.h"
#include "WindRule.h"
#include <wtf/FastMalloc.h>
#include <wtf/Function.h>
#include <wtf/Forward.h>

#if USE(CG)

#include <wtf/RetainPtr.h>
#include <CoreGraphics/CGPath.h>
typedef struct CGPath PlatformPath;

#elif USE(DIRECT2D)
#include "COMPtr.h"

interface ID2D1Geometry;
interface ID2D1GeometryGroup;
interface ID2D1PathGeometry;
interface ID2D1GeometrySink;

typedef ID2D1GeometryGroup PlatformPath;

namespace WebCore {
class PlatformContextDirect2D;
}

#elif USE(CAIRO) || USE(WKC_CAIRO)

namespace WebCore {
class CairoPath;
}
typedef WebCore::CairoPath PlatformPath;

#elif USE(WINGDI)

namespace WebCore {
class PlatformPath;
}
typedef WebCore::PlatformPath PlatformPath;

#else

typedef void PlatformPath;

#endif

typedef PlatformPath* PlatformPathPtr;

namespace WTF {
class TextStream;
}

namespace WebCore {

class AffineTransform;
class FloatPoint;
class FloatRoundedRect;
class FloatSize;
class GraphicsContext;
class PathTraversalState;
class RoundedRect;
class StrokeStyleApplier;

// The points in the structure are the same as those that would be used with the
// add... method. For example, a line returns the endpoint, while a cubic returns
// two tangent points and the endpoint.
struct PathElement {
    enum class Type : uint8_t {
        MoveToPoint, // The points member will contain 1 value.
        AddLineToPoint, // The points member will contain 1 value.
        AddQuadCurveToPoint, // The points member will contain 2 values.
        AddCurveToPoint, // The points member will contain 3 values.
        CloseSubpath // The points member will contain no values.
    };

    FloatPoint points[3];
    Type type;
};

using PathApplierFunction = WTF::Function<void(const PathElement&)>;

class Path {
    WTF_MAKE_FAST_ALLOCATED;
public:
    WEBCORE_EXPORT Path();
#if USE(CG)
    Path(RetainPtr<CGMutablePathRef>);
#endif
    WEBCORE_EXPORT ~Path();

    WEBCORE_EXPORT Path(const Path&);
    WEBCORE_EXPORT Path(Path&&);
    WEBCORE_EXPORT Path& operator=(const Path&);
    WEBCORE_EXPORT Path& operator=(Path&&);

    static Path polygonPathFromPoints(const Vector<FloatPoint>&);

    bool contains(const FloatPoint&, WindRule = WindRule::NonZero) const;
    bool strokeContains(StrokeStyleApplier&, const FloatPoint&) const;
    // fastBoundingRect() should equal or contain boundingRect(); boundingRect()
    // should perfectly bound the points within the path.
    FloatRect boundingRect() const;
    WEBCORE_EXPORT FloatRect fastBoundingRect() const;
    FloatRect strokeBoundingRect(StrokeStyleApplier* = 0) const;

    float length() const;
    PathTraversalState traversalStateAtLength(float length) const;
    FloatPoint pointAtLength(float length) const;

    WEBCORE_EXPORT void clear();
    bool isNull() const { return !m_path; }
    bool isEmpty() const;
    // Gets the current point of the current path, which is conceptually the final point reached by the path so far.
    // Note the Path can be empty (isEmpty() == true) and still have a current point.
    bool hasCurrentPoint() const;
    FloatPoint currentPoint() const;

    WEBCORE_EXPORT void moveTo(const FloatPoint&);
    WEBCORE_EXPORT void addLineTo(const FloatPoint&);
    WEBCORE_EXPORT void addQuadCurveTo(const FloatPoint& controlPoint, const FloatPoint& endPoint);
    WEBCORE_EXPORT void addBezierCurveTo(const FloatPoint& controlPoint1, const FloatPoint& controlPoint2, const FloatPoint& endPoint);
    void addArcTo(const FloatPoint&, const FloatPoint&, float radius);
    WEBCORE_EXPORT void closeSubpath();

    void addArc(const FloatPoint&, float radius, float startAngle, float endAngle, bool anticlockwise);
    void addRect(const FloatRect&);
    void addEllipse(FloatPoint, float radiusX, float radiusY, float rotation, float startAngle, float endAngle, bool anticlockwise);
    void addEllipse(const FloatRect&);

    enum class RoundedRectStrategy : uint8_t {
        PreferNative,
        PreferBezier
    };

    WEBCORE_EXPORT void addRoundedRect(const FloatRect&, const FloatSize& roundingRadii, RoundedRectStrategy = RoundedRectStrategy::PreferNative);
    WEBCORE_EXPORT void addRoundedRect(const FloatRoundedRect&, RoundedRectStrategy = RoundedRectStrategy::PreferNative);
    void addRoundedRect(const RoundedRect&);

    void addPath(const Path&, const AffineTransform&);

    void translate(const FloatSize&);

    // To keep Path() cheap, it does not allocate a PlatformPath immediately
    // meaning Path::platformPath() can return null.
#if USE(DIRECT2D)
    FloatRect fastBoundingRectForStroke(const PlatformContextDirect2D&) const;
    PlatformPathPtr platformPath() const { return m_path.get(); }
#else
    PlatformPathPtr platformPath() const { return m_path; }
#endif
    // ensurePlatformPath() will allocate a PlatformPath if it has not yet been and will never return null.
    WEBCORE_EXPORT PlatformPathPtr ensurePlatformPath();

    WEBCORE_EXPORT void apply(const PathApplierFunction&) const;
    void transform(const AffineTransform&);

    static float circleControlPoint()
    {
        // Approximation of control point positions on a bezier to simulate a quarter of a circle.
        // This is 1-kappa, where kappa = 4 * (sqrt(2) - 1) / 3
        return 0.447715;
    }

    void addBeziersForRoundedRect(const FloatRect&, const FloatSize& topLeftRadius, const FloatSize& topRightRadius, const FloatSize& bottomLeftRadius, const FloatSize& bottomRightRadius);

#if USE(CG) || USE(DIRECT2D)
    void platformAddPathForRoundedRect(const FloatRect&, const FloatSize& topLeftRadius, const FloatSize& topRightRadius, const FloatSize& bottomLeftRadius, const FloatSize& bottomRightRadius);
#endif

#if USE(DIRECT2D)
    void appendGeometry(ID2D1Geometry*);
    void createGeometryWithFillMode(WindRule, COMPtr<ID2D1GeometryGroup>&) const;

    void openFigureAtCurrentPointIfNecessary();
    void closeAnyOpenGeometries(unsigned figureEndStyle) const;
    void clearGeometries();
#endif

#ifndef NDEBUG
    void dump() const;
#endif

    template<class Encoder> void encode(Encoder&) const;
    template<class Decoder> static Optional<Path> decode(Decoder&);

private:
#if USE(DIRECT2D)
    Vector<ID2D1Geometry*> m_geometries;
    COMPtr<ID2D1GeometryGroup> m_path;
    mutable COMPtr<ID2D1GeometrySink> m_activePath;
    mutable bool m_figureIsOpened { false };
#else
    PlatformPathPtr m_path { nullptr };
#endif
};

WTF::TextStream& operator<<(WTF::TextStream&, const Path&);

template<class Encoder> void Path::encode(Encoder& encoder) const
{
    uint64_t numPoints = 0;
    apply([&numPoints](const PathElement&) {
        ++numPoints;
    });

    encoder << numPoints;

    apply([&](auto& element) {
        encoder.encodeEnum(element.type);

        switch (element.type) {
        case PathElement::Type::MoveToPoint:
            encoder << element.points[0];
            break;
        case PathElement::Type::AddLineToPoint:
            encoder << element.points[0];
            break;
        case PathElement::Type::AddQuadCurveToPoint:
            encoder << element.points[0];
            encoder << element.points[1];
            break;
        case PathElement::Type::AddCurveToPoint:
            encoder << element.points[0];
            encoder << element.points[1];
            encoder << element.points[2];
            break;
        case PathElement::Type::CloseSubpath:
            break;
        }
    });
}

template<class Decoder> Optional<Path> Path::decode(Decoder& decoder)
{
    Path path;
    uint64_t numPoints;
    if (!decoder.decode(numPoints))
        return WTF::nullopt;

    path.clear();

    for (uint64_t i = 0; i < numPoints; ++i) {
        PathElement::Type elementType;
        if (!decoder.decodeEnum(elementType))
            return WTF::nullopt;

        switch (elementType) {
        case PathElement::Type::MoveToPoint: {
            FloatPoint point;
            if (!decoder.decode(point))
                return WTF::nullopt;
            path.moveTo(point);
            break;
        }
        case PathElement::Type::AddLineToPoint: {
            FloatPoint point;
            if (!decoder.decode(point))
                return WTF::nullopt;
            path.addLineTo(point);
            break;
        }
        case PathElement::Type::AddQuadCurveToPoint: {
            FloatPoint controlPoint;
            if (!decoder.decode(controlPoint))
                return WTF::nullopt;

            FloatPoint endPoint;
            if (!decoder.decode(endPoint))
                return WTF::nullopt;

            path.addQuadCurveTo(controlPoint, endPoint);
            break;
        }
        case PathElement::Type::AddCurveToPoint: {
            FloatPoint controlPoint1;
            if (!decoder.decode(controlPoint1))
                return WTF::nullopt;

            FloatPoint controlPoint2;
            if (!decoder.decode(controlPoint2))
                return WTF::nullopt;

            FloatPoint endPoint;
            if (!decoder.decode(endPoint))
                return WTF::nullopt;

            path.addBezierCurveTo(controlPoint1, controlPoint2, endPoint);
            break;
        }
        case PathElement::Type::CloseSubpath:
            path.closeSubpath();
            break;
        }
    }

    return path;
}

}

#endif