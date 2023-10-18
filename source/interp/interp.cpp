// interp.cpp
// Copyright (c) 2022, zhiayang
// SPDX-License-Identifier: Apache-2.0

#include "location.h" // for error

#include "pdf/font.h"

#include "tree/base.h"

#include "interp/ast.h"         // for Definition, makeParamList, Expr, Stmt
#include "interp/type.h"        // for Type
#include "interp/interp.h"      // for StackFrame, Interpreter, DefnTree
#include "interp/eval_result.h" // for EvalResult

namespace sap::interp
{
	extern void defineBuiltins(Interpreter* interp, DefnTree* builtin_ns);

	Interpreter::Interpreter()
	    : m_typechecker(new Typechecker(this))
	    , m_evaluator(new Evaluator(this))
	    , m_current_phase(ProcessingPhase::Start)
	{
		m_typechecker->pushLocation(Location::builtin()).cancel();
		m_evaluator->pushLocation(Location::builtin()).cancel();

		defineBuiltins(this, m_typechecker->top()->lookupOrDeclareNamespace("builtin"));
	}


	void Interpreter::addImportedFile(std::string filename)
	{
		m_imported_files.insert(std::move(filename));
	}

	bool Interpreter::wasFileImported(const std::string& filename) const
	{
		return m_imported_files.contains(filename);
	}

	void Interpreter::addHookBlock(const ast::HookBlock* block)
	{
		m_hook_blocks[block->phase].push_back(block);
	}

	void Interpreter::addHookBlock(ProcessingPhase phase, const cst::Block* block)
	{
		m_hook_blocks2[phase].push_back(block);
	}

	ErrorOr<void> Interpreter::runHooks()
	{
		for(auto* blk : m_hook_blocks[m_current_phase])
		{
			auto tc_blk = TRY(blk->typecheck2(&this->typechecker())).take<cst::Block>();
			TRY(tc_blk->evaluate(&this->evaluator()));
		}

		return Ok();
	}

	zst::byte_span Interpreter::loadFile(zst::str_view filename)
	{
		auto file = util::readEntireFile(filename.str());
		return m_file_contents.emplace_back(std::move(file)).span();
	}

	std::u32string& Interpreter::keepStringAlive(zst::wstr_view str)
	{
		return m_leaked_strings32.emplace_back(str.str());
	}

	std::string& Interpreter::keepStringAlive(zst::str_view str)
	{
		return m_leaked_strings.emplace_back(str.str());
	}

	pdf::PdfFont& Interpreter::addLoadedFont(std::unique_ptr<pdf::PdfFont> font)
	{
		if(auto it = m_loaded_fonts.find(font->fontId()); it != m_loaded_fonts.end())
			return *it->second;

		auto id = font->fontId();
		return *m_loaded_fonts.emplace(id, std::move(font)).first->second;
	}

	ErrorOr<pdf::PdfFont*> Interpreter::getLoadedFontById(int64_t font_id)
	{
		if(auto it = m_loaded_fonts.find(font_id); it != m_loaded_fonts.end())
			return Ok(it->second.get());

		return ErrMsg(m_evaluator.get(), "font with id {} was not loaded", font_id);
	}

	tree::BlockObject* Interpreter::addAbsolutelyPositionedBlockObject(zst::SharedPtr<tree::BlockObject> tbo)
	{
		return m_abs_tbos.emplace_back(std::move(tbo)).get();
	}




	void Interpreter::addMicrotypeConfig(config::MicrotypeConfig config)
	{
		m_microtype_configs.push_back(std::move(config));
	}

	std::optional<CharacterProtrusion> Interpreter::getMicrotypeProtrusionFor(char32_t ch, const Style& style) const
	{
		auto font = style.font();
		auto font_name = font->source().name();
		auto font_style = style.font_style();

		using enum FontStyle;
		for(auto& config : m_microtype_configs)
		{
			if(not config.matched_fonts.contains(font_name))
				continue;

			if(config.enable_if_italic && not util::is_one_of(font_style, Italic, BoldItalic))
				continue;

			// ok, it should match. TODO: check features.
			if(auto it = config.protrusions.find(ch); it != config.protrusions.end())
				return CharacterProtrusion { it->second.left, it->second.right };
		}

		return std::nullopt;
	}










	ErrorOr<TCResult2> ast::Stmt::typecheck2(Typechecker* ts, const Type* infer, bool keep_lvalue) const
	{
		auto _ = ts->pushLocation(m_location);
		return this->typecheck_impl2(ts, infer, keep_lvalue);
	}

	ErrorOr<EvalResult> cst::Stmt::evaluate(Evaluator* ev) const
	{
		auto _ = ev->pushLocation(m_location);
		return this->evaluate_impl(ev);
	}
}
