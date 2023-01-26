// interp.h
// Copyright (c) 2022, zhiayang
// SPDX-License-Identifier: Apache-2.0

#pragma once

#include <deque>

#include "ast.h"   // for Definition, Declaration, QualifiedId
#include "util.h"  // for Defer, hashmap
#include "value.h" // for Value

#include "interp/type.h"        // for Type
#include "interp/basedefs.h"    // for InlineObject
#include "interp/eval_result.h" // for EvalResult

#include "interp/evaluator.h"
#include "interp/typechecker.h"

namespace sap::tree
{
	struct BlockObject;
	struct InlineObject;
}

namespace sap::interp
{
	struct Interpreter
	{
		Interpreter();

		zst::byte_span loadFile(zst::str_view filename);

		ErrorOr<EvalResult> run(const Stmt* stmt);

		Evaluator& evaluator() { return *m_evaluator; }
		Typechecker& typechecker() { return *m_typechecker; }

		pdf::PdfFont& addLoadedFont(std::unique_ptr<pdf::PdfFont> font);
		ErrorOr<pdf::PdfFont*> getLoadedFontById(int64_t font_id);

		// this is necessary to keep the strings around...
		std::u32string& keepStringAlive(zst::wstr_view str);
		std::string& keepStringAlive(zst::str_view str);

		ProcessingPhase currentPhase() const { return m_current_phase; }
		void setCurrentPhase(ProcessingPhase phase) { m_current_phase = phase; }

	private:
		std::unique_ptr<Typechecker> m_typechecker;
		std::unique_ptr<Evaluator> m_evaluator;

		ProcessingPhase m_current_phase;

		std::vector<zst::unique_span<uint8_t[]>> m_file_contents;

		util::hashmap<int64_t, std::unique_ptr<pdf::PdfFont>> m_loaded_fonts;

		std::vector<std::string> m_leaked_strings;
		std::vector<std::u32string> m_leaked_strings32;
	};
}
