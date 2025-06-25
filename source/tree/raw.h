// raw.h
// Copyright (c) 2022, yuki
// SPDX-License-Identifier: Apache-2.0

#pragma once

#include "tree/base.h"

namespace sap::tree
{
	struct RawBlock : BlockObject
	{
		explicit RawBlock(zst::wstr_view str);

		virtual ErrorOr<void> evaluateScripts(interp::Interpreter* cs) const override;

	private:
		virtual ErrorOr<LayoutResult> create_layout_object_impl(interp::Interpreter* cs,
		    const Style& parent_style,
		    Size2d available_space) const override;

	private:
		std::u32string m_text;
		std::vector<zst::SharedPtr<InlineObject>> m_lines;
	};
}
