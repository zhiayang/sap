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

	Interpreter::Interpreter() : m_typechecker(new Typechecker()), m_evaluator(new Evaluator(this))
	{
		m_typechecker->pushLocation(Location::builtin()).cancel();
		m_evaluator->pushLocation(Location::builtin()).cancel();

		defineBuiltins(this, m_typechecker->top()->lookupOrDeclareNamespace("builtin"));
	}

	zst::byte_span Interpreter::loadFile(zst::str_view filename)
	{
		auto file = util::readEntireFile(filename.str());
		return m_file_contents.emplace_back(std::move(file)).span();
	}

	ErrorOr<EvalResult> Interpreter::run(const Stmt* stmt)
	{
		TRY(stmt->typecheck(m_typechecker.get()));
		return stmt->evaluate(m_evaluator.get());
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


	ErrorOr<EvalResult> Stmt::evaluate(Evaluator* ev) const
	{
		auto _ = ev->pushLocation(m_location);
		return this->evaluate_impl(ev);
	}

	ErrorOr<TCResult> Stmt::typecheck(Typechecker* ts, const Type* infer) const
	{
		auto _ = ts->pushLocation(m_location);

		if(not m_tc_result.has_value())
			m_tc_result = TRY(this->typecheck_impl(ts, infer));

		return Ok(*m_tc_result);
	}
}
