// container.h
// Copyright (c) 2022, yuki
// SPDX-License-Identifier: Apache-2.0

#pragma once

#include "sap/path.h"
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

		explicit Container(Direction direction, bool glue = false);
		Direction direction() const { return m_direction; }

		virtual ErrorOr<void> evaluateScripts(interp::Interpreter* cs) const override;

		std::vector<zst::SharedPtr<BlockObject>>& contents() { return m_objects; }
		const std::vector<zst::SharedPtr<BlockObject>>& contents() const { return m_objects; }

		BorderStyle borderStyle() const { return m_border_style; }
		void setBorderStyle(BorderStyle bs) { m_border_style = std::move(bs); }

		static zst::SharedPtr<Container> makeVertBox();
		static zst::SharedPtr<Container> makeHorzBox();
		static zst::SharedPtr<Container> makeStackBox();

	private:
		virtual ErrorOr<LayoutResult> create_layout_object_impl(interp::Interpreter* cs,
		    const Style& parent_style,
		    Size2d available_space) const override;

	private:
		bool m_glued;
		Direction m_direction;
		BorderStyle m_border_style {};
		std::vector<zst::SharedPtr<BlockObject>> m_objects;
	};
}
