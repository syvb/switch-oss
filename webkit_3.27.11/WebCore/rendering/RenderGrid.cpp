/*
 * Copyright (C) 2011, 2022 Apple Inc. All rights reserved.
 * Copyright (C) 2013-2017 Igalia S.L.
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

#include "config.h"
#include "RenderGrid.h"

#include "GridArea.h"
#include "GridLayoutFunctions.h"
#include "GridPositionsResolver.h"
#include "GridTrackSizingAlgorithm.h"
#include "InspectorInstrumentation.h"
#include "LayoutRepainter.h"
#include "RenderChildIterator.h"
#include "RenderLayer.h"
#include "RenderLayoutState.h"
#include "RenderTreeBuilder.h"
#include "RenderView.h"
#include <cstdlib>
#include <wtf/IsoMallocInlines.h>

namespace WebCore {

WTF_MAKE_ISO_ALLOCATED_IMPL(RenderGrid);

enum TrackSizeRestriction {
    AllowInfinity,
    ForbidInfinity,
};

RenderGrid::RenderGrid(Element& element, RenderStyle&& style)
    : RenderBlock(element, WTFMove(style), 0)
    , m_grid(*this)
    , m_trackSizingAlgorithm(this, currentGrid())
{
    // All of our children must be block level.
    setChildrenInline(false);

    InspectorInstrumentation::nodeLayoutContextChanged(element, this);
}

RenderGrid::~RenderGrid()
{
    InspectorInstrumentation::nodeLayoutContextChanged(element(), nullptr);
}

StyleSelfAlignmentData RenderGrid::selfAlignmentForChild(GridAxis axis, const RenderBox& child, const RenderStyle* gridStyle) const
{
    return axis == GridRowAxis ? justifySelfForChild(child, StretchingMode::Any, gridStyle) : alignSelfForChild(child, StretchingMode::Any, gridStyle);
}

bool RenderGrid::selfAlignmentChangedToStretch(GridAxis axis, const RenderStyle& oldStyle, const RenderStyle& newStyle, const RenderBox& child) const
{
    return selfAlignmentForChild(axis, child, &oldStyle).position() != ItemPosition::Stretch
        && selfAlignmentForChild(axis, child, &newStyle).position() == ItemPosition::Stretch;
}

bool RenderGrid::selfAlignmentChangedFromStretch(GridAxis axis, const RenderStyle& oldStyle, const RenderStyle& newStyle, const RenderBox& child) const
{
    return selfAlignmentForChild(axis, child, &oldStyle).position() == ItemPosition::Stretch
        && selfAlignmentForChild(axis, child, &newStyle).position() != ItemPosition::Stretch;
}

void RenderGrid::styleDidChange(StyleDifference diff, const RenderStyle* oldStyle)
{
    RenderBlock::styleDidChange(diff, oldStyle);
    if (!oldStyle || diff != StyleDifference::Layout)
        return;

    const RenderStyle& newStyle = this->style();
    if (oldStyle->resolvedAlignItems(selfAlignmentNormalBehavior(this)).position() == ItemPosition::Stretch) {
        // Style changes on the grid container implying stretching (to-stretch) or
        // shrinking (from-stretch) require the affected items to be laid out again.
        // These logic only applies to 'stretch' since the rest of the alignment
        // values don't change the size of the box.
        // In any case, the items' overrideSize will be cleared and recomputed (if
        // necessary)  as part of the Grid layout logic, triggered by this style
        // change.
        for (auto& child : childrenOfType<RenderBox>(*this)) {
            if (child.isOutOfFlowPositioned())
                continue;
            if (selfAlignmentChangedToStretch(GridRowAxis, *oldStyle, newStyle, child)
                || selfAlignmentChangedFromStretch(GridRowAxis, *oldStyle, newStyle, child)
                || selfAlignmentChangedToStretch(GridColumnAxis, *oldStyle, newStyle, child)
                || selfAlignmentChangedFromStretch(GridColumnAxis, *oldStyle, newStyle, child)) {
                child.setNeedsLayout();
            }
        }
    }

    if (explicitGridDidResize(*oldStyle) || namedGridLinesDefinitionDidChange(*oldStyle) || oldStyle->gridAutoFlow() != style().gridAutoFlow()
        || (style().gridAutoRepeatColumns().size() || style().gridAutoRepeatRows().size()))
        dirtyGrid();
}

bool RenderGrid::explicitGridDidResize(const RenderStyle& oldStyle) const
{
    return oldStyle.gridColumns().size() != style().gridColumns().size()
        || oldStyle.gridRows().size() != style().gridRows().size()
        || oldStyle.namedGridAreaColumnCount() != style().namedGridAreaColumnCount()
        || oldStyle.namedGridAreaRowCount() != style().namedGridAreaRowCount()
        || oldStyle.gridAutoRepeatColumns().size() != style().gridAutoRepeatColumns().size()
        || oldStyle.gridAutoRepeatRows().size() != style().gridAutoRepeatRows().size();
}

bool RenderGrid::namedGridLinesDefinitionDidChange(const RenderStyle& oldStyle) const
{
    return oldStyle.namedGridRowLines() != style().namedGridRowLines()
        || oldStyle.namedGridColumnLines() != style().namedGridColumnLines();
}

// This method optimizes the gutters computation by skiping the available size
// call if gaps are fixed size (it's only needed for percentages).
std::optional<LayoutUnit> RenderGrid::availableSpaceForGutters(GridTrackSizingDirection direction) const
{
    bool isRowAxis = direction == ForColumns;
    const GapLength& gapLength = isRowAxis ? style().columnGap() : style().rowGap();
    if (gapLength.isNormal() || !gapLength.length().isPercentOrCalculated())
        return std::nullopt;

    return isRowAxis ? availableLogicalWidth() : contentLogicalHeight();
}

void RenderGrid::computeTrackSizesForDefiniteSize(GridTrackSizingDirection direction, LayoutUnit availableSpace)
{
    m_trackSizingAlgorithm.setup(direction, numTracks(direction), TrackSizing, availableSpace);
    m_trackSizingAlgorithm.run();

    ASSERT(m_trackSizingAlgorithm.tracksAreWiderThanMinTrackBreadth());
}

void RenderGrid::repeatTracksSizingIfNeeded(LayoutUnit availableSpaceForColumns, LayoutUnit availableSpaceForRows)
{
    // In orthogonal flow cases column track's size is determined by using the computed
    // row track's size, which it was estimated during the first cycle of the sizing
    // algorithm. Hence we need to repeat computeUsedBreadthOfGridTracks for both,
    // columns and rows, to determine the final values.
    // TODO (lajava): orthogonal flows is just one of the cases which may require
    // a new cycle of the sizing algorithm; there may be more. In addition, not all the
    // cases with orthogonal flows require this extra cycle; we need a more specific
    // condition to detect whether child's min-content contribution has changed or not.
    // The complication with repeating the track sizing algorithm for flex max-sizing is that
    // it might change a grid item's status of participating in Baseline Alignment for
    // a cyclic sizing dependncy case, which should be definitively excluded. See
    // https://github.com/w3c/csswg-drafts/issues/3046 for details.
    // FIXME: we are avoiding repeating the track sizing algorithm for grid item with baseline alignment
    // here in the case of using flex max-sizing functions. We probably also need to investigate whether
    // it is applicable for the case of percent-sized rows with indefinite height as well.
    if (m_hasAnyOrthogonalItem || m_trackSizingAlgorithm.hasAnyPercentSizedRowsIndefiniteHeight() || (m_trackSizingAlgorithm.hasAnyFlexibleMaxTrackBreadth() && !m_trackSizingAlgorithm.hasAnyBaselineAlignmentItem()) || m_hasAspectRatioBlockSizeDependentItem) {
        computeTrackSizesForDefiniteSize(ForColumns, availableSpaceForColumns);
        computeContentPositionAndDistributionOffset(ForColumns, m_trackSizingAlgorithm.freeSpace(ForColumns).value(), nonCollapsedTracks(ForColumns));
        computeTrackSizesForDefiniteSize(ForRows, availableSpaceForRows);
        computeContentPositionAndDistributionOffset(ForRows, m_trackSizingAlgorithm.freeSpace(ForRows).value(), nonCollapsedTracks(ForRows));
    }
}

bool RenderGrid::canPerformSimplifiedLayout() const
{
    // We cannot perform a simplified layout if we need to position the items and we have some
    // positioned items to be laid out.
    if (currentGrid().needsItemsPlacement() && posChildNeedsLayout())
        return false;

    return RenderBlock::canPerformSimplifiedLayout();
}

Vector<RenderBox*> RenderGrid::computeAspectRatioDependentAndBaselineItems()
{
    Vector<RenderBox*> dependentGridItems;

    m_baselineItemsCached = true;
    m_hasAnyOrthogonalItem = false;
    m_hasAspectRatioBlockSizeDependentItem = false;

    for (auto* child = firstChildBox(); child; child = child->nextSiblingBox()) {
        if (child->isOutOfFlowPositioned() || child->isLegend())
            continue;

        // Grid's layout logic controls the grid item's override height, hence we need to
        // clear any override height set previously, so it doesn't interfere in current layout
        // execution. Grid never uses the override width, that's why we don't need to clear  it.
        child->clearOverridingLogicalHeight();

        // Grid's layout logic controls the grid item's override height, hence we need to
        // clear any override height set previously, so it doesn't interfere in current layout
        // execution. Grid never uses the override width, that's why we don't need to clear  it.
        child->clearOverridingLogicalHeight();

        // We may need to repeat the track sizing in case of any grid item was orthogonal.
        if (GridLayoutFunctions::isOrthogonalChild(*this, *child))
            m_hasAnyOrthogonalItem = true;

        // For a grid item that has an aspect-ratio and block-constraints such as the relative logical height,
        // when the grid width is auto, we may need get the real grid width before laying out the item.
        if (GridLayoutFunctions::isAspectRatioBlockSizeDependentChild(*child) && (style().logicalWidth().isAuto() || style().logicalWidth().isMinContent() || style().logicalWidth().isMaxContent())) {
            dependentGridItems.append(child);
            m_hasAspectRatioBlockSizeDependentItem = true;
        }

        // We keep a cache of items with baseline as alignment values so that we only compute the baseline shims for
        // such items. This cache is needed for performance related reasons due to the cost of evaluating the item's
        // participation in a baseline context during the track sizing algorithm.
        if (isBaselineAlignmentForChild(*child, GridColumnAxis))
            m_trackSizingAlgorithm.cacheBaselineAlignedItem(*child, GridColumnAxis);
        if (isBaselineAlignmentForChild(*child, GridRowAxis))
            m_trackSizingAlgorithm.cacheBaselineAlignedItem(*child, GridRowAxis);
    }

    return dependentGridItems;
}

void RenderGrid::layoutBlock(bool relayoutChildren, LayoutUnit)
{
    ASSERT(needsLayout());

    if (!relayoutChildren && simplifiedLayout())
        return;

    LayoutRepainter repainter(*this, checkForRepaintDuringLayout());
    {
        LayoutStateMaintainer statePusher(*this, locationOffset(), hasTransform() || hasReflection() || style().isFlippedBlocksWritingMode());

        preparePaginationBeforeBlockLayout(relayoutChildren);
        beginUpdateScrollInfoAfterLayoutTransaction();

        LayoutSize previousSize = size();

        // FIXME: We should use RenderBlock::hasDefiniteLogicalHeight() only but it does not work for positioned stuff.
        // FIXME: Consider caching the hasDefiniteLogicalHeight value throughout the layout.
        // FIXME: We might need to cache the hasDefiniteLogicalHeight if the call of RenderBlock::hasDefiniteLogicalHeight() causes a relevant performance regression.
        bool hasDefiniteLogicalHeight = RenderBlock::hasDefiniteLogicalHeight() || hasOverridingLogicalHeight() || computeContentLogicalHeight(MainOrPreferredSize, style().logicalHeight(), std::nullopt);

        auto aspectRatioBlockSizeDependentGridItems = computeAspectRatioDependentAndBaselineItems();

        resetLogicalHeightBeforeLayoutIfNeeded();

        updateLogicalWidth();

        // Fieldsets need to find their legend and position it inside the border of the object.
        // The legend then gets skipped during normal layout. The same is true for ruby text.
        // It doesn't get included in the normal layout process but is instead skipped.
        layoutExcludedChildren(relayoutChildren);

        LayoutUnit availableSpaceForColumns = availableLogicalWidth();
        placeItemsOnGrid(availableSpaceForColumns);

        m_trackSizingAlgorithm.setAvailableSpace(ForColumns, availableSpaceForColumns);
        performGridItemsPreLayout(m_trackSizingAlgorithm);

        // 1- First, the track sizing algorithm is used to resolve the sizes of the grid columns. At this point the 
        // logical width is always definite as the above call to updateLogicalWidth() properly resolves intrinsic 
        // sizes. We cannot do the same for heights though because many code paths inside updateLogicalHeight() require 
        // a previous call to setLogicalHeight() to resolve heights properly (like for positioned items for example).
        computeTrackSizesForDefiniteSize(ForColumns, availableSpaceForColumns);

        // 1.5- Compute Content Distribution offsets for column tracks
        computeContentPositionAndDistributionOffset(ForColumns, m_trackSizingAlgorithm.freeSpace(ForColumns).value(), nonCollapsedTracks(ForColumns));

        // 2- Next, the track sizing algorithm resolves the sizes of the grid rows,
        // using the grid column sizes calculated in the previous step.
        bool shouldRecomputeHeight = false;
        if (!hasDefiniteLogicalHeight) {
            computeTrackSizesForIndefiniteSize(m_trackSizingAlgorithm, ForRows);
            if (shouldApplySizeContainment(*this))
                shouldRecomputeHeight = true;
        } else
            computeTrackSizesForDefiniteSize(ForRows, availableLogicalHeight(ExcludeMarginBorderPadding));
        LayoutUnit trackBasedLogicalHeight = m_trackSizingAlgorithm.computeTrackBasedSize() + borderAndPaddingLogicalHeight() + scrollbarLogicalHeight();
        if (shouldRecomputeHeight)
            computeTrackSizesForDefiniteSize(ForRows, trackBasedLogicalHeight);

        setLogicalHeight(trackBasedLogicalHeight);

        updateLogicalHeight();

        // Once grid's indefinite height is resolved, we can compute the
        // available free space for Content Alignment.
        if (!hasDefiniteLogicalHeight)
            m_trackSizingAlgorithm.setFreeSpace(ForRows, logicalHeight() - trackBasedLogicalHeight);

        // 2.5- Compute Content Distribution offsets for rows tracks
        computeContentPositionAndDistributionOffset(ForRows, m_trackSizingAlgorithm.freeSpace(ForRows).value(), nonCollapsedTracks(ForRows));

        if (!aspectRatioBlockSizeDependentGridItems.isEmpty()) {
            updateGridAreaForAspectRatioItems(aspectRatioBlockSizeDependentGridItems);
            updateLogicalWidth();
        }

        // 3- If the min-content contribution of any grid items have changed based on the row
        // sizes calculated in step 2, steps 1 and 2 are repeated with the new min-content
        // contribution (once only).
        repeatTracksSizingIfNeeded(availableSpaceForColumns, contentLogicalHeight());

        // Grid container should have the minimum height of a line if it's editable. That does not affect track sizing though.
        if (hasLineIfEmpty()) {
            LayoutUnit minHeightForEmptyLine = borderAndPaddingLogicalHeight()
                + lineHeight(true, isHorizontalWritingMode() ? HorizontalLine : VerticalLine, PositionOfInteriorLineBoxes)
                + scrollbarLogicalHeight();
            setLogicalHeight(std::max(logicalHeight(), minHeightForEmptyLine));
        }

        layoutGridItems();
        m_trackSizingAlgorithm.reset();

        endAndCommitUpdateScrollInfoAfterLayoutTransaction();

        if (size() != previousSize)
            relayoutChildren = true;

        m_outOfFlowItemColumn.clear();
        m_outOfFlowItemRow.clear();

        layoutPositionedObjects(relayoutChildren || isDocumentElementRenderer());

        computeOverflow(clientLogicalRightAndBottomAfterRepositioning().height());
    }

    updateLayerTransform();

    // Update our scroll information if we're overflow:auto/scroll/hidden now that we know if
    // we overflow or not.
    updateScrollInfoAfterLayout();

    repainter.repaintAfterLayout();

    clearNeedsLayout();

    m_trackSizingAlgorithm.clearBaselineItemsCache();
    m_baselineItemsCached = false;
}

LayoutUnit RenderGrid::gridGap(GridTrackSizingDirection direction, std::optional<LayoutUnit> availableSize) const
{
    ASSERT(!availableSize || *availableSize >= 0);
    const GapLength& gapLength = direction == ForColumns? style().columnGap() : style().rowGap();
    if (gapLength.isNormal())
        return 0_lu;

    return valueForLength(gapLength.length(), availableSize.value_or(0));
}

LayoutUnit RenderGrid::gridGap(GridTrackSizingDirection direction) const
{
    return gridGap(direction, availableSpaceForGutters(direction));
}

LayoutUnit RenderGrid::gridItemOffset(GridTrackSizingDirection direction) const
{
    return direction == ForRows ? m_offsetBetweenRows.distributionOffset : m_offsetBetweenColumns.distributionOffset;
}

LayoutUnit RenderGrid::guttersSize(GridTrackSizingDirection direction, unsigned startLine, unsigned span, std::optional<LayoutUnit> availableSize) const
{
    if (span <= 1)
        return { };

    LayoutUnit gap = gridGap(direction, availableSize);

    // Fast path, no collapsing tracks.
    if (!currentGrid().hasAutoRepeatEmptyTracks(direction))
        return gap * (span - 1);

    // If there are collapsing tracks we need to be sure that gutters are properly collapsed. Apart
    // from that, if we have a collapsed track in the edges of the span we're considering, we need
    // to move forward (or backwards) in order to know whether the collapsed tracks reach the end of
    // the grid (so the gap becomes 0) or there is a non empty track before that.

    LayoutUnit gapAccumulator;
    unsigned endLine = startLine + span;

    for (unsigned line = startLine; line < endLine - 1; ++line) {
        if (!currentGrid().isEmptyAutoRepeatTrack(direction, line))
            gapAccumulator += gap;
    }

    // The above loop adds one extra gap for trailing collapsed tracks.
    if (gapAccumulator && currentGrid().isEmptyAutoRepeatTrack(direction, endLine - 1)) {
        ASSERT(gapAccumulator >= gap);
        gapAccumulator -= gap;
    }

    // If the startLine is the start line of a collapsed track we need to go backwards till we reach
    // a non collapsed track. If we find a non collapsed track we need to add that gap.
    size_t nonEmptyTracksBeforeStartLine = 0;
    if (startLine && currentGrid().isEmptyAutoRepeatTrack(direction, startLine)) {
        nonEmptyTracksBeforeStartLine = startLine;
        auto begin = currentGrid().autoRepeatEmptyTracks(direction)->begin();
        for (auto it = begin; *it != startLine; ++it) {
            ASSERT(nonEmptyTracksBeforeStartLine);
            --nonEmptyTracksBeforeStartLine;
        }
        if (nonEmptyTracksBeforeStartLine)
            gapAccumulator += gap;
    }

    // If the endLine is the end line of a collapsed track we need to go forward till we reach a non
    // collapsed track. If we find a non collapsed track we need to add that gap.
    if (currentGrid().isEmptyAutoRepeatTrack(direction, endLine - 1)) {
        unsigned nonEmptyTracksAfterEndLine = currentGrid().numTracks(direction) - endLine;
        auto currentEmptyTrack = currentGrid().autoRepeatEmptyTracks(direction)->find(endLine - 1);
        auto endEmptyTrack = currentGrid().autoRepeatEmptyTracks(direction)->end();
        // HashSet iterators do not implement operator- so we have to manually iterate to know the number of remaining empty tracks.
        for (auto it = ++currentEmptyTrack; it != endEmptyTrack; ++it) {
            ASSERT(nonEmptyTracksAfterEndLine >= 1);
            --nonEmptyTracksAfterEndLine;
        }
        if (nonEmptyTracksAfterEndLine) {
            // We shouldn't count the gap twice if the span starts and ends in a collapsed track bewtween two non-empty tracks.
            if (!nonEmptyTracksBeforeStartLine)
                gapAccumulator += gap;
        } else if (nonEmptyTracksBeforeStartLine) {
            // We shouldn't count the gap if the the span starts and ends in a collapsed but there isn't non-empty tracks afterwards (it's at the end of the grid).
            gapAccumulator -= gap;
        }
    }

    return gapAccumulator;
}

void RenderGrid::computeIntrinsicLogicalWidths(LayoutUnit& minLogicalWidth, LayoutUnit& maxLogicalWidth) const
{
    LayoutUnit childMinWidth;
    LayoutUnit childMaxWidth;
    bool hadExcludedChildren = computePreferredWidthsForExcludedChildren(childMinWidth, childMaxWidth);

    Grid grid(const_cast<RenderGrid&>(*this));
    m_grid.m_currentGrid = std::ref(grid);
    GridTrackSizingAlgorithm algorithm(this, grid);
    // placeItemsOnGrid isn't const since it mutates our grid, but it's safe to do
    // so here since we've overriden m_currentGrid with a stack based temporary.
    const_cast<RenderGrid&>(*this).placeItemsOnGrid(std::nullopt);

    performGridItemsPreLayout(algorithm);

    if (m_baselineItemsCached)
        algorithm.copyBaselineItemsCache(m_trackSizingAlgorithm, GridRowAxis);
    else {
        for (auto* child = firstChildBox(); child; child = child->nextSiblingBox()) {
            if (child->isOutOfFlowPositioned())
                continue;
            if (isBaselineAlignmentForChild(*child, GridRowAxis))
                algorithm.cacheBaselineAlignedItem(*child, GridRowAxis);
        }
    }

    computeTrackSizesForIndefiniteSize(algorithm, ForColumns, &minLogicalWidth, &maxLogicalWidth);

    m_grid.resetCurrentGrid();

    if (hadExcludedChildren) {
        minLogicalWidth = std::max(minLogicalWidth, childMinWidth);
        maxLogicalWidth = std::max(maxLogicalWidth, childMaxWidth);
    }

    LayoutUnit scrollbarWidth = intrinsicScrollbarLogicalWidth();
    minLogicalWidth += scrollbarWidth;
    maxLogicalWidth += scrollbarWidth;
}

void RenderGrid::computeTrackSizesForIndefiniteSize(GridTrackSizingAlgorithm& algorithm, GridTrackSizingDirection direction, LayoutUnit* minIntrinsicSize, LayoutUnit* maxIntrinsicSize) const
{
    algorithm.setup(direction, numTracks(direction), IntrinsicSizeComputation, std::nullopt);
    algorithm.run();

    size_t numberOfTracks = algorithm.tracks(direction).size();
    LayoutUnit totalGuttersSize = guttersSize(direction, 0, numberOfTracks, std::nullopt);

    if (minIntrinsicSize)
        *minIntrinsicSize = algorithm.minContentSize() + totalGuttersSize;
    if (maxIntrinsicSize)
        *maxIntrinsicSize = algorithm.maxContentSize() + totalGuttersSize;

    ASSERT(algorithm.tracksAreWiderThanMinTrackBreadth());
}

unsigned RenderGrid::computeAutoRepeatTracksCount(GridTrackSizingDirection direction, std::optional<LayoutUnit> availableSize) const
{
    ASSERT(!availableSize || availableSize.value() != -1);
    bool isRowAxis = direction == ForColumns;
    const auto& autoRepeatTracks = isRowAxis ? style().gridAutoRepeatColumns() : style().gridAutoRepeatRows();
    unsigned autoRepeatTrackListLength = autoRepeatTracks.size();

    if (!autoRepeatTrackListLength)
        return 0;

    bool needsToFulfillMinimumSize = false;
    if (!availableSize) {
        const Length& maxSize = isRowAxis ? style().logicalMaxWidth() : style().logicalMaxHeight();
        std::optional<LayoutUnit> containingBlockAvailableSize;
        std::optional<LayoutUnit> availableMaxSize;
        if (maxSize.isSpecified()) {
            if (maxSize.isPercentOrCalculated())
                containingBlockAvailableSize = isRowAxis ? containingBlockLogicalWidthForContent() : containingBlockLogicalHeightForContent(ExcludeMarginBorderPadding);
            LayoutUnit maxSizeValue = valueForLength(maxSize, valueOrDefault(containingBlockAvailableSize));
            availableMaxSize = isRowAxis ? adjustContentBoxLogicalWidthForBoxSizing(maxSizeValue, maxSize.type()) : adjustContentBoxLogicalHeightForBoxSizing(maxSizeValue);
        }

        const Length& minSize = isRowAxis ? style().logicalMinWidth() : style().logicalMinHeight();
        const auto& minSizeForOrthogonalAxis = isRowAxis ? style().logicalMinHeight() : style().logicalMinWidth();
        bool shouldComputeMinSizeFromAspectRatio = minSizeForOrthogonalAxis.isSpecified() && !shouldIgnoreAspectRatio();

        if (!availableMaxSize && !minSize.isSpecified() && !shouldComputeMinSizeFromAspectRatio)
            return autoRepeatTrackListLength;

        std::optional<LayoutUnit> availableMinSize;
        if (minSize.isSpecified()) {
            if (!containingBlockAvailableSize && minSize.isPercentOrCalculated())
                containingBlockAvailableSize = isRowAxis ? containingBlockLogicalWidthForContent() : containingBlockLogicalHeightForContent(ExcludeMarginBorderPadding);
            LayoutUnit minSizeValue = valueForLength(minSize, valueOrDefault(containingBlockAvailableSize));
            availableMinSize = isRowAxis ? adjustContentBoxLogicalWidthForBoxSizing(minSizeValue, minSize.type()) : adjustContentBoxLogicalHeightForBoxSizing(minSizeValue);
        } else if (shouldComputeMinSizeFromAspectRatio) {
            auto [logicalMinWidth, logicalMaxWidth] = computeMinMaxLogicalWidthFromAspectRatio();
            availableMinSize = logicalMinWidth;
        }
        if (!maxSize.isSpecified())
            needsToFulfillMinimumSize = true;

        availableSize = std::max(valueOrDefault(availableMinSize), valueOrDefault(availableMaxSize));
    }

    LayoutUnit autoRepeatTracksSize;
    for (auto& autoTrackSize : autoRepeatTracks) {
        ASSERT(autoTrackSize.minTrackBreadth().isLength());
        ASSERT(!autoTrackSize.minTrackBreadth().isFlex());
        bool hasDefiniteMaxTrackSizingFunction = autoTrackSize.maxTrackBreadth().isLength() && !autoTrackSize.maxTrackBreadth().isContentSized();
        auto trackLength = hasDefiniteMaxTrackSizingFunction ? autoTrackSize.maxTrackBreadth().length() : autoTrackSize.minTrackBreadth().length();
        bool hasDefiniteMinTrackSizingFunction = autoTrackSize.minTrackBreadth().isLength() && !autoTrackSize.minTrackBreadth().isContentSized();
        if (hasDefiniteMinTrackSizingFunction && (trackLength.value() < autoTrackSize.minTrackBreadth().length().value()))
            trackLength = autoTrackSize.minTrackBreadth().length();
        autoRepeatTracksSize += valueForLength(trackLength, availableSize.value());
    }
    // For the purpose of finding the number of auto-repeated tracks, the UA must floor the track size to a UA-specified
    // value to avoid division by zero. It is suggested that this floor be 1px.
    autoRepeatTracksSize = std::max<LayoutUnit>(1_lu, autoRepeatTracksSize);

    // There will be always at least 1 auto-repeat track, so take it already into account when computing the total track size.
    LayoutUnit tracksSize = autoRepeatTracksSize;
    auto& trackSizes = isRowAxis ? style().gridColumns() : style().gridRows();

    for (const auto& track : trackSizes) {
        bool hasDefiniteMaxTrackBreadth = track.maxTrackBreadth().isLength() && !track.maxTrackBreadth().isContentSized();
        ASSERT(hasDefiniteMaxTrackBreadth || (track.minTrackBreadth().isLength() && !track.minTrackBreadth().isContentSized()));
        tracksSize += valueForLength(hasDefiniteMaxTrackBreadth ? track.maxTrackBreadth().length() : track.minTrackBreadth().length(), availableSize.value());
    }

    // Add gutters as if auto repeat tracks were only repeated once. Gaps between different repetitions will be added later when
    // computing the number of repetitions of the auto repeat().
    LayoutUnit gapSize = gridGap(direction, availableSize);
    tracksSize += gapSize * (trackSizes.size() + autoRepeatTrackListLength - 1);

    LayoutUnit freeSpace = availableSize.value() - tracksSize;
    if (freeSpace <= 0)
        return autoRepeatTrackListLength;

    LayoutUnit autoRepeatSizeWithGap = autoRepeatTracksSize + gapSize * autoRepeatTrackListLength;
    unsigned repetitions = 1 + (freeSpace / autoRepeatSizeWithGap).toUnsigned();
    freeSpace -= autoRepeatSizeWithGap * (repetitions - 1);
    ASSERT(freeSpace >= 0);

    // Provided the grid container does not have a definite size or max-size in the relevant axis,
    // if the min size is definite then the number of repetitions is the largest possible positive
    // integer that fulfills that minimum requirement.
    if (needsToFulfillMinimumSize && freeSpace)
        ++repetitions;

    return repetitions * autoRepeatTrackListLength;
}


std::unique_ptr<OrderedTrackIndexSet> RenderGrid::computeEmptyTracksForAutoRepeat(GridTrackSizingDirection direction) const
{
    bool isRowAxis = direction == ForColumns;
    if ((isRowAxis && style().gridAutoRepeatColumnsType() != AutoRepeatType::Fit)
        || (!isRowAxis && style().gridAutoRepeatRowsType() != AutoRepeatType::Fit))
        return nullptr;

    std::unique_ptr<OrderedTrackIndexSet> emptyTrackIndexes;
    unsigned insertionPoint = isRowAxis ? style().gridAutoRepeatColumnsInsertionPoint() : style().gridAutoRepeatRowsInsertionPoint();
    unsigned firstAutoRepeatTrack = insertionPoint + currentGrid().explicitGridStart(direction);
    unsigned lastAutoRepeatTrack = firstAutoRepeatTrack + currentGrid().autoRepeatTracks(direction);

    if (!currentGrid().hasGridItems() || shouldApplySizeContainment(*this)) {
        emptyTrackIndexes = makeUnique<OrderedTrackIndexSet>();
        for (unsigned trackIndex = firstAutoRepeatTrack; trackIndex < lastAutoRepeatTrack; ++trackIndex)
            emptyTrackIndexes->add(trackIndex);
    } else {
        for (unsigned trackIndex = firstAutoRepeatTrack; trackIndex < lastAutoRepeatTrack; ++trackIndex) {
            GridIterator iterator(currentGrid(), direction, trackIndex);
            if (!iterator.nextGridItem()) {
                if (!emptyTrackIndexes)
                    emptyTrackIndexes = makeUnique<OrderedTrackIndexSet>();
                emptyTrackIndexes->add(trackIndex);
            }
        }
    }
    return emptyTrackIndexes;
}

unsigned RenderGrid::clampAutoRepeatTracks(GridTrackSizingDirection direction, unsigned autoRepeatTracks) const
{
    if (!autoRepeatTracks)
        return 0;

    unsigned insertionPoint = direction == ForColumns ? style().gridAutoRepeatColumnsInsertionPoint() : style().gridAutoRepeatRowsInsertionPoint();
    unsigned maxTracks = static_cast<unsigned>(GridPosition::max());

    if (!insertionPoint)
        return std::min(autoRepeatTracks, maxTracks);

    if (insertionPoint >= maxTracks)
        return 0;

    return std::min(autoRepeatTracks, maxTracks - insertionPoint);
}

// FIXME: We shouldn't have to pass the available logical width as argument. The problem is that
// availableLogicalWidth() does always return a value even if we cannot resolve it like when
// computing the intrinsic size (preferred widths). That's why we pass the responsibility to the
// caller who does know whether the available logical width is indefinite or not.
void RenderGrid::placeItemsOnGrid(std::optional<LayoutUnit> availableLogicalWidth)
{
    unsigned autoRepeatColumns = computeAutoRepeatTracksCount(ForColumns, availableLogicalWidth);
    unsigned autoRepeatRows = computeAutoRepeatTracksCount(ForRows, availableLogicalHeightForPercentageComputation());

    autoRepeatRows = clampAutoRepeatTracks(ForRows, autoRepeatRows);
    autoRepeatColumns = clampAutoRepeatTracks(ForColumns, autoRepeatColumns);

    if (autoRepeatColumns != currentGrid().autoRepeatTracks(ForColumns) || autoRepeatRows != currentGrid().autoRepeatTracks(ForRows)) {
        currentGrid().setNeedsItemsPlacement(true);
        currentGrid().setAutoRepeatTracks(autoRepeatRows, autoRepeatColumns);
    }

    if (!currentGrid().needsItemsPlacement())
        return;

    ASSERT(!currentGrid().hasGridItems());
    populateExplicitGridAndOrderIterator();

    Vector<RenderBox*> autoMajorAxisAutoGridItems;
    Vector<RenderBox*> specifiedMajorAxisAutoGridItems;
    for (auto* child = currentGrid().orderIterator().first(); child; child = currentGrid().orderIterator().next()) {
        if (currentGrid().orderIterator().shouldSkipChild(*child))
            continue;

        // Grid items should use the grid area sizes instead of the containing block (grid container)
        // sizes, we initialize the overrides here if needed to ensure it.
        if (!child->hasOverridingContainingBlockContentLogicalWidth())
            child->setOverridingContainingBlockContentLogicalWidth(LayoutUnit());
        if (!child->hasOverridingContainingBlockContentLogicalHeight())
            child->setOverridingContainingBlockContentLogicalHeight(std::nullopt);

        GridArea area = currentGrid().gridItemArea(*child);
        if (!area.rows.isIndefinite())
            area.rows.translate(currentGrid().explicitGridStart(ForRows));
        if (!area.columns.isIndefinite())
            area.columns.translate(currentGrid().explicitGridStart(ForColumns));

        if (area.rows.isIndefinite() || area.columns.isIndefinite()) {
            currentGrid().setGridItemArea(*child, area);
            bool majorAxisDirectionIsForColumns = autoPlacementMajorAxisDirection() == ForColumns;
            if ((majorAxisDirectionIsForColumns && area.columns.isIndefinite())
                || (!majorAxisDirectionIsForColumns && area.rows.isIndefinite()))
                autoMajorAxisAutoGridItems.append(child);
            else
                specifiedMajorAxisAutoGridItems.append(child);
            continue;
        }
        currentGrid().insert(*child, { area.rows, area.columns });
    }

#if ASSERT_ENABLED
    if (currentGrid().hasGridItems()) {
        ASSERT(currentGrid().numTracks(ForRows) >= GridPositionsResolver::explicitGridRowCount(style(), currentGrid().autoRepeatTracks(ForRows)));
        ASSERT(currentGrid().numTracks(ForColumns) >= GridPositionsResolver::explicitGridColumnCount(style(), currentGrid().autoRepeatTracks(ForColumns)));
    }
#endif

    placeSpecifiedMajorAxisItemsOnGrid(specifiedMajorAxisAutoGridItems);
    placeAutoMajorAxisItemsOnGrid(autoMajorAxisAutoGridItems);

    // Compute collapsible tracks for auto-fit.
    currentGrid().setAutoRepeatEmptyColumns(computeEmptyTracksForAutoRepeat(ForColumns));
    currentGrid().setAutoRepeatEmptyRows(computeEmptyTracksForAutoRepeat(ForRows));

    currentGrid().setNeedsItemsPlacement(false);

#if ASSERT_ENABLED
    for (auto* child = currentGrid().orderIterator().first(); child; child = currentGrid().orderIterator().next()) {
        if (currentGrid().orderIterator().shouldSkipChild(*child))
            continue;

        GridArea area = currentGrid().gridItemArea(*child);
        ASSERT(area.rows.isTranslatedDefinite() && area.columns.isTranslatedDefinite());
    }
#endif
}

void RenderGrid::performGridItemsPreLayout(const GridTrackSizingAlgorithm& algorithm) const
{
    ASSERT(!algorithm.grid().needsItemsPlacement());
    // FIXME: We need a way when we are calling this during intrinsic size compuation before performing
    // the layout. Maybe using the PreLayout phase ?
    for (auto* child = firstChildBox(); child; child = child->nextSiblingBox()) {
        if (child->isOutOfFlowPositioned())
            continue;
        // Orthogonal items should be laid out in order to properly compute content-sized tracks that may depend on item's intrinsic size.
        // We also need to properly estimate its grid area size, since it may affect to the baseline shims if such item particiaptes in baseline alignment. 
        if (GridLayoutFunctions::isOrthogonalChild(*this, *child)) {
            updateGridAreaLogicalSize(*child, algorithm.estimatedGridAreaBreadthForChild(*child, ForColumns), algorithm.estimatedGridAreaBreadthForChild(*child, ForRows));
            child->layoutIfNeeded();
            continue;
        }
        // We need to layout the item to know whether it must synthesize its
        // baseline or not, which may imply a cyclic sizing dependency.
        // FIXME: Can we avoid it ?
        if (isBaselineAlignmentForChild(*child)) {
            updateGridAreaLogicalSize(*child, algorithm.estimatedGridAreaBreadthForChild(*child, ForColumns), algorithm.estimatedGridAreaBreadthForChild(*child, ForRows));
            child->layoutIfNeeded();
        }
    }
}

void RenderGrid::populateExplicitGridAndOrderIterator()
{
    OrderIteratorPopulator populator(currentGrid().orderIterator());
    unsigned explicitRowStart = 0;
    unsigned explicitColumnStart = 0;
    unsigned autoRepeatRows = currentGrid().autoRepeatTracks(ForRows);
    unsigned autoRepeatColumns = currentGrid().autoRepeatTracks(ForColumns);
    unsigned maximumRowIndex = GridPositionsResolver::explicitGridRowCount(style(), autoRepeatRows);
    unsigned maximumColumnIndex = GridPositionsResolver::explicitGridColumnCount(style(), autoRepeatColumns);

    for (RenderBox* child = firstChildBox(); child; child = child->nextSiblingBox()) {
        if (!populator.collectChild(*child))
            continue;
        
        GridSpan rowPositions = GridPositionsResolver::resolveGridPositionsFromStyle(style(), *child, ForRows, autoRepeatRows);
        if (!rowPositions.isIndefinite()) {
            explicitRowStart = std::max<int>(explicitRowStart, -rowPositions.untranslatedStartLine());
            maximumRowIndex = std::max<int>(maximumRowIndex, rowPositions.untranslatedEndLine());
        } else {
            // Grow the grid for items with a definite row span, getting the largest such span.
            unsigned spanSize = GridPositionsResolver::spanSizeForAutoPlacedItem(*child, ForRows);
            maximumRowIndex = std::max(maximumRowIndex, spanSize);
        }

        GridSpan columnPositions = GridPositionsResolver::resolveGridPositionsFromStyle(style(), *child, ForColumns, autoRepeatColumns);
        if (!columnPositions.isIndefinite()) {
            explicitColumnStart = std::max<int>(explicitColumnStart, -columnPositions.untranslatedStartLine());
            maximumColumnIndex = std::max<int>(maximumColumnIndex, columnPositions.untranslatedEndLine());
        } else {
            // Grow the grid for items with a definite column span, getting the largest such span.
            unsigned spanSize = GridPositionsResolver::spanSizeForAutoPlacedItem(*child, ForColumns);
            maximumColumnIndex = std::max(maximumColumnIndex, spanSize);
        }

        currentGrid().setGridItemArea(*child, { rowPositions, columnPositions });
    }

    currentGrid().setExplicitGridStart(explicitRowStart, explicitColumnStart);
    currentGrid().ensureGridSize(maximumRowIndex + explicitRowStart, maximumColumnIndex + explicitColumnStart);
}

std::unique_ptr<GridArea> RenderGrid::createEmptyGridAreaAtSpecifiedPositionsOutsideGrid(const RenderBox& gridItem, GridTrackSizingDirection specifiedDirection, const GridSpan& specifiedPositions) const
{
    GridTrackSizingDirection crossDirection = specifiedDirection == ForColumns ? ForRows : ForColumns;
    const unsigned endOfCrossDirection = currentGrid().numTracks(crossDirection);
    unsigned crossDirectionSpanSize = GridPositionsResolver::spanSizeForAutoPlacedItem(gridItem, crossDirection);
    GridSpan crossDirectionPositions = GridSpan::translatedDefiniteGridSpan(endOfCrossDirection, endOfCrossDirection + crossDirectionSpanSize);
    return makeUnique<GridArea>(specifiedDirection == ForColumns ? crossDirectionPositions : specifiedPositions, specifiedDirection == ForColumns ? specifiedPositions : crossDirectionPositions);
}

void RenderGrid::placeSpecifiedMajorAxisItemsOnGrid(const Vector<RenderBox*>& autoGridItems)
{
    bool isForColumns = autoPlacementMajorAxisDirection() == ForColumns;
    bool isGridAutoFlowDense = style().isGridAutoFlowAlgorithmDense();

    // Mapping between the major axis tracks (rows or columns) and the last auto-placed item's position inserted on
    // that track. This is needed to implement "sparse" packing for items locked to a given track.
    // See http://dev.w3.org/csswg/css-grid/#auto-placement-algorithm
    HashMap<unsigned, unsigned, DefaultHash<unsigned>, WTF::UnsignedWithZeroKeyHashTraits<unsigned>> minorAxisCursors;

    for (auto& autoGridItem : autoGridItems) {
        GridSpan majorAxisPositions = currentGrid().gridItemSpan(*autoGridItem, autoPlacementMajorAxisDirection());
        ASSERT(majorAxisPositions.isTranslatedDefinite());
        ASSERT(currentGrid().gridItemSpan(*autoGridItem, autoPlacementMinorAxisDirection()).isIndefinite());
        unsigned minorAxisSpanSize = GridPositionsResolver::spanSizeForAutoPlacedItem(*autoGridItem, autoPlacementMinorAxisDirection());
        unsigned majorAxisInitialPosition = majorAxisPositions.startLine();

        GridIterator iterator(currentGrid(), autoPlacementMajorAxisDirection(), majorAxisPositions.startLine(), isGridAutoFlowDense ? 0 : minorAxisCursors.get(majorAxisInitialPosition));
        std::unique_ptr<GridArea> emptyGridArea = iterator.nextEmptyGridArea(majorAxisPositions.integerSpan(), minorAxisSpanSize);
        if (!emptyGridArea)
            emptyGridArea = createEmptyGridAreaAtSpecifiedPositionsOutsideGrid(*autoGridItem, autoPlacementMajorAxisDirection(), majorAxisPositions);

        currentGrid().insert(*autoGridItem, *emptyGridArea);

        if (!isGridAutoFlowDense)
            minorAxisCursors.set(majorAxisInitialPosition, isForColumns ? emptyGridArea->rows.startLine() : emptyGridArea->columns.startLine());
    }
}

void RenderGrid::placeAutoMajorAxisItemsOnGrid(const Vector<RenderBox*>& autoGridItems)
{
    AutoPlacementCursor autoPlacementCursor = {0, 0};
    bool isGridAutoFlowDense = style().isGridAutoFlowAlgorithmDense();

    for (auto& autoGridItem : autoGridItems) {
        placeAutoMajorAxisItemOnGrid(*autoGridItem, autoPlacementCursor);

        if (isGridAutoFlowDense) {
            autoPlacementCursor.first = 0;
            autoPlacementCursor.second = 0;
        }
    }
}

void RenderGrid::placeAutoMajorAxisItemOnGrid(RenderBox& gridItem, AutoPlacementCursor& autoPlacementCursor)
{
    ASSERT(currentGrid().gridItemSpan(gridItem, autoPlacementMajorAxisDirection()).isIndefinite());
    unsigned majorAxisSpanSize = GridPositionsResolver::spanSizeForAutoPlacedItem(gridItem, autoPlacementMajorAxisDirection());

    const unsigned endOfMajorAxis = currentGrid().numTracks(autoPlacementMajorAxisDirection());
    unsigned majorAxisAutoPlacementCursor = autoPlacementMajorAxisDirection() == ForColumns ? autoPlacementCursor.second : autoPlacementCursor.first;
    unsigned minorAxisAutoPlacementCursor = autoPlacementMajorAxisDirection() == ForColumns ? autoPlacementCursor.first : autoPlacementCursor.second;

    std::unique_ptr<GridArea> emptyGridArea;
    GridSpan minorAxisPositions = currentGrid().gridItemSpan(gridItem, autoPlacementMinorAxisDirection());
    if (minorAxisPositions.isTranslatedDefinite()) {
        // Move to the next track in major axis if initial position in minor axis is before auto-placement cursor.
        if (minorAxisPositions.startLine() < minorAxisAutoPlacementCursor)
            majorAxisAutoPlacementCursor++;

        if (majorAxisAutoPlacementCursor < endOfMajorAxis) {
            GridIterator iterator(currentGrid(), autoPlacementMinorAxisDirection(), minorAxisPositions.startLine(), majorAxisAutoPlacementCursor);
            emptyGridArea = iterator.nextEmptyGridArea(minorAxisPositions.integerSpan(), majorAxisSpanSize);
        }

        if (!emptyGridArea)
            emptyGridArea = createEmptyGridAreaAtSpecifiedPositionsOutsideGrid(gridItem, autoPlacementMinorAxisDirection(), minorAxisPositions);
    } else {
        unsigned minorAxisSpanSize = GridPositionsResolver::spanSizeForAutoPlacedItem(gridItem, autoPlacementMinorAxisDirection());

        for (unsigned majorAxisIndex = majorAxisAutoPlacementCursor; majorAxisIndex < endOfMajorAxis; ++majorAxisIndex) {
            GridIterator iterator(currentGrid(), autoPlacementMajorAxisDirection(), majorAxisIndex, minorAxisAutoPlacementCursor);
            emptyGridArea = iterator.nextEmptyGridArea(majorAxisSpanSize, minorAxisSpanSize);

            if (emptyGridArea) {
                // Check that it fits in the minor axis direction, as we shouldn't grow in that direction here (it was already managed in populateExplicitGridAndOrderIterator()).
                unsigned minorAxisFinalPositionIndex = autoPlacementMinorAxisDirection() == ForColumns ? emptyGridArea->columns.endLine() : emptyGridArea->rows.endLine();
                const unsigned endOfMinorAxis = currentGrid().numTracks(autoPlacementMinorAxisDirection());
                if (minorAxisFinalPositionIndex <= endOfMinorAxis)
                    break;

                // Discard empty grid area as it does not fit in the minor axis direction.
                // We don't need to create a new empty grid area yet as we might find a valid one in the next iteration.
                emptyGridArea = nullptr;
            }

            // As we're moving to the next track in the major axis we should reset the auto-placement cursor in the minor axis.
            minorAxisAutoPlacementCursor = 0;
        }

        if (!emptyGridArea)
            emptyGridArea = createEmptyGridAreaAtSpecifiedPositionsOutsideGrid(gridItem, autoPlacementMinorAxisDirection(), GridSpan::translatedDefiniteGridSpan(0, minorAxisSpanSize));
    }

    currentGrid().insert(gridItem, *emptyGridArea);
    autoPlacementCursor.first = emptyGridArea->rows.startLine();
    autoPlacementCursor.second = emptyGridArea->columns.startLine();
}

GridTrackSizingDirection RenderGrid::autoPlacementMajorAxisDirection() const
{
    return style().isGridAutoFlowDirectionColumn() ? ForColumns : ForRows;
}

GridTrackSizingDirection RenderGrid::autoPlacementMinorAxisDirection() const
{
    return style().isGridAutoFlowDirectionColumn() ? ForRows : ForColumns;
}

void RenderGrid::dirtyGrid()
{
    if (currentGrid().needsItemsPlacement())
        return;

    currentGrid().setNeedsItemsPlacement(true);
}

Vector<LayoutUnit> RenderGrid::trackSizesForComputedStyle(GridTrackSizingDirection direction) const
{
    bool isRowAxis = direction == ForColumns;
    auto& positions = isRowAxis ? m_columnPositions : m_rowPositions;
    size_t numPositions = positions.size();
    LayoutUnit offsetBetweenTracks = isRowAxis ? m_offsetBetweenColumns.distributionOffset : m_offsetBetweenRows.distributionOffset;

    Vector<LayoutUnit> tracks;
    if (numPositions < 2)
        return tracks;

    ASSERT(!currentGrid().needsItemsPlacement());
    bool hasCollapsedTracks = currentGrid().hasAutoRepeatEmptyTracks(direction);
    LayoutUnit gap = !hasCollapsedTracks ? gridGap(direction) : 0_lu;
    tracks.reserveCapacity(numPositions - 1);
    for (size_t i = 0; i < numPositions - 2; ++i)
        tracks.append(positions[i + 1] - positions[i] - offsetBetweenTracks - gap);
    tracks.append(positions[numPositions - 1] - positions[numPositions - 2]);

    if (!hasCollapsedTracks)
        return tracks;

    size_t remainingEmptyTracks = currentGrid().autoRepeatEmptyTracks(direction)->size();
    size_t lastLine = tracks.size();
    gap = gridGap(direction);
    for (size_t i = 1; i < lastLine; ++i) {
        if (currentGrid().isEmptyAutoRepeatTrack(direction, i - 1))
            --remainingEmptyTracks;
        else {
            // Remove the gap between consecutive non empty tracks. Remove it also just once for an
            // arbitrary number of empty tracks between two non empty ones.
            bool allRemainingTracksAreEmpty = remainingEmptyTracks == (lastLine - i);
            if (!allRemainingTracksAreEmpty || !currentGrid().isEmptyAutoRepeatTrack(direction, i))
                tracks[i - 1] -= gap;
        }
    }

    return tracks;
}

static const StyleContentAlignmentData& contentAlignmentNormalBehaviorGrid()
{
    static const StyleContentAlignmentData normalBehavior = {ContentPosition::Normal, ContentDistribution::Stretch};
    return normalBehavior;
}

static bool overrideSizeChanged(const RenderBox& child, GridTrackSizingDirection direction, std::optional<LayoutUnit> width, std::optional<LayoutUnit> height)
{
    if (direction == ForColumns)
        return !child.hasOverridingContainingBlockContentLogicalWidth() || child.overridingContainingBlockContentLogicalWidth() != width;
    return !child.hasOverridingContainingBlockContentLogicalHeight() || child.overridingContainingBlockContentLogicalHeight() != height;
}

static bool hasRelativeBlockAxisSize(const RenderGrid& grid, const RenderBox& child)
{
    return GridLayoutFunctions::isOrthogonalChild(grid, child) ? child.hasRelativeLogicalWidth() || child.style().logicalWidth().isAuto() : child.hasRelativeLogicalHeight();
}

void RenderGrid::updateGridAreaLogicalSize(RenderBox& child, std::optional<LayoutUnit> width, std::optional<LayoutUnit> height) const
{
    // Because the grid area cannot be styled, we don't need to adjust
    // the grid breadth to account for 'box-sizing'.
    bool gridAreaWidthChanged = overrideSizeChanged(child, ForColumns, width, height);
    bool gridAreaHeightChanged = overrideSizeChanged(child, ForRows, width, height);
    if (gridAreaWidthChanged || (gridAreaHeightChanged && hasRelativeBlockAxisSize(*this, child)))
        child.setNeedsLayout(MarkOnlyThis);

    child.setOverridingContainingBlockContentLogicalWidth(width);
    child.setOverridingContainingBlockContentLogicalHeight(height);
}

void RenderGrid::updateGridAreaForAspectRatioItems(const Vector<RenderBox*>& autoGridItems)
{
    populateGridPositionsForDirection(ForColumns);
    populateGridPositionsForDirection(ForRows);

    for (auto& autoGridItem : autoGridItems) {
        updateGridAreaLogicalSize(*autoGridItem, gridAreaBreadthForChildIncludingAlignmentOffsets(*autoGridItem, ForColumns), gridAreaBreadthForChildIncludingAlignmentOffsets(*autoGridItem, ForRows));
        // For an item wtih aspect-ratio, if it has stretch alignment that stretches to the definite row, we also need to transfer the size before laying out the grid item.
        if (autoGridItem->hasStretchedLogicalHeight())
            applyStretchAlignmentToChildIfNeeded(*autoGridItem);
    }
}

void RenderGrid::layoutGridItems()
{
    populateGridPositionsForDirection(ForColumns);
    populateGridPositionsForDirection(ForRows);

    for (RenderBox* child = firstChildBox(); child; child = child->nextSiblingBox()) {
        
        if (currentGrid().orderIterator().shouldSkipChild(*child)) {
            if (child->isOutOfFlowPositioned())
                prepareChildForPositionedLayout(*child);
            continue;
        }

        // Setting the definite grid area's sizes. It may imply that the
        // item must perform a layout if its area differs from the one
        // used during the track sizing algorithm.
        updateGridAreaLogicalSize(*child, gridAreaBreadthForChildIncludingAlignmentOffsets(*child, ForColumns), gridAreaBreadthForChildIncludingAlignmentOffsets(*child, ForRows));

        LayoutRect oldChildRect = child->frameRect();

        // Stretching logic might force a child layout, so we need to run it before the layoutIfNeeded
        // call to avoid unnecessary relayouts. This might imply that child margins, needed to correctly
        // determine the available space before stretching, are not set yet.
        applyStretchAlignmentToChildIfNeeded(*child);

        child->layoutIfNeeded();

        // We need pending layouts to be done in order to compute auto-margins properly.
        updateAutoMarginsInColumnAxisIfNeeded(*child);
        updateAutoMarginsInRowAxisIfNeeded(*child);

        setLogicalPositionForChild(*child);

        // If the child moved, we have to repaint it as well as any floating/positioned
        // descendants. An exception is if we need a layout. In this case, we know we're going to
        // repaint ourselves (and the child) anyway.
        if (!selfNeedsLayout() && child->checkForRepaintDuringLayout())
            child->repaintDuringLayoutIfMoved(oldChildRect);
    }
}

void RenderGrid::prepareChildForPositionedLayout(RenderBox& child)
{
    ASSERT(child.isOutOfFlowPositioned());
    child.containingBlock()->insertPositionedObject(child);

    RenderLayer* childLayer = child.layer();
    // Static position of a positioned child should use the content-box (https://drafts.csswg.org/css-grid/#static-position).
    childLayer->setStaticInlinePosition(borderAndPaddingStart());
    childLayer->setStaticBlockPosition(borderAndPaddingBefore());
}

bool RenderGrid::hasStaticPositionForChild(const RenderBox& child, GridTrackSizingDirection direction) const
{
    return direction == ForColumns ? child.style().hasStaticInlinePosition(isHorizontalWritingMode()) : child.style().hasStaticBlockPosition(isHorizontalWritingMode());
}

void RenderGrid::layoutPositionedObject(RenderBox& child, bool relayoutChildren, bool fixedPositionObjectsOnly)
{
    LayoutUnit columnBreadth = gridAreaBreadthForOutOfFlowChild(child, ForColumns);
    LayoutUnit rowBreadth = gridAreaBreadthForOutOfFlowChild(child, ForRows);

    child.setOverridingContainingBlockContentLogicalWidth(columnBreadth);
    child.setOverridingContainingBlockContentLogicalHeight(rowBreadth);

    // Mark for layout as we're resetting the position before and we relay in generic layout logic
    // for positioned items in order to get the offsets properly resolved.
    child.setChildNeedsLayout(MarkOnlyThis);

    RenderBlock::layoutPositionedObject(child, relayoutChildren, fixedPositionObjectsOnly);

    setLogicalOffsetForChild(child, ForColumns);
    setLogicalOffsetForChild(child, ForRows);
}

LayoutUnit RenderGrid::gridAreaBreadthForChildIncludingAlignmentOffsets(const RenderBox& child, GridTrackSizingDirection direction) const
{
    // We need the cached value when available because Content Distribution alignment properties
    // may have some influence in the final grid area breadth.
    const auto& tracks = m_trackSizingAlgorithm.tracks(direction);
    const auto& span = currentGrid().gridItemSpan(child, direction);
    const auto& linePositions = (direction == ForColumns) ? m_columnPositions : m_rowPositions;

    LayoutUnit initialTrackPosition = linePositions[span.startLine()];
    LayoutUnit finalTrackPosition = linePositions[span.endLine() - 1];

    // Track Positions vector stores the 'start' grid line of each track, so we have to add last track's baseSize.
    return finalTrackPosition - initialTrackPosition + tracks[span.endLine() - 1].baseSize();
}

void RenderGrid::populateGridPositionsForDirection(GridTrackSizingDirection direction)
{
    // Since we add alignment offsets and track gutters, grid lines are not always adjacent. Hence we will have to
    // assume from now on that we just store positions of the initial grid lines of each track,
    // except the last one, which is the only one considered as a final grid line of a track.

    // The grid container's frame elements (border, padding and <content-position> offset) are sensible to the
    // inline-axis flow direction. However, column lines positions are 'direction' unaware. This simplification
    // allows us to use the same indexes to identify the columns independently on the inline-axis direction.
    bool isRowAxis = direction == ForColumns;
    auto& tracks = m_trackSizingAlgorithm.tracks(direction);
    unsigned numberOfTracks = tracks.size();
    unsigned numberOfLines = numberOfTracks + 1;
    unsigned lastLine = numberOfLines - 1;
    bool hasCollapsedTracks = currentGrid().hasAutoRepeatEmptyTracks(direction);
    size_t numberOfCollapsedTracks = hasCollapsedTracks ? currentGrid().autoRepeatEmptyTracks(direction)->size() : 0;
    const auto& offset = direction == ForColumns ? m_offsetBetweenColumns : m_offsetBetweenRows;
    auto& positions = isRowAxis ? m_columnPositions : m_rowPositions;
    positions.resize(numberOfLines);

    auto borderAndPadding = isRowAxis ? borderAndPaddingLogicalLeft() : borderAndPaddingBefore();
#if !PLATFORM(IOS_FAMILY)
    // FIXME: Ideally scrollbarLogicalWidth() should return zero in iOS so we don't need this
    // (see bug https://webkit.org/b/191857).
    // If we are in horizontal writing mode and RTL direction the scrollbar is painted on the left,
    // so we need to take into account when computing the position of the columns.
    if (isRowAxis && style().isHorizontalWritingMode() && !style().isLeftToRightDirection())
        borderAndPadding += scrollbarLogicalWidth();
#endif

    positions[0] = borderAndPadding + offset.positionOffset;
    if (numberOfLines > 1) {
        // If we have collapsed tracks we just ignore gaps here and add them later as we might not
        // compute the gap between two consecutive tracks without examining the surrounding ones.
        LayoutUnit gap = !hasCollapsedTracks ? gridGap(direction) : 0_lu;
        unsigned nextToLastLine = numberOfLines - 2;
        for (unsigned i = 0; i < nextToLastLine; ++i)
            positions[i + 1] = positions[i] + offset.distributionOffset + tracks[i].baseSize() + gap;
        positions[lastLine] = positions[nextToLastLine] + tracks[nextToLastLine].baseSize();

        // Adjust collapsed gaps. Collapsed tracks cause the surrounding gutters to collapse (they
        // coincide exactly) except on the edges of the grid where they become 0.
        if (hasCollapsedTracks) {
            gap = gridGap(direction);
            unsigned remainingEmptyTracks = numberOfCollapsedTracks;
            LayoutUnit offsetAccumulator;
            LayoutUnit gapAccumulator;
            for (unsigned i = 1; i < lastLine; ++i) {
                if (currentGrid().isEmptyAutoRepeatTrack(direction, i - 1)) {
                    --remainingEmptyTracks;
                    offsetAccumulator += offset.distributionOffset;
                } else {
                    // Add gap between consecutive non empty tracks. Add it also just once for an
                    // arbitrary number of empty tracks between two non empty ones.
                    bool allRemainingTracksAreEmpty = remainingEmptyTracks == (lastLine - i);
                    if (!allRemainingTracksAreEmpty || !currentGrid().isEmptyAutoRepeatTrack(direction, i))
                        gapAccumulator += gap;
                }
                positions[i] += gapAccumulator - offsetAccumulator;
            }
            positions[lastLine] += gapAccumulator - offsetAccumulator;
        }
    }
}

static LayoutUnit computeOverflowAlignmentOffset(OverflowAlignment overflow, LayoutUnit trackSize, LayoutUnit childSize)
{
    LayoutUnit offset = trackSize - childSize;
    switch (overflow) {
    case OverflowAlignment::Safe:
        // If overflow is 'safe', we have to make sure we don't overflow the 'start'
        // edge (potentially cause some data loss as the overflow is unreachable).
        return std::max<LayoutUnit>(0, offset);
    case OverflowAlignment::Unsafe:
    case OverflowAlignment::Default:
        // If we overflow our alignment container and overflow is 'true' (default), we
        // ignore the overflow and just return the value regardless (which may cause data
        // loss as we overflow the 'start' edge).
        return offset;
    }

    ASSERT_NOT_REACHED();
    return 0;
}

LayoutUnit RenderGrid::availableAlignmentSpaceForChildBeforeStretching(LayoutUnit gridAreaBreadthForChild, const RenderBox& child) const
{
    // Because we want to avoid multiple layouts, stretching logic might be performed before
    // children are laid out, so we can't use the child cached values. Hence, we need to
    // compute margins in order to determine the available height before stretching.
    GridTrackSizingDirection childBlockFlowDirection = GridLayoutFunctions::flowAwareDirectionForChild(*this, child, ForRows);
    return std::max(0_lu, gridAreaBreadthForChild - GridLayoutFunctions::marginLogicalSizeForChild(*this, childBlockFlowDirection, child));
}

StyleSelfAlignmentData RenderGrid::alignSelfForChild(const RenderBox& child, StretchingMode stretchingMode, const RenderStyle* gridStyle) const
{
    if (!gridStyle)
        gridStyle = &style();
    auto normalBehavior = stretchingMode == StretchingMode::Any ? selfAlignmentNormalBehavior(&child) : ItemPosition::Normal;
    return child.style().resolvedAlignSelf(gridStyle, normalBehavior);
}

StyleSelfAlignmentData RenderGrid::justifySelfForChild(const RenderBox& child, StretchingMode stretchingMode, const RenderStyle* gridStyle) const
{
    if (!gridStyle)
        gridStyle = &style();
    auto normalBehavior = stretchingMode == StretchingMode::Any ? selfAlignmentNormalBehavior(&child) : ItemPosition::Normal;
    return child.style().resolvedJustifySelf(gridStyle, normalBehavior);
}

bool RenderGrid::aspectRatioPrefersInline(const RenderBox& child, bool blockFlowIsColumnAxis)
{
    if (!child.style().hasAspectRatio())
        return false;
    bool hasExplicitInlineStretch = justifySelfForChild(child, StretchingMode::Explicit).position() == ItemPosition::Stretch;
    bool hasExplicitBlockStretch = alignSelfForChild(child, StretchingMode::Explicit).position() == ItemPosition::Stretch;
    if (!blockFlowIsColumnAxis)
        std::swap(hasExplicitInlineStretch, hasExplicitBlockStretch);
    return !hasExplicitBlockStretch;
}

// FIXME: This logic is shared by RenderFlexibleBox, so it should be moved to RenderBox.
void RenderGrid::applyStretchAlignmentToChildIfNeeded(RenderBox& child)
{
    ASSERT(child.overridingContainingBlockContentLogicalHeight());

    // We clear height override values because we will decide now whether it's allowed or
    // not, evaluating the conditions which might have changed since the old values were set.
    child.clearOverridingLogicalHeight();

    GridTrackSizingDirection childBlockDirection = GridLayoutFunctions::flowAwareDirectionForChild(*this, child, ForRows);
    bool blockFlowIsColumnAxis = childBlockDirection == ForRows;
    bool allowedToStretchChildBlockSize = blockFlowIsColumnAxis ? allowedToStretchChildAlongColumnAxis(child) : allowedToStretchChildAlongRowAxis(child);
    if (allowedToStretchChildBlockSize && !aspectRatioPrefersInline(child, blockFlowIsColumnAxis)) {
        LayoutUnit stretchedLogicalHeight = availableAlignmentSpaceForChildBeforeStretching(GridLayoutFunctions::overridingContainingBlockContentSizeForChild(child, childBlockDirection).value(), child);
        LayoutUnit desiredLogicalHeight = child.constrainLogicalHeightByMinMax(stretchedLogicalHeight, std::nullopt);
        child.setOverridingLogicalHeight(desiredLogicalHeight);

        // Checking the logical-height of a child isn't enough. Setting an override logical-height
        // changes the definiteness, resulting in percentages to resolve differently.
        //
        // FIXME: Can avoid laying out here in some cases. See https://webkit.org/b/87905.
        if (desiredLogicalHeight != child.logicalHeight() || (is<RenderBlock>(child) && downcast<RenderBlock>(child).hasPercentHeightDescendants())) {
            child.setLogicalHeight(0_lu);
            child.setNeedsLayout(MarkOnlyThis);
        }
    }
}

// FIXME: This logic is shared by RenderFlexibleBox, so it should be moved to RenderBox.
bool RenderGrid::hasAutoMarginsInColumnAxis(const RenderBox& child) const
{
    if (isHorizontalWritingMode())
        return child.style().marginTop().isAuto() || child.style().marginBottom().isAuto();
    return child.style().marginLeft().isAuto() || child.style().marginRight().isAuto();
}

// FIXME: This logic is shared by RenderFlexibleBox, so it should be moved to RenderBox.
bool RenderGrid::hasAutoMarginsInRowAxis(const RenderBox& child) const
{
    if (isHorizontalWritingMode())
        return child.style().marginLeft().isAuto() || child.style().marginRight().isAuto();
    return child.style().marginTop().isAuto() || child.style().marginBottom().isAuto();
}

// FIXME: This logic is shared by RenderFlexibleBox, so it should be moved to RenderBox.
void RenderGrid::updateAutoMarginsInRowAxisIfNeeded(RenderBox& child)
{
    ASSERT(!child.isOutOfFlowPositioned());

    const RenderStyle& parentStyle = style();
    Length marginStart = child.style().marginStartUsing(&parentStyle);
    Length marginEnd = child.style().marginEndUsing(&parentStyle);
    LayoutUnit marginLogicalWidth;
    // We should only consider computed margins if their specified value isn't
    // 'auto', since such computed value may come from a previous layout and may
    // be incorrect now.
    if (!marginStart.isAuto())
        marginLogicalWidth += child.marginStart();
    if (!marginEnd.isAuto())
        marginLogicalWidth += child.marginEnd();

    LayoutUnit availableAlignmentSpace = child.overridingContainingBlockContentLogicalWidth().value() - child.logicalWidth() - marginLogicalWidth;
    if (availableAlignmentSpace <= 0)
        return;

    if (marginStart.isAuto() && marginEnd.isAuto()) {
        child.setMarginStart(availableAlignmentSpace / 2, &parentStyle);
        child.setMarginEnd(availableAlignmentSpace / 2, &parentStyle);
    } else if (marginStart.isAuto()) {
        child.setMarginStart(availableAlignmentSpace, &parentStyle);
    } else if (marginEnd.isAuto()) {
        child.setMarginEnd(availableAlignmentSpace, &parentStyle);
    }
}

// FIXME: This logic is shared by RenderFlexibleBox, so it should be moved to RenderBox.
void RenderGrid::updateAutoMarginsInColumnAxisIfNeeded(RenderBox& child)
{
    ASSERT(!child.isOutOfFlowPositioned());

    const RenderStyle& parentStyle = style();
    Length marginBefore = child.style().marginBeforeUsing(&parentStyle);
    Length marginAfter = child.style().marginAfterUsing(&parentStyle);
    LayoutUnit marginLogicalHeight;
    // We should only consider computed margins if their specified value isn't
    // 'auto', since such computed value may come from a previous layout and may
    // be incorrect now.
    if (!marginBefore.isAuto())
        marginLogicalHeight += child.marginBefore();
    if (!marginAfter.isAuto())
        marginLogicalHeight += child.marginAfter();

    LayoutUnit availableAlignmentSpace = child.overridingContainingBlockContentLogicalHeight().value() - child.logicalHeight() - marginLogicalHeight;
    if (availableAlignmentSpace <= 0)
        return;

    if (marginBefore.isAuto() && marginAfter.isAuto()) {
        child.setMarginBefore(availableAlignmentSpace / 2, &parentStyle);
        child.setMarginAfter(availableAlignmentSpace / 2, &parentStyle);
    } else if (marginBefore.isAuto()) {
        child.setMarginBefore(availableAlignmentSpace, &parentStyle);
    } else if (marginAfter.isAuto()) {
        child.setMarginAfter(availableAlignmentSpace, &parentStyle);
    }
}

bool RenderGrid::isBaselineAlignmentForChild(const RenderBox& child) const
{
    return isBaselineAlignmentForChild(child, GridRowAxis) || isBaselineAlignmentForChild(child, GridColumnAxis);
}

bool RenderGrid::isBaselineAlignmentForChild(const RenderBox& child, GridAxis baselineAxis, AllowedBaseLine allowed) const
{
    if (child.isOutOfFlowPositioned())
        return false;
    ItemPosition align = selfAlignmentForChild(baselineAxis, child).position();
    bool hasAutoMargins = baselineAxis == GridColumnAxis ? hasAutoMarginsInColumnAxis(child) : hasAutoMarginsInRowAxis(child);
    bool isBaseline = allowed == FirstLine ? isFirstBaselinePosition(align) : isBaselinePosition(align);
    return isBaseline && !hasAutoMargins;
}

// FIXME: This logic is shared by RenderFlexibleBox, so it might be refactored somehow.
LayoutUnit RenderGrid::baselinePosition(FontBaseline, bool, LineDirectionMode direction, LinePositionMode mode) const
{
    ASSERT_UNUSED(mode, mode == PositionOnContainingLine);
    auto baseline = firstLineBaseline();
    if (!baseline)
        return synthesizedBaselineFromBorderBox(*this, direction) + marginLogicalHeight();

    return baseline.value() + (direction == HorizontalLine ? marginTop() : marginRight()).toInt();
}

std::optional<LayoutUnit> RenderGrid::firstLineBaseline() const
{
    if (isWritingModeRoot() || !currentGrid().hasGridItems() || shouldApplyLayoutContainment(*this))
        return std::nullopt;

    const RenderBox* baselineChild = nullptr;
    // Finding the first grid item in grid order.
    unsigned numColumns = currentGrid().numTracks(ForColumns);
    for (size_t column = 0; column < numColumns; column++) {
        for (auto& child : currentGrid().cell(0, column)) {
            ASSERT(child.get());
            // If an item participates in baseline alignment, we select such item.
            if (isBaselineAlignmentForChild(*child, GridColumnAxis, FirstLine)) {
                // FIXME: self-baseline and content-baseline alignment not implemented yet.
                baselineChild = child.get();
                break;
            }
            if (!baselineChild)
                baselineChild = child.get();
        }
    }

    if (!baselineChild)
        return std::nullopt;

    auto baseline = GridLayoutFunctions::isOrthogonalChild(*this, *baselineChild) ? std::nullopt : baselineChild->firstLineBaseline();
    // We take border-box's bottom if no valid baseline.
    if (!baseline) {
        // FIXME: We should pass |direction| into firstLineBaseline and stop bailing out if we're a writing
        // mode root. This would also fix some cases where the grid is orthogonal to its container.
        LineDirectionMode direction = isHorizontalWritingMode() ? HorizontalLine : VerticalLine;
        return synthesizedBaselineFromBorderBox(*baselineChild, direction) + logicalTopForChild(*baselineChild);
    }
    return baseline.value() + baselineChild->logicalTop().toInt();
}

std::optional<LayoutUnit> RenderGrid::inlineBlockBaseline(LineDirectionMode) const
{
    return firstLineBaseline();
}

LayoutUnit RenderGrid::columnAxisBaselineOffsetForChild(const RenderBox& child) const
{
    return m_trackSizingAlgorithm.baselineOffsetForChild(child, GridColumnAxis);
}

LayoutUnit RenderGrid::rowAxisBaselineOffsetForChild(const RenderBox& child) const
{
    return m_trackSizingAlgorithm.baselineOffsetForChild(child, GridRowAxis);
}

GridAxisPosition RenderGrid::columnAxisPositionForChild(const RenderBox& child) const
{
    bool hasSameWritingMode = child.style().writingMode() == style().writingMode();
    bool childIsLTR = child.style().isLeftToRightDirection();
    if (child.isOutOfFlowPositioned() && !hasStaticPositionForChild(child, ForRows))
        return GridAxisStart;

    switch (alignSelfForChild(child).position()) {
    case ItemPosition::SelfStart:
        // FIXME: Should we implement this logic in a generic utility function ?
        // Aligns the alignment subject to be flush with the edge of the alignment container
        // corresponding to the alignment subject's 'start' side in the column axis.
        if (GridLayoutFunctions::isOrthogonalChild(*this, child)) {
            // If orthogonal writing-modes, self-start will be based on the child's inline-axis
            // direction (inline-start), because it's the one parallel to the column axis.
            if (style().isFlippedBlocksWritingMode())
                return childIsLTR ? GridAxisEnd : GridAxisStart;
            return childIsLTR ? GridAxisStart : GridAxisEnd;
        }
        // self-start is based on the child's block-flow direction. That's why we need to check against the grid container's block-flow direction.
        return hasSameWritingMode ? GridAxisStart : GridAxisEnd;
    case ItemPosition::SelfEnd:
        // FIXME: Should we implement this logic in a generic utility function ?
        // Aligns the alignment subject to be flush with the edge of the alignment container
        // corresponding to the alignment subject's 'end' side in the column axis.
        if (GridLayoutFunctions::isOrthogonalChild(*this, child)) {
            // If orthogonal writing-modes, self-end will be based on the child's inline-axis
            // direction, (inline-end) because it's the one parallel to the column axis.
            if (style().isFlippedBlocksWritingMode())
                return childIsLTR ? GridAxisStart : GridAxisEnd;
            return childIsLTR ? GridAxisEnd : GridAxisStart;
        }
        // self-end is based on the child's block-flow direction. That's why we need to check against the grid container's block-flow direction.
        return hasSameWritingMode ? GridAxisEnd : GridAxisStart;
    case ItemPosition::Left:
        // Aligns the alignment subject to be flush with the alignment container's 'line-left' edge.
        // The alignment axis (column axis) is always orthogonal to the inline axis, hence this value behaves as 'start'.
        return GridAxisStart;
    case ItemPosition::Right:
        // Aligns the alignment subject to be flush with the alignment container's 'line-right' edge.
        // The alignment axis (column axis) is always orthogonal to the inline axis, hence this value behaves as 'start'.
        return GridAxisStart;
    case ItemPosition::Center:
        return GridAxisCenter;
    case ItemPosition::FlexStart: // Only used in flex layout, otherwise equivalent to 'start'.
        // Aligns the alignment subject to be flush with the alignment container's 'start' edge (block-start) in the column axis.
    case ItemPosition::Start:
        return GridAxisStart;
    case ItemPosition::FlexEnd: // Only used in flex layout, otherwise equivalent to 'end'.
        // Aligns the alignment subject to be flush with the alignment container's 'end' edge (block-end) in the column axis.
    case ItemPosition::End:
        return GridAxisEnd;
    case ItemPosition::Stretch:
        return GridAxisStart;
    case ItemPosition::Baseline:
    case ItemPosition::LastBaseline:
        // FIXME: Implement the previous values. For now, we always 'start' align the child.
        return GridAxisStart;
    case ItemPosition::Legacy:
    case ItemPosition::Auto:
    case ItemPosition::Normal:
        break;
    }

    ASSERT_NOT_REACHED();
    return GridAxisStart;
}

GridAxisPosition RenderGrid::rowAxisPositionForChild(const RenderBox& child) const
{
    bool hasSameDirection = child.style().direction() == style().direction();
    bool gridIsLTR = style().isLeftToRightDirection();
    if (child.isOutOfFlowPositioned() && !hasStaticPositionForChild(child, ForColumns))
        return GridAxisStart;

    switch (justifySelfForChild(child).position()) {
    case ItemPosition::SelfStart:
        // FIXME: Should we implement this logic in a generic utility function ?
        // Aligns the alignment subject to be flush with the edge of the alignment container
        // corresponding to the alignment subject's 'start' side in the row axis.
        if (GridLayoutFunctions::isOrthogonalChild(*this, child)) {
            // If orthogonal writing-modes, self-start will be based on the child's block-axis
            // direction, because it's the one parallel to the row axis.
            if (child.style().isFlippedBlocksWritingMode())
                return gridIsLTR ? GridAxisEnd : GridAxisStart;
            return gridIsLTR ? GridAxisStart : GridAxisEnd;
        }
        // self-start is based on the child's inline-flow direction. That's why we need to check against the grid container's direction.
        return hasSameDirection ? GridAxisStart : GridAxisEnd;
    case ItemPosition::SelfEnd:
        // FIXME: Should we implement this logic in a generic utility function ?
        // Aligns the alignment subject to be flush with the edge of the alignment container
        // corresponding to the alignment subject's 'end' side in the row axis.
        if (GridLayoutFunctions::isOrthogonalChild(*this, child)) {
            // If orthogonal writing-modes, self-end will be based on the child's block-axis
            // direction, because it's the one parallel to the row axis.
            if (child.style().isFlippedBlocksWritingMode())
                return gridIsLTR ? GridAxisStart : GridAxisEnd;
            return gridIsLTR ? GridAxisEnd : GridAxisStart;
        }
        // self-end is based on the child's inline-flow direction. That's why we need to check against the grid container's direction.
        return hasSameDirection ? GridAxisEnd : GridAxisStart;
    case ItemPosition::Left:
        // Aligns the alignment subject to be flush with the alignment container's 'line-left' edge.
        // We want the physical 'left' side, so we have to take account, container's inline-flow direction.
        return gridIsLTR ? GridAxisStart : GridAxisEnd;
    case ItemPosition::Right:
        // Aligns the alignment subject to be flush with the alignment container's 'line-right' edge.
        // We want the physical 'right' side, so we have to take account, container's inline-flow direction.
        return gridIsLTR ? GridAxisEnd : GridAxisStart;
    case ItemPosition::Center:
        return GridAxisCenter;
    case ItemPosition::FlexStart: // Only used in flex layout, otherwise equivalent to 'start'.
        // Aligns the alignment subject to be flush with the alignment container's 'start' edge (inline-start) in the row axis.
    case ItemPosition::Start:
        return GridAxisStart;
    case ItemPosition::FlexEnd: // Only used in flex layout, otherwise equivalent to 'end'.
        // Aligns the alignment subject to be flush with the alignment container's 'end' edge (inline-end) in the row axis.
    case ItemPosition::End:
        return GridAxisEnd;
    case ItemPosition::Stretch:
        return GridAxisStart;
    case ItemPosition::Baseline:
    case ItemPosition::LastBaseline:
        // FIXME: Implement the previous values. For now, we always 'start' align the child.
        return GridAxisStart;
    case ItemPosition::Legacy:
    case ItemPosition::Auto:
    case ItemPosition::Normal:
        break;
    }

    ASSERT_NOT_REACHED();
    return GridAxisStart;
}

LayoutUnit RenderGrid::columnAxisOffsetForChild(const RenderBox& child) const
{
    LayoutUnit startOfRow;
    LayoutUnit endOfRow;
    gridAreaPositionForChild(child, ForRows, startOfRow, endOfRow);
    LayoutUnit startPosition = startOfRow + marginBeforeForChild(child);
    if (hasAutoMarginsInColumnAxis(child))
        return startPosition;
    GridAxisPosition axisPosition = columnAxisPositionForChild(child);
    switch (axisPosition) {
    case GridAxisStart:
        return startPosition + columnAxisBaselineOffsetForChild(child);
    case GridAxisEnd:
    case GridAxisCenter: {
        LayoutUnit columnAxisChildSize = GridLayoutFunctions::isOrthogonalChild(*this, child) ? child.logicalWidth() + child.marginLogicalWidth() : child.logicalHeight() + child.marginLogicalHeight();
        auto overflow = alignSelfForChild(child).overflow();
        LayoutUnit offsetFromStartPosition = computeOverflowAlignmentOffset(overflow, endOfRow - startOfRow, columnAxisChildSize);
        return startPosition + (axisPosition == GridAxisEnd ? offsetFromStartPosition : offsetFromStartPosition / 2);
    }
    }

    ASSERT_NOT_REACHED();
    return 0;
}

LayoutUnit RenderGrid::rowAxisOffsetForChild(const RenderBox& child) const
{
    LayoutUnit startOfColumn;
    LayoutUnit endOfColumn;
    gridAreaPositionForChild(child, ForColumns, startOfColumn, endOfColumn);
    LayoutUnit startPosition = startOfColumn + marginStartForChild(child);
    if (hasAutoMarginsInRowAxis(child))
        return startPosition;
    GridAxisPosition axisPosition = rowAxisPositionForChild(child);
    switch (axisPosition) {
    case GridAxisStart:
        return startPosition + rowAxisBaselineOffsetForChild(child);
    case GridAxisEnd:
    case GridAxisCenter: {
        LayoutUnit rowAxisChildSize = GridLayoutFunctions::isOrthogonalChild(*this, child) ? child.logicalHeight() + child.marginLogicalHeight() : child.logicalWidth() + child.marginLogicalWidth();
        auto overflow = justifySelfForChild(child).overflow();
        LayoutUnit offsetFromStartPosition = computeOverflowAlignmentOffset(overflow, endOfColumn - startOfColumn, rowAxisChildSize);
        return startPosition + (axisPosition == GridAxisEnd ? offsetFromStartPosition : offsetFromStartPosition / 2);
    }
    }

    ASSERT_NOT_REACHED();
    return 0;
}

LayoutUnit RenderGrid::resolveAutoStartGridPosition(GridTrackSizingDirection direction) const
{
    if (direction == ForRows || style().isLeftToRightDirection())
        return 0_lu;

    int lastLine = numTracks(ForColumns);
    ContentPosition position = style().resolvedJustifyContentPosition(contentAlignmentNormalBehaviorGrid());
    if (position == ContentPosition::End)
        return m_columnPositions[lastLine] - clientLogicalWidth();
    if (position == ContentPosition::Start || style().resolvedJustifyContentDistribution(contentAlignmentNormalBehaviorGrid()) == ContentDistribution::Stretch)
        return m_columnPositions[0] - borderAndPaddingLogicalLeft();
    return 0_lu;
}

LayoutUnit RenderGrid::resolveAutoEndGridPosition(GridTrackSizingDirection direction) const
{
    if (direction == ForRows)
        return clientLogicalHeight();
    if (style().isLeftToRightDirection())
        return clientLogicalWidth();

    int lastLine = numTracks(ForColumns);
    ContentPosition position = style().resolvedJustifyContentPosition(contentAlignmentNormalBehaviorGrid());
    if (position == ContentPosition::End)
        return m_columnPositions[lastLine];
    if (position == ContentPosition::Start || style().resolvedJustifyContentDistribution(contentAlignmentNormalBehaviorGrid()) == ContentDistribution::Stretch)
        return m_columnPositions[0] - borderAndPaddingLogicalLeft() + clientLogicalWidth();
    return clientLogicalWidth();
}

const Grid& RenderGrid::currentGrid() const
{
    return m_grid.m_currentGrid;
}

Grid& RenderGrid::currentGrid()
{
    return m_grid.m_currentGrid;
}

LayoutUnit RenderGrid::gridAreaBreadthForOutOfFlowChild(const RenderBox& child, GridTrackSizingDirection direction)
{
    ASSERT(child.isOutOfFlowPositioned());
    bool isRowAxis = direction == ForColumns;
    GridSpan span = GridPositionsResolver::resolveGridPositionsFromStyle(style(), child, direction, autoRepeatCountForDirection(direction));
    if (span.isIndefinite())
        return isRowAxis ? clientLogicalWidth() : clientLogicalHeight();

    unsigned explicitStart = currentGrid().explicitGridStart(direction);
    int startLine = span.untranslatedStartLine() + explicitStart;
    int endLine = span.untranslatedEndLine() + explicitStart;
    int lastLine = numTracks(direction);
    GridPosition startPosition = direction == ForColumns ? child.style().gridItemColumnStart() : child.style().gridItemRowStart();
    GridPosition endPosition = direction == ForColumns ? child.style().gridItemColumnEnd() : child.style().gridItemRowEnd();

    bool startIsAuto = startPosition.isAuto() || startLine < 0 || startLine > lastLine;
    bool endIsAuto = endPosition.isAuto() || endLine < 0 || endLine > lastLine;

    if (startIsAuto && endIsAuto)
        return isRowAxis ? clientLogicalWidth() : clientLogicalHeight();

    LayoutUnit start;
    LayoutUnit end;
    auto& positions = isRowAxis ? m_columnPositions : m_rowPositions;
    auto& outOfFlowItemLine = isRowAxis ? m_outOfFlowItemColumn : m_outOfFlowItemRow;
    LayoutUnit borderEdge = isRowAxis ? (style().isLeftToRightDirection() ? borderLogicalLeft() : borderLogicalRight()) : borderBefore();
    if (startIsAuto)
        start = resolveAutoStartGridPosition(direction) + borderEdge;
    else {
        outOfFlowItemLine.set(&child, startLine);
        start = positions[startLine];
    }
    if (endIsAuto)
        end = resolveAutoEndGridPosition(direction) + borderEdge;
    else {
        end = positions[endLine];
        // These vectors store line positions including gaps, but we shouldn't consider them for the edges of the grid.
        std::optional<LayoutUnit> availableSizeForGutters = availableSpaceForGutters(direction);
        if (endLine > 0 && endLine < lastLine) {
            ASSERT(!currentGrid().needsItemsPlacement());
            end -= guttersSize(direction, endLine - 1, 2, availableSizeForGutters);
            end -= isRowAxis ? m_offsetBetweenColumns.distributionOffset : m_offsetBetweenRows.distributionOffset;
        }
    }
    return std::max(end - start, 0_lu);
}

LayoutUnit RenderGrid::logicalOffsetForOutOfFlowChild(const RenderBox& child, GridTrackSizingDirection direction, LayoutUnit trackBreadth) const
{
    ASSERT(child.isOutOfFlowPositioned());
    if (hasStaticPositionForChild(child, direction))
        return 0_lu;

    bool isRowAxis = direction == ForColumns;
    bool isFlowAwareRowAxis = GridLayoutFunctions::flowAwareDirectionForChild(*this, child, direction) == ForColumns;
    LayoutUnit childPosition = isFlowAwareRowAxis ? child.logicalLeft() : child.logicalTop();
    LayoutUnit gridBorder = isRowAxis ? borderLogicalLeft() : borderBefore();
    LayoutUnit childMargin = isRowAxis ? child.marginLogicalLeft(&style()) : child.marginBefore(&style());
    LayoutUnit offset = childPosition - gridBorder - childMargin;
    if (!isRowAxis || style().isLeftToRightDirection())
        return offset;

    LayoutUnit childBreadth = isFlowAwareRowAxis ? child.logicalWidth() + child.marginLogicalWidth() : child.logicalHeight() + child.marginLogicalHeight();
    return trackBreadth - offset - childBreadth;
}

void RenderGrid::gridAreaPositionForOutOfFlowChild(const RenderBox& child, GridTrackSizingDirection direction, LayoutUnit& start, LayoutUnit& end) const
{
    ASSERT(child.isOutOfFlowPositioned());
    ASSERT(GridLayoutFunctions::hasOverridingContainingBlockContentSizeForChild(child, direction));
    LayoutUnit trackBreadth = GridLayoutFunctions::overridingContainingBlockContentSizeForChild(child, direction).value();
    bool isRowAxis = direction == ForColumns;
    auto& outOfFlowItemLine = isRowAxis ? m_outOfFlowItemColumn : m_outOfFlowItemRow;
    start = isRowAxis ? (style().isLeftToRightDirection() ? borderLogicalLeft() : borderLogicalRight()) : borderBefore();
    if (auto line = outOfFlowItemLine.get(&child)) {
        auto& positions = isRowAxis ? m_columnPositions : m_rowPositions;
        start = positions[line.value()];
    }
    start += logicalOffsetForOutOfFlowChild(child, direction, trackBreadth);
    end = start + trackBreadth;
}

void RenderGrid::gridAreaPositionForInFlowChild(const RenderBox& child, GridTrackSizingDirection direction, LayoutUnit& start, LayoutUnit& end) const
{
    ASSERT(!child.isOutOfFlowPositioned());
    const GridSpan& span = currentGrid().gridItemSpan(child, direction);
    // FIXME (lajava): This is a common pattern, why not defining a function like
    // positions(direction) ?
    auto& positions = direction == ForColumns ? m_columnPositions : m_rowPositions;
    start = positions[span.startLine()];
    end = positions[span.endLine()];
    // The 'positions' vector includes distribution offset (because of content
    // alignment) and gutters so we need to subtract them to get the actual
    // end position for a given track (this does not have to be done for the
    // last track as there are no more positions's elements after it, nor for
    // collapsed tracks).
    if (span.endLine() < positions.size() - 1
        && !(currentGrid().hasAutoRepeatEmptyTracks(direction)
        && currentGrid().isEmptyAutoRepeatTrack(direction, span.endLine()))) {
        end -= gridGap(direction) + gridItemOffset(direction);
    }
}

void RenderGrid::gridAreaPositionForChild(const RenderBox& child, GridTrackSizingDirection direction, LayoutUnit& start, LayoutUnit& end) const
{
    if (child.isOutOfFlowPositioned())
        gridAreaPositionForOutOfFlowChild(child, direction, start, end);
    else
        gridAreaPositionForInFlowChild(child, direction, start, end);
}

ContentPosition static resolveContentDistributionFallback(ContentDistribution distribution)
{
    switch (distribution) {
    case ContentDistribution::SpaceBetween:
        return ContentPosition::Start;
    case ContentDistribution::SpaceAround:
        return ContentPosition::Center;
    case ContentDistribution::SpaceEvenly:
        return ContentPosition::Center;
    case ContentDistribution::Stretch:
        return ContentPosition::Start;
    case ContentDistribution::Default:
        return ContentPosition::Normal;
    }

    ASSERT_NOT_REACHED();
    return ContentPosition::Normal;
}

static void contentDistributionOffset(ContentAlignmentData& offset, const LayoutUnit& availableFreeSpace, ContentPosition& fallbackPosition, ContentDistribution distribution, unsigned numberOfGridTracks)
{
    if (distribution != ContentDistribution::Default && fallbackPosition == ContentPosition::Normal)
        fallbackPosition = resolveContentDistributionFallback(distribution);

    // Initialize to an invalid offset.
    offset.positionOffset = -1_lu;
    offset.distributionOffset = -1_lu;
    if (availableFreeSpace <= 0)
        return;

    LayoutUnit positionOffset;
    LayoutUnit distributionOffset;
    switch (distribution) {
    case ContentDistribution::SpaceBetween:
        if (numberOfGridTracks < 2)
            return;
        distributionOffset = availableFreeSpace / (numberOfGridTracks - 1);
        positionOffset = 0_lu;
        break;
    case ContentDistribution::SpaceAround:
        if (numberOfGridTracks < 1)
            return;
        distributionOffset = availableFreeSpace / numberOfGridTracks;
        positionOffset = distributionOffset / 2;
        break;
    case ContentDistribution::SpaceEvenly:
        distributionOffset = availableFreeSpace / (numberOfGridTracks + 1);
        positionOffset = distributionOffset;
        break;
    case ContentDistribution::Stretch:
    case ContentDistribution::Default:
        return;
    default:
        ASSERT_NOT_REACHED();
        return;
    }

    offset.positionOffset = positionOffset;
    offset.distributionOffset = distributionOffset;
}

StyleContentAlignmentData RenderGrid::contentAlignment(GridTrackSizingDirection direction) const
{
    return direction == ForColumns ? style().resolvedJustifyContent(contentAlignmentNormalBehaviorGrid()) : style().resolvedAlignContent(contentAlignmentNormalBehaviorGrid());
}

void RenderGrid::computeContentPositionAndDistributionOffset(GridTrackSizingDirection direction, const LayoutUnit& availableFreeSpace, unsigned numberOfGridTracks)
{
    bool isRowAxis = direction == ForColumns;
    auto& offset =
        isRowAxis ? m_offsetBetweenColumns : m_offsetBetweenRows;
    auto contentAlignmentData = contentAlignment(direction);
    auto position = contentAlignmentData.position();
    // If <content-distribution> value can't be applied, 'position' will become the associated
    // <content-position> fallback value.
    contentDistributionOffset(offset, availableFreeSpace, position, contentAlignmentData.distribution(), numberOfGridTracks);
    if (offset.isValid())
        return;

    if (availableFreeSpace <= 0 && contentAlignmentData.overflow() == OverflowAlignment::Safe) {
        offset.positionOffset = 0_lu;
        offset.distributionOffset = 0_lu;
        return;
    }

    LayoutUnit positionOffset;
    switch (position) {
    case ContentPosition::Left:
        ASSERT(isRowAxis);
        break;
    case ContentPosition::Right:
        ASSERT(isRowAxis);
        positionOffset = availableFreeSpace;
        break;
    case ContentPosition::Center:
        positionOffset = availableFreeSpace / 2;
        break;
    case ContentPosition::FlexEnd: // Only used in flex layout, for other layout, it's equivalent to 'end'.
    case ContentPosition::End:
        if (isRowAxis)
            positionOffset = style().isLeftToRightDirection() ? availableFreeSpace : 0_lu;
        else
            positionOffset = availableFreeSpace;
        break;
    case ContentPosition::FlexStart: // Only used in flex layout, for other layout, it's equivalent to 'start'.
    case ContentPosition::Start:
        if (isRowAxis)
            positionOffset = style().isLeftToRightDirection() ? 0_lu : availableFreeSpace;
        break;
    case ContentPosition::Baseline:
    case ContentPosition::LastBaseline:
        // FIXME: Implement the previous values. For now, we always 'start' align.
        // http://webkit.org/b/145566
        if (isRowAxis)
            positionOffset = style().isLeftToRightDirection() ? 0_lu : availableFreeSpace;
        break;
    case ContentPosition::Normal:
    default:
        ASSERT_NOT_REACHED();
        return;
    }

    offset.positionOffset = positionOffset;
    offset.distributionOffset = 0_lu;
}

LayoutUnit RenderGrid::translateOutOfFlowRTLCoordinate(const RenderBox& child, LayoutUnit coordinate) const
{
    ASSERT(child.isOutOfFlowPositioned());
    ASSERT(!style().isLeftToRightDirection());

    if (m_outOfFlowItemColumn.get(&child))
        return translateRTLCoordinate(coordinate);

    return borderLogicalLeft() + borderLogicalRight() + clientLogicalWidth() - coordinate;
}

LayoutUnit RenderGrid::translateRTLCoordinate(LayoutUnit coordinate) const
{
    ASSERT(!style().isLeftToRightDirection());

    LayoutUnit alignmentOffset = m_columnPositions[0];
    LayoutUnit rightGridEdgePosition = m_columnPositions[m_columnPositions.size() - 1];
    return rightGridEdgePosition + alignmentOffset - coordinate;
}

// FIXME: SetLogicalPositionForChild has only one caller, consider its refactoring in the future.
void RenderGrid::setLogicalPositionForChild(RenderBox& child) const
{
    // "In the positioning phase [...] calculations are performed according to the writing mode of the containing block of the box establishing the
    // orthogonal flow." However, 'setLogicalLocation' will only take into account the child's writing-mode, so the position may need to be transposed.
    LayoutPoint childLocation(logicalOffsetForChild(child, ForColumns), logicalOffsetForChild(child, ForRows));
    child.setLogicalLocation(GridLayoutFunctions::isOrthogonalChild(*this, child) ? childLocation.transposedPoint() : childLocation);
}

void RenderGrid::setLogicalOffsetForChild(RenderBox& child, GridTrackSizingDirection direction) const
{
    if (!child.isGridItem() && hasStaticPositionForChild(child, direction))
        return;
    // 'setLogicalLeft' and 'setLogicalTop' only take into account the child's writing-mode, that's why 'flowAwareDirectionForChild' is needed.
    if (GridLayoutFunctions::flowAwareDirectionForChild(*this, child, direction) == ForColumns)
        child.setLogicalLeft(logicalOffsetForChild(child, direction));
    else
        child.setLogicalTop(logicalOffsetForChild(child, direction));
}

LayoutUnit RenderGrid::logicalOffsetForChild(const RenderBox& child, GridTrackSizingDirection direction) const
{
    if (direction == ForRows)
        return columnAxisOffsetForChild(child);
    LayoutUnit rowAxisOffset = rowAxisOffsetForChild(child);
    // We stored m_columnPositions's data ignoring the direction, hence we might need now
    // to translate positions from RTL to LTR, as it's more convenient for painting.
    if (!style().isLeftToRightDirection())
        rowAxisOffset = (child.isOutOfFlowPositioned() ? translateOutOfFlowRTLCoordinate(child, rowAxisOffset) : translateRTLCoordinate(rowAxisOffset)) - (GridLayoutFunctions::isOrthogonalChild(*this, child) ? child.logicalHeight()  : child.logicalWidth());
    return rowAxisOffset;
}

unsigned RenderGrid::nonCollapsedTracks(GridTrackSizingDirection direction) const
{
    auto& tracks = m_trackSizingAlgorithm.tracks(direction);
    size_t numberOfTracks = tracks.size();
    bool hasCollapsedTracks = currentGrid().hasAutoRepeatEmptyTracks(direction);
    size_t numberOfCollapsedTracks = hasCollapsedTracks ? currentGrid().autoRepeatEmptyTracks(direction)->size() : 0;
    return numberOfTracks - numberOfCollapsedTracks;
}

unsigned RenderGrid::numTracks(GridTrackSizingDirection direction) const
{
    // Due to limitations in our internal representation, we cannot know the number of columns from
    // currentGrid *if* there is no row (because currentGrid would be empty). That's why in that case we need
    // to get it from the style. Note that we know for sure that there are't any implicit tracks,
    // because not having rows implies that there are no "normal" children (out-of-flow children are
    // not stored in currentGrid).
    ASSERT(!currentGrid().needsItemsPlacement());
    if (direction == ForRows)
        return currentGrid().numTracks(ForRows);

    // FIXME: This still requires knowledge about currentGrid internals.
    return currentGrid().numTracks(ForRows) ? currentGrid().numTracks(ForColumns) : GridPositionsResolver::explicitGridColumnCount(style(), currentGrid().autoRepeatTracks(ForColumns));
}

void RenderGrid::paintChildren(PaintInfo& paintInfo, const LayoutPoint& paintOffset, PaintInfo& forChild, bool usePrintRect)
{
    ASSERT(!currentGrid().needsItemsPlacement());
    for (RenderBox* child = currentGrid().orderIterator().first(); child; child = currentGrid().orderIterator().next())
        paintChild(*child, paintInfo, paintOffset, forChild, usePrintRect, PaintAsInlineBlock);
}

const char* RenderGrid::renderName() const
{
    if (isFloating())
        return "RenderGrid (floating)";
    if (isOutOfFlowPositioned())
        return "RenderGrid (positioned)";
    if (isAnonymous())
        return "RenderGrid (generated)";
    if (isRelativelyPositioned())
        return "RenderGrid (relative positioned)";
    return "RenderGrid";
}

bool RenderGrid::hasAutoSizeInColumnAxis(const RenderBox& child) const
{
    if (child.style().hasAspectRatio()) {
        // FIXME: should align-items + align-self: auto/justify-items + justify-self: auto be taken into account?
        if (isHorizontalWritingMode() == child.isHorizontalWritingMode() && child.style().alignSelf().position() != ItemPosition::Stretch) {
            // A non-auto inline size means the same for block size (column axis size) because of the aspect ratio.
            if (!child.style().logicalWidth().isAuto())
                return false;
        } else if (child.style().justifySelf().position() != ItemPosition::Stretch) {
            const Length& logicalHeight = child.style().logicalHeight();
            if (logicalHeight.isFixed() || (logicalHeight.isPercentOrCalculated() && child.percentageLogicalHeightIsResolvable()))
                return false;
        }
    }
    return isHorizontalWritingMode() ? child.style().height().isAuto() : child.style().width().isAuto();
}

bool RenderGrid::hasAutoSizeInRowAxis(const RenderBox& child) const
{
    if (child.style().hasAspectRatio()) {
        // FIXME: should align-items + align-self: auto/justify-items + justify-self: auto be taken into account?
        if (isHorizontalWritingMode() == child.isHorizontalWritingMode() && child.style().justifySelf().position() != ItemPosition::Stretch) {
            // A non-auto block size means the same for inline size (row axis size) because of the aspect ratio.
            const Length& logicalHeight = child.style().logicalHeight();
            if (logicalHeight.isFixed() || (logicalHeight.isPercentOrCalculated() && child.percentageLogicalHeightIsResolvable()))
                return false;
        } else if (child.style().alignSelf().position() != ItemPosition::Stretch) {
            if (!child.style().logicalWidth().isAuto())
                return false;
        }
    }
    return isHorizontalWritingMode() ? child.style().width().isAuto() : child.style().height().isAuto();
}

RenderGrid::GridWrapper::GridWrapper(RenderGrid& renderGrid)
    : m_layoutGrid(renderGrid)
{ }

void RenderGrid::GridWrapper::resetCurrentGrid() const
{
    m_currentGrid = std::ref(const_cast<Grid&>(m_layoutGrid));
}

} // namespace WebCore
