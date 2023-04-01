// container.h
// Copyright (c) 2022, zhiayang
// SPDX-License-Identifier: Apache-2.0

#pragma once

#include "tree/base.h"

namespace sap::tree
{
	struct Container : BlockObject
	{
		enum class Direction
		{
			None,
			Vertical,
			Horizontal,
		};

		explicit Container(Direction direction);
		Direction direction() const { return m_direction; }

		virtual ErrorOr<void> evaluateScripts(interp::Interpreter* cs) const override;

		std::vector<std::unique_ptr<BlockObject>>& contents() { return m_objects; }
		const std::vector<std::unique_ptr<BlockObject>>& contents() const { return m_objects; }

		static std::unique_ptr<Container> makeVertBox();
		static std::unique_ptr<Container> makeHorzBox();
		static std::unique_ptr<Container> makeStackBox();

	private:
		virtual ErrorOr<LayoutResult> create_layout_object_impl(interp::Interpreter* cs,
			const Style* parent_style,
			Size2d available_space) const override;

	private:
		Direction m_direction;
		std::vector<std::unique_ptr<BlockObject>> m_objects;
	};
}
