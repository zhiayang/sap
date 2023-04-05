// raw.h
// Copyright (c) 2022, zhiayang
// SPDX-License-Identifier: Apache-2.0

#pragma once

#include "tree/base.h"

namespace sap::tree
{
	struct RawBlock : BlockObject
	{
		explicit RawBlock(zst::wstr_view str);

		virtual ErrorOr<void> evaluateScripts(interp::Interpreter* cs) const override;
		std::unique_ptr<RawBlock> clone() const;

	private:
		virtual ErrorOr<LayoutResult> create_layout_object_impl(interp::Interpreter* cs,
			const Style* parent_style,
			Size2d available_space) const override;

	private:
		std::u32string m_text;
		std::vector<std::unique_ptr<InlineObject>> m_lines;
	};
}
