// document.h
// Copyright (c) 2022, zhiayang
// SPDX-License-Identifier: Apache-2.0

#pragma once

#include "tree/base.h"

namespace sap::tree
{
	struct Document
	{
		void addObject(std::unique_ptr<DocumentObject> obj);

		std::vector<DocumentObject*>& objects() { return m_objects; }
		const std::vector<DocumentObject*>& objects() const { return m_objects; }

		std::vector<std::unique_ptr<DocumentObject>>& allObjects() { return m_all_objects; }

		void evaluateScripts(interp::Interpreter* cs);
		void processWordSeparators();

	private:
		void evaluate_scripts(interp::Interpreter* cs, std::vector<DocumentObject*>& objs, DocumentObject* obj, bool is_top);
		void evaluate_script_call(interp::Interpreter* cs, std::vector<DocumentObject*>& objs, ScriptCall* call, bool is_top);

		bool process_word_separators(DocumentObject* obj);

		std::vector<DocumentObject*> m_objects {};
		std::vector<std::unique_ptr<DocumentObject>> m_all_objects {};
	};
}
