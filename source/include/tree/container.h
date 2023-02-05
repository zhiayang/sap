// container.h
// Copyright (c) 2022, zhiayang
// SPDX-License-Identifier: Apache-2.0

#pragma once

#include "tree/base.h"

namespace sap::tree
{
	struct VertBox : BlockObject
	{
		virtual ErrorOr<void> evaluateScripts(interp::Interpreter* cs) const override;
		virtual ErrorOr<LayoutResult> createLayoutObject(interp::Interpreter* cs,
			const Style* parent_style,
			Size2d available_space) const override;

		std::vector<std::unique_ptr<BlockObject>>& contents() { return m_objects; }
		const std::vector<std::unique_ptr<BlockObject>>& contents() const { return m_objects; }

	private:
		std::vector<std::unique_ptr<BlockObject>> m_objects;
	};

	struct HorzBox : BlockObject
	{
		virtual ErrorOr<void> evaluateScripts(interp::Interpreter* cs) const override;
		virtual ErrorOr<LayoutResult> createLayoutObject(interp::Interpreter* cs,
			const Style* parent_style,
			Size2d available_space) const override;

		std::vector<std::unique_ptr<BlockObject>>& contents() { return m_objects; }
		const std::vector<std::unique_ptr<BlockObject>>& contents() const { return m_objects; }

	private:
		std::vector<std::unique_ptr<BlockObject>> m_objects;
	};
}
