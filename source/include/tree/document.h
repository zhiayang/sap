// document.h
// Copyright (c) 2022, zhiayang
// SPDX-License-Identifier: Apache-2.0

#pragma once

#include "tree/base.h"

namespace sap::layout
{
	struct Document;
	struct LineCursor;
}

namespace sap::tree
{
	struct Document
	{
		void addObject(std::unique_ptr<DocumentObject> obj);

		std::vector<std::unique_ptr<DocumentObject>>& objects() { return m_objects; }
		const std::vector<std::unique_ptr<DocumentObject>>& objects() const { return m_objects; }

		void layout(interp::Interpreter* cs, layout::Document* layout_doc);

	private:
		std::vector<std::unique_ptr<DocumentObject>> m_objects {};
	};
}
