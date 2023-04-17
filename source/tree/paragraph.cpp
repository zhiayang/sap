// paragraph.cpp
// Copyright (c) 2022, zhiayang
// SPDX-License-Identifier: Apache-2.0

#include "sap/paths.h"
#include "tree/wrappers.h"
#include "misc/hyphenator.h"

#include "interp/ast.h"         // for FunctionCall
#include "interp/value.h"       // for Value
#include "interp/interp.h"      // for Interpreter
#include "interp/basedefs.h"    // for InlineObject
#include "interp/eval_result.h" // for EvalResult

#include "layout/base.h"
#include "layout/paragraph.h"

namespace sap::tree
{
	ErrorOr<void> Paragraph::evaluateScripts(interp::Interpreter* cs) const
	{
		return Ok();
	}

	auto Paragraph::eval_single_script_in_para(interp::Interpreter* cs,
	    Size2d available_space,
	    ScriptCall* iscr,
	    bool allow_blocks) const -> ErrorOr<std::optional<EvalScriptResult>>
	{
		if(iscr->runPhase() != ProcessingPhase::Layout)
			return ErrMsg(iscr->call->loc(), "inline script calls must run at layout time");

		auto result = TRY(iscr->evaluate_script(cs, m_style, available_space));
		if(not result.has_value())
			return Ok(std::nullopt);

		// if we allow blocks, we can just pass through the result.
		if(allow_blocks)
			return Ok(std::move(result));

		if(result->is_left())
			return Ok(std::move(result));
		else
			return ErrMsg(iscr->call->loc(), "cannot insert LayoutObject or Block objects in paragraphs");
	}


	auto Paragraph::evaluate_scripts(interp::Interpreter* cs, Size2d available_space) const
	    -> ErrorOr<std::optional<EvalScriptResult>>
	{
		// if the paragraph has exactly one guy and that guy is a script call,
		// then we can allow that guy to be promoted to a block object.
		if(m_contents.size() == 1)
		{
			if(auto iscr = m_contents.front()->castToScriptCall())
				return this->eval_single_script_in_para(cs, available_space, iscr, /* allow blocks: */ true);
		}

		auto inlines = zst::make_shared<InlineSpan>(/* glue: */ false);
		for(auto& obj : m_contents)
		{
			if(obj->isText() || obj->isSeparator() || obj->isSpan())
			{
				inlines->addObject(obj);
			}
			else if(auto iscr = obj->castToScriptCall())
			{
				auto tmp = TRY(this->eval_single_script_in_para(cs, available_space, iscr, /* allow blocks: */ false));
				if(not tmp.has_value())
					continue;

				assert(tmp->is_left());
				inlines->addObject(tmp->take_left());
			}
			else
			{
				return ErrMsg(&cs->evaluator(), "unsupported");
			}
		}

		return Ok(Left(std::move(inlines)));
	}


	Paragraph::Paragraph(std::vector<zst::SharedPtr<InlineObject>> objs)
	    : BlockObject(Kind::Paragraph), m_contents(std::move(objs))
	{
	}

	void Paragraph::addObject(zst::SharedPtr<InlineObject> obj)
	{
		m_contents.push_back(std::move(obj));
	}

	void Paragraph::addObjects(std::vector<zst::SharedPtr<InlineObject>> objs)
	{
		m_contents.insert(m_contents.end(), std::move_iterator(objs.begin()), std::move_iterator(objs.end()));
	}


	Text::Text(std::u32string text) : InlineObject(Kind::Text), m_contents(std::move(text))
	{
	}

	Text::Text(std::u32string text, const Style& style) : InlineObject(Kind::Text), m_contents(std::move(text))
	{
		this->setStyle(style);
	}

	Separator::Separator(SeparatorKind kind, int hyphenation_cost)
	    : InlineObject(Kind::Separator), m_kind(kind), m_hyphenation_cost(hyphenation_cost)
	{
		switch(m_kind)
		{
			case decltype(kind)::SPACE:
			case decltype(kind)::SENTENCE_END:
				m_end_of_line_char = 0;
				m_middle_of_line_char = &s_space;
				break;

			case decltype(kind)::BREAK_POINT:
				m_end_of_line_char = 0;
				m_middle_of_line_char = 0;
				break;

			case decltype(kind)::HYPHENATION_POINT:
				m_end_of_line_char = &s_hyphen;
				m_middle_of_line_char = 0;
				break;
		}
	}


}
