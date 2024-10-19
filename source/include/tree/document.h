// document.h
// Copyright (c) 2022, zhiayang
// SPDX-License-Identifier: Apache-2.0

#pragma once

#include "tree/base.h"
#include "interp/eval_result.h"

namespace sap::layout
{
	struct Document;
	struct PageCursor;
}

namespace sap::interp::ast
{
	struct Stmt;
	struct Block;
}

namespace sap::tree
{
	struct Container;

	struct Document
	{
		explicit Document(std::vector<std::unique_ptr<interp::ast::Stmt>> preamble, bool have_doc_start);

		void addObject(zst::SharedPtr<BlockObject> obj);

		ErrorOr<std::unique_ptr<layout::Document>> layout(interp::Interpreter* cs);
		ErrorOr<std::vector<std::pair<std::unique_ptr<interp::cst::Stmt>, interp::EvalResult>>> //
		runPreamble(interp::Interpreter* cs);

		bool haveDocStart() const { return m_have_document_start; }

		zst::SharedPtr<tree::Container> takeContainer() &&;
		std::vector<std::unique_ptr<interp::ast::Stmt>> takePreamble() &&;

		const tree::Container& container() const { return *m_container; }

	private:
		zst::SharedPtr<tree::Container> m_container;
		std::vector<std::unique_ptr<interp::ast::Stmt>> m_preamble;
		bool m_have_document_start;
	};
}
