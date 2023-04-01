// spacer.h
// Copyright (c) 2022, zhiayang
// SPDX-License-Identifier: Apache-2.0

#pragma once

#include "tree/base.h"

namespace sap::tree
{
	struct Spacer : BlockObject
	{
		explicit Spacer(DynLength2d size);

		virtual ErrorOr<void> evaluateScripts(interp::Interpreter* cs) const override;

	private:
		virtual ErrorOr<LayoutResult> create_layout_object_impl(interp::Interpreter* cs,
			const Style* parent_style,
			Size2d available_space) const override;

	private:
		DynLength2d m_size;
	};
}
