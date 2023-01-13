// paragraph.cpp
// Copyright (c) 2022, zhiayang
// SPDX-License-Identifier: Apache-2.0

#include <utf8proc/utf8proc.h>

#include "misc/hyphenator.h"

#include "interp/ast.h"         // for FunctionCall
#include "interp/value.h"       // for Value
#include "interp/interp.h"      // for Interpreter
#include "interp/basedefs.h"    // for InlineObject
#include "interp/eval_result.h" // for EvalResult

#include "layout/paragraph.h"

namespace sap::tree
{
	void Paragraph::evaluateScripts(interp::Interpreter* cs)
	{
		std::vector<std::unique_ptr<InlineObject>> ret {};
		ret.reserve(m_contents.size());

		for(auto& obj : m_contents)
		{
			if(auto word = dynamic_cast<tree::Text*>(obj.get()); word != nullptr)
			{
				ret.push_back(std::move(obj));
			}
			else if(auto iscr = dynamic_cast<tree::ScriptCall*>(obj.get()); iscr != nullptr)
			{
				auto value_or_err = cs->run(iscr->call.get());
				if(value_or_err.is_err())
					error("interp", "evaluation failed: {}", value_or_err.error());

				auto value_or_empty = value_or_err.take_value();
				if(not value_or_empty.hasValue())
				{
					ret.emplace_back(new tree::Text(U"", obj->style()));
					continue;
				}

				if(value_or_empty.get().isTreeBlockObj())
					error("interp", "TODO: unsupported block object in paragraph!");

				auto tmp = cs->evaluator().convertValueToText(std::move(value_or_empty).take());
				if(tmp.is_err())
					error("interp", "convertion to text failed: {}", tmp.error());

				auto objs = tmp.take_value();
				ret.insert(ret.end(), std::move_iterator(objs.begin()), std::move_iterator(objs.end()));
			}
			else if(auto iscb = dynamic_cast<tree::ScriptBlock*>(obj.get()); iscb != nullptr)
			{
				error("interp", "script block unsupported");
			}
			else
			{
				error("interp", "unsupported");
			}
		}

		m_contents.swap(ret);
	}

	static bool is_letter(char32_t c)
	{
		auto category = utf8proc_category((utf8proc_int32_t) c);
		return util::is_one_of(category, UTF8PROC_CATEGORY_LU, UTF8PROC_CATEGORY_LL, UTF8PROC_CATEGORY_LT, UTF8PROC_CATEGORY_LM,
		    UTF8PROC_CATEGORY_LO);
	}


	static void make_separators_for_word(std::vector<std::unique_ptr<InlineObject>>& vec, std::unique_ptr<Text> text)
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
			vec.push_back(std::make_unique<Text>(pre_chunk.str(), text->style()));

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
			for(size_t i = 0; (i = orig_span.find(U'-')) != (size_t) -1;)
			{
				auto part = orig_span.take_prefix(i + 1);
				vec.push_back(std::make_unique<Text>(part.str(), text->style()));
				vec.push_back(std::make_unique<Separator>(Separator::BREAK_POINT));
			}

			vec.push_back(std::make_unique<Text>(orig_span.str(), text->style()));
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
					// hyphenation preference increases from 1 to 3 to 5, so 5 has the lowest cost, and 1 the highest.
					auto part = orig_span.take_prefix(k);
					vec.push_back(std::make_unique<Text>(part.str(), text->style()));
					vec.push_back(std::make_unique<Separator>(Separator::HYPHENATION_POINT, /* cost: */ 6 - points[i]));
					k = 1;
				}
			}

			if(orig_span.size() > 0)
				vec.push_back(std::make_unique<Text>(orig_span.str(), text->style()));
		}


		if(not post_chunk.empty())
			vec.push_back(std::make_unique<Text>(post_chunk.str(), text->style()));
	}



	void Paragraph::processWordSeparators()
	{
		std::vector<std::unique_ptr<InlineObject>> ret {};

		bool first_obj = true;
		bool seen_whitespace = false;
		for(auto& uwu : m_contents)
		{
			auto tree_text = dynamic_cast<tree::Text*>(uwu.get());
			assert(tree_text != nullptr);

			auto current_text = std::make_unique<Text>(U"");
			current_text->setStyle(tree_text->style());

			for(char32_t c : tree_text->contents())
			{
				if(c == U' ' || c == U'\t' || c == U'\n' || c == U'\r')
				{
					if(not seen_whitespace)
					{
						if(not current_text->contents().empty())
						{
							make_separators_for_word(ret, std::move(current_text));
							ret.push_back(std::make_unique<Separator>(Separator::SPACE));

							current_text = std::make_unique<Text>(U"");
							current_text->setStyle(tree_text->style());
						}
						else if(not first_obj)
						{
							ret.push_back(std::make_unique<Separator>(Separator::SPACE));
						}

						seen_whitespace = true;
					}
					else
					{
						// do nothing (consume more whitespace)
					}
				}
				else
				{
					current_text->contents() += c;
					seen_whitespace = false;
				}
			}

			if(not current_text->contents().empty())
				ret.push_back(std::move(current_text));

			first_obj = false;
		}

		m_contents.swap(ret);
	}


	Paragraph::Paragraph(std::vector<std::unique_ptr<InlineObject>> objs) : m_contents(std::move(objs))
	{
	}

	void Paragraph::addObject(std::unique_ptr<InlineObject> obj)
	{
		m_contents.push_back(std::move(obj));
	}

	void Paragraph::addObjects(std::vector<std::unique_ptr<InlineObject>> objs)
	{
		m_contents.insert(m_contents.end(), std::move_iterator(objs.begin()), std::move_iterator(objs.end()));
	}

	auto Paragraph::getLayoutFunction() const -> std::optional<LayoutFn>
	{
		return &layout::Paragraph::fromTree;
	}
}
