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
	struct Stmt;
	struct Block;
}

namespace sap::tree
{
	struct Container;

	struct Document
	{
		explicit Document(std::vector<std::unique_ptr<interp::Stmt>> preamble, bool have_doc_start);

		void addObject(zst::SharedPtr<BlockObject> obj);
		ErrorOr<std::unique_ptr<layout::Document>> layout(interp::Interpreter* cs);

		bool haveDocStart() const { return m_have_document_start; }

		zst::SharedPtr<tree::Container> takeContainer() &&;
		std::vector<std::unique_ptr<interp::Stmt>> takePreamble() &&;

	private:
		zst::SharedPtr<tree::Container> m_container;
		std::vector<std::unique_ptr<interp::Stmt>> m_preamble;
		bool m_have_document_start;
	};
}
