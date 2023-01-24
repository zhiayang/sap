// document.h
// Copyright (c) 2022, zhiayang
// SPDX-License-Identifier: Apache-2.0

#pragma once

#include "tree/base.h"

namespace sap::layout
{
	struct Document;
	struct PageCursor;
}

namespace sap::tree
{
	struct VertBox;

	struct Document
	{
		explicit Document(std::unique_ptr<tree::ScriptBlock> preamble);

		void addObject(std::unique_ptr<BlockObject> obj);
		void layout(interp::Interpreter* cs, layout::Document* layout_doc);

		std::unique_ptr<tree::VertBox> takeContainer() &&;

	private:
		std::unique_ptr<tree::VertBox> m_container;
	};
}
