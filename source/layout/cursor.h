// cursor.h
// Copyright (c) 2022, yuki
// SPDX-License-Identifier: Apache-2.0

#pragma once

#include "sap/units.h"

namespace sap::layout
{
	struct LayoutBase;

	struct RelativePos
	{
		struct TAG_RELATIVE;
		using Pos = dim::Vector2<sap::Length::unit_system, TAG_RELATIVE>;

		Pos pos;
		size_t page_num;
	};

	struct AbsolutePagePos
	{
		Position pos;
		size_t page_num;
	};


	struct CursorPayload
	{
		uint8_t payload[48];
	};

	struct PageCursor
	{
		PageCursor(LayoutBase* layout, CursorPayload payload);
		~PageCursor();

		PageCursor(const PageCursor& other);
		PageCursor& operator=(const PageCursor& other);

		PageCursor(PageCursor&& other);
		PageCursor& operator=(PageCursor&& other);

		Length widthAtCursor() const;
		Length verticalSpaceAtCursor() const;
		[[nodiscard]] PageCursor moveRight(Length shift) const;
		[[nodiscard]] PageCursor moveDown(Length shift) const;
		[[nodiscard]] PageCursor newPage() const;
		[[nodiscard]] PageCursor newLine(Length line_height, bool* made_new_page = nullptr) const;
		[[nodiscard]] PageCursor ensureVerticalSpace(Length line_height, bool* made_new_page = nullptr) const;
		[[nodiscard]] PageCursor carriageReturn() const;
		[[nodiscard]] PageCursor limitWidth(Length width) const;

		RelativePos position() const;
		LayoutBase* layout() const { return m_layout; }

		PageCursor moveToPosition(RelativePos pos) const;

	private:
		LayoutBase* m_layout;
		CursorPayload m_payload;
	};
}
