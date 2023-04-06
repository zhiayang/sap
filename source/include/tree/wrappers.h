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
		explicit WrappedLine(std::vector<zst::SharedPtr<InlineObject>> objs);

		virtual ErrorOr<void> evaluateScripts(interp::Interpreter* cs) const override;
		void addObjects(std::vector<zst::SharedPtr<InlineObject>> objs);

		const std::vector<zst::SharedPtr<InlineObject>>& objects() const { return m_objects; }

	private:
		virtual ErrorOr<LayoutResult> create_layout_object_impl(interp::Interpreter* cs,
			const Style* parent_style,
			Size2d available_space) const override;

	private:
		std::vector<zst::SharedPtr<InlineObject>> m_objects;
	};
}
