// paragraph.cpp
// Copyright (c) 2022, zhiayang
// SPDX-License-Identifier: Apache-2.0

#include <utf8proc/utf8proc.h>

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
			if(auto iscr = dynamic_cast<tree::ScriptCall*>(m_contents.front().get()))
				return this->eval_single_script_in_para(cs, available_space, iscr, /* allow blocks: */ true);
		}

		auto inlines = zst::make_shared<InlineSpan>();
		for(auto& obj : m_contents)
		{
			if(auto txt = dynamic_cast<tree::Text*>(obj.get()))
			{
				inlines->addObject(obj);
			}
			else if(auto sep = dynamic_cast<tree::Separator*>(obj.get()))
			{
				inlines->addObject(obj);
			}
			else if(auto span = dynamic_cast<tree::InlineSpan*>(obj.get()))
			{
				inlines->addObject(obj);
			}
			else if(auto iscr = dynamic_cast<tree::ScriptCall*>(obj.get()))
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

	static bool is_letter(char32_t c)
	{
		auto category = utf8proc_category((utf8proc_int32_t) c);
		return util::is_one_of(category, UTF8PROC_CATEGORY_LU, UTF8PROC_CATEGORY_LL, UTF8PROC_CATEGORY_LT,
		    UTF8PROC_CATEGORY_LM, UTF8PROC_CATEGORY_LO);
	}

	template <typename T, typename... Args>
	static zst::SharedPtr<T> new_thing_from_existing(const zst::SharedPtr<InlineObject>& orig, Args&&... args)
	{
		auto ret = zst::make_shared<T>(static_cast<Args&&>(args)...);
		ret->copyAttributesFrom(*orig);

		return ret;
	}

	static void make_separators_for_word(std::vector<zst::SharedPtr<InlineObject>>& vec, zst::SharedPtr<Text> text)
	{
		static auto hyphenator = hyph::Hyphenator::parseFromFile("hyph-en-gb.tex");

		// TODO: maybe cache at this level as well

		auto word = std::move(text->contents());
		auto orig_span = zst::wstr_view(word);

		zst::wstr_view pre_chunk {};
		zst::wstr_view post_chunk {};

		for(size_t i = 0; i < orig_span.size(); i++)
		{
			if(not is_letter(orig_span[i]))
				orig_span.transfer_prefix(pre_chunk, 1);
			else
				break;
		}

		for(size_t i = orig_span.size(); i-- > 0;)
		{
			if(not is_letter(orig_span[i]))
				orig_span.transfer_suffix(post_chunk, 1);
			else
				break;
		}

		if(not pre_chunk.empty())
			vec.push_back(new_thing_from_existing<Text>(text, pre_chunk.str()));

		bool manual_hyphenation = false;
		for(size_t i = 0; i < orig_span.size(); i++)
		{
			if(not is_letter(orig_span[i]))
			{
				manual_hyphenation = true;
				break;
			}
		}

		if(manual_hyphenation)
		{
			for(size_t i = 0; (i = orig_span.find_first_of(U"-/.")) != (size_t) -1;)
			{
				auto part = orig_span.take_prefix(i + 1);
				vec.push_back(new_thing_from_existing<Text>(text, part.str()));
				vec.push_back(new_thing_from_existing<Separator>(text, Separator::BREAK_POINT));
			}

			vec.push_back(new_thing_from_existing<Text>(text, orig_span.str()));
		}
		else if(not orig_span.empty())
		{
			std::u32string lowercased;
			lowercased.reserve(orig_span.size());
			for(auto c : orig_span)
				lowercased.push_back((char32_t) utf8proc_tolower((utf8proc_int32_t) c));

			auto lower_span = zst::wstr_view(lowercased);
			auto& points = hyphenator.computeHyphenationPoints(lower_span);

			// ignore hyphenations at the first index and last index, since
			// those imply inserting a hyphen before the first character or after the last character
			for(size_t i = 1, k = 1; i < points.size() - 1; i++)
			{
				if(points[i] % 2 == 0)
				{
					k++;
				}
				else
				{
					// hyphenation preference increases from 1 to 3 to 5, so 5 has the lowest cost, and 1 the
					// highest.
					auto part = orig_span.take_prefix(k);
					vec.push_back(new_thing_from_existing<Text>(text, part.str()));
					vec.push_back(new_thing_from_existing<Separator>(text, Separator::HYPHENATION_POINT,
					    /* cost: */ 6 - points[i]));
					k = 1;
				}
			}

			if(orig_span.size() > 0)
				vec.push_back(new_thing_from_existing<Text>(text, orig_span.str()));
		}


		if(not post_chunk.empty())
			vec.push_back(new_thing_from_existing<Text>(text, post_chunk.str()));
	}



	ErrorOr<std::vector<zst::SharedPtr<InlineObject>>> Paragraph::processWordSeparators( //
	    std::vector<zst::SharedPtr<InlineObject>> input)
	{
		std::vector<zst::SharedPtr<InlineObject>> ret {};
		ret.reserve(input.size());

		bool first_obj = true;
		bool seen_whitespace = false;
		for(auto& uwu : input)
		{
			if(dynamic_cast<tree::Separator*>(uwu.get()))
			{
				ret.push_back(std::move(uwu));
				continue;
			}
			else if(auto span = dynamic_cast<tree::InlineSpan*>(uwu.get()); span)
			{
				// note: don't make a new span here, preserve the identities.
				// if we ref a span, it needs to keep existing.
				auto objs = TRY(processWordSeparators(std::move(span->objects())));
				span->objects().clear();
				span->addObjects(std::move(objs));

				ret.push_back(std::move(uwu));
				seen_whitespace = false;
				continue;
			}

			auto tree_text = dynamic_cast<tree::Text*>(uwu.get());
			assert(tree_text != nullptr);

			auto current_text = new_thing_from_existing<Text>(uwu, U"");

			for(char32_t c : tree_text->contents())
			{
				if(c == U' ' || c == U'\t' || c == U'\n' || c == U'\r')
				{
					if(not seen_whitespace)
					{
						// we've seen a space, so flush the current text (if not empty)
						if(not current_text->contents().empty())
						{
							bool did_end_sentence = util::is_one_of(current_text->contents().back(), U'.', U'?', U'!');

							make_separators_for_word(ret, std::move(current_text));

							ret.push_back(new_thing_from_existing<Separator>(uwu,
							    did_end_sentence ? Separator::SENTENCE_END : Separator::SPACE));

							current_text = new_thing_from_existing<Text>(uwu, U"");
						}
						// if the current text is empty, we just put another separator unless
						// we're the first object (don't start with a separator)
						else if(not first_obj)
						{
							ret.push_back(new_thing_from_existing<Separator>(uwu, Separator::SPACE));
						}

						seen_whitespace = true;
					}
					else
					{
						// do nothing (consume more whitespace). note that we don't need to specially
						// handle the case of consecutive newlines forming a paragraph break, since
						// we should not get those things inside Texts anyway.
					}
				}
				else
				{
					current_text->contents() += c;
					seen_whitespace = false;
				}
			}

			if(not current_text->contents().empty())
				make_separators_for_word(ret, std::move(current_text));

			first_obj = false;
		}

		return Ok(std::move(ret));
	}


	Paragraph::Paragraph(std::vector<zst::SharedPtr<InlineObject>> objs) : m_contents(std::move(objs))
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
}
