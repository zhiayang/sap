// wrappers.h
// Copyright (c) 2022, zhiayang
// SPDX-License-Identifier: Apache-2.0

#pragma once

#include "tree/base.h"

namespace sap::tree
{
	struct WrappedLine : BlockObject
	{
		virtual ErrorOr<void> evaluateScripts(interp::Interpreter* cs) const override;
		virtual ErrorOr<LayoutResult> createLayoutObject(interp::Interpreter* cs,
			const Style* parent_style,
			Size2d available_space) const override;

		void addObjects(std::vector<std::unique_ptr<InlineObject>> objs);

	private:
		std::vector<std::unique_ptr<InlineObject>> m_objects;
	};
}