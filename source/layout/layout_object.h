// layout_object.h
// Copyright (c) 2022, yuki
// SPDX-License-Identifier: Apache-2.0

#pragma once

#include "sap/style.h"

#include "tree/base.h"
#include "layout/cursor.h"

namespace pdf
{
	struct Page;
}

namespace sap::tree
{
	struct BlockObject;
	struct InlineObject;
}

namespace sap::layout
{
	struct LayoutBase;
	struct PageLayout;

	struct LayoutObject : Stylable
	{
		explicit LayoutObject(const Style& style, LayoutSize size);
		virtual ~LayoutObject() = default;

		LayoutSize layoutSize() const;

		void positionAbsolutely(AbsolutePagePos pos);
		void positionRelatively(RelativePos pos);

		bool isPositioned() const;
		bool isAbsolutelyPositioned() const;
		bool isRelativelyPositioned() const;

		RelativePos relativePosition() const;
		AbsolutePagePos absolutePosition() const;
		AbsolutePagePos resolveAbsPosition(const LayoutBase* layout) const;

		void render(const LayoutBase* layout, std::vector<pdf::Page*>& pages) const;
		PageCursor computePosition(PageCursor cursor);

		void overrideLayoutSizeX(Length x);
		void overrideLayoutSizeY(Length y);
		void overrideAbsolutePosition(AbsolutePagePos pos);
		void addRelativePositionOffset(Size2d offset);

		tree::LinkDestination linkDestination() const;
		void setLinkDestination(tree::LinkDestination dest);

		virtual bool is_phantom() const { return false; }
		virtual bool requires_space_reservation() const { return false; }

		virtual PageCursor compute_position_impl(PageCursor cursor) = 0;
		virtual void render_impl(const LayoutBase* layout, std::vector<pdf::Page*>& pages) const = 0;

		static void* operator new(size_t count);
		static void operator delete(void* ptr, size_t count);

	protected:
		std::optional<Either<RelativePos, AbsolutePagePos>> m_position {};
		LayoutSize m_layout_size;

	private:
		std::optional<Size2d> m_relative_pos_offset {};
		std::optional<AbsolutePagePos> m_absolute_pos_override {};
		tree::LinkDestination m_link_destination {};
	};



	struct LayoutSpan : LayoutObject
	{
		LayoutSpan(Length relative_offset, Length raise_height, LayoutSize size);

		virtual layout::PageCursor compute_position_impl(layout::PageCursor cursor) override;
		virtual void render_impl(const LayoutBase* layout, std::vector<pdf::Page*>& pages) const override;

		Length relativeOffset() const { return m_relative_offset; }

	private:
		Length m_relative_offset {};
		Length m_raise_height {};
	};
}
