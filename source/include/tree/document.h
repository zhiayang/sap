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

namespace sap::interp
{
	struct Block;
}

namespace sap::tree
{
	struct VertBox;

	struct Document
	{
		explicit Document(std::unique_ptr<interp::Block> preamble, std::unique_ptr<interp::FunctionCall> doc_start);

		void addObject(std::unique_ptr<BlockObject> obj);
		std::unique_ptr<layout::Document> layout(interp::Interpreter* cs);

		bool haveDocStart() const { return m_document_start != nullptr; }

		std::unique_ptr<tree::VertBox> takeContainer() &&;
		std::unique_ptr<interp::Block> takePreamble() &&;
		std::unique_ptr<interp::FunctionCall> takeDocStart() &&;

	private:
		std::unique_ptr<tree::VertBox> m_container;

		std::unique_ptr<interp::Block> m_preamble;
		std::unique_ptr<interp::FunctionCall> m_document_start;
	};
}
