// interp.h
// Copyright (c) 2022, yuki
// SPDX-License-Identifier: Apache-2.0

#pragma once

#include <deque>

#include "ast.h"
#include "util.h"
#include "value.h"

#include "sap/config.h"

#include "interp/type.h"
#include "interp/basedefs.h"
#include "interp/eval_result.h"

#include "interp/evaluator.h"
#include "interp/typechecker.h"

namespace sap::tree
{
	struct BlockObject;
	struct InlineObject;
}

namespace sap::layout
{
	struct Document;
}

namespace sap::interp
{
	struct Interpreter
	{
		Interpreter();

		zst::byte_span loadFile(zst::str_view filename);

		Evaluator& evaluator() { return *m_evaluator; }
		Typechecker& typechecker() { return *m_typechecker; }

		pdf::PdfFont& addLoadedFont(std::unique_ptr<pdf::PdfFont> font);
		ErrorOr<pdf::PdfFont*> getLoadedFontById(int64_t font_id);

		// this is necessary to keep the strings around...
		std::u32string& keepStringAlive(zst::wstr_view str);
		std::string& keepStringAlive(zst::str_view str);

		ProcessingPhase currentPhase() const { return m_current_phase; }
		void setCurrentPhase(ProcessingPhase phase) { m_current_phase = phase; }

		void addHookBlock(const ast::HookBlock* block);
		void addHookBlock(ProcessingPhase phase, const cst::Block* block);

		ErrorOr<void> runHooks();

		void addImportedFile(std::string filename);
		bool wasFileImported(const std::string& filename) const;

		tree::BlockObject* retainBlockObject(zst::SharedPtr<tree::BlockObject> tbo);

		void addMicrotypeConfig(config::MicrotypeConfig config);
		std::optional<CharacterProtrusion> getMicrotypeProtrusionFor(char32_t ch, const Style& style) const;

	private:
		std::unique_ptr<Typechecker> m_typechecker;
		std::unique_ptr<Evaluator> m_evaluator;

		ProcessingPhase m_current_phase;
		util::hashmap<ProcessingPhase, std::vector<const cst::Block*>> m_hook_blocks;

		std::vector<zst::unique_span<uint8_t[]>> m_file_contents;

		util::hashmap<int64_t, std::unique_ptr<pdf::PdfFont>> m_loaded_fonts;

		std::vector<zst::SharedPtr<tree::BlockObject>> m_retained_tbos;

		util::hashset<std::string> m_imported_files;

		std::vector<std::string> m_leaked_strings;
		std::vector<std::u32string> m_leaked_strings32;

		std::vector<config::MicrotypeConfig> m_microtype_configs;
	};
}
