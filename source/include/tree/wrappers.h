// wrappers.h
// Copyright (c) 2022, zhiayang
// SPDX-License-Identifier: Apache-2.0

#pragma once

#include "tree/base.h"

namespace sap::tree
{
	struct WrappedLine : BlockObject
	{
		WrappedLine() { }
		explicit WrappedLine(std::vector<std::unique_ptr<InlineObject>> objs);

		virtual ErrorOr<void> evaluateScripts(interp::Interpreter* cs) const override;
		void addObjects(std::vector<std::unique_ptr<InlineObject>> objs);

		const std::vector<std::unique_ptr<InlineObject>>& objects() const { return m_objects; }

	private:
		virtual ErrorOr<LayoutResult> create_layout_object_impl(interp::Interpreter* cs,
			const Style* parent_style,
			Size2d available_space) const override;

	private:
		std::vector<std::unique_ptr<InlineObject>> m_objects;
	};
}
