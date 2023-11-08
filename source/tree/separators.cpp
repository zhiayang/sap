// separators.cpp
// Copyright (c) 2022, zhiayang
// SPDX-License-Identifier: Apache-2.0

#include <utf8proc/utf8proc.h>

#include "sap/paths.h"
#include "tree/base.h"
#include "tree/paragraph.h"
#include "misc/hyphenator.h"

namespace sap::tree
{
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
		static auto hyphenator = []() {
			auto resolved = paths::resolveLibrary(Location::builtin(), "data/hyphenation/hyph-en-gb.tex");
			if(resolved.is_err())
				resolved.error().showAndExit();

			return hyph::Hyphenator::parseFromFile(resolved.unwrap());
		}();

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


	using InlineObjVec = std::vector<zst::SharedPtr<InlineObject>>;

	ErrorOr<InlineObjVec> performReplacements(const Style& parent_style, InlineObjVec input)
	{
		std::vector<zst::SharedPtr<InlineObject>> ret {};
		ret.reserve(input.size());

		bool last_sep_was_space = true;
		for(size_t i = 0; i < input.size(); i++)
		{
			if(auto sep = input[i]->castToSeparator())
			{
				last_sep_was_space = sep->hasWhitespace();
				ret.push_back(std::move(input[i]));
			}
			else if(auto text = input[i]->castToText())
			{
				auto style = parent_style.extendWith(text->style());
				if(not style.smart_quotes_enabled())
				{
					ret.push_back(std::move(input[i]));
					last_sep_was_space = false;
					continue;
				}

				auto& txt_ref = text->contents();
				auto k = txt_ref.find_first_of(U"\"\'");
				if(k == std::string::npos)
				{
					ret.push_back(std::move(input[i]));
					last_sep_was_space = false;
					continue;
				}

				auto txt = txt_ref;

				do
				{
					auto ch = txt[k];

					// replace the first.
					if(k == 0 && last_sep_was_space)
						txt[k] = (ch == U'\'' ? U'\u2018' : U'\u201C'); // left quote
					else
						txt[k] = (ch == U'\'' ? U'\u2019' : U'\u201D'); // right quotes for everything else

					k = txt.find_first_of(U"\"\'", k);
				} while(k != std::string::npos);

				auto new_text = new_thing_from_existing<Text>(input[i], std::move(txt));

				ret.push_back(std::move(new_text));
				last_sep_was_space = false;
			}
			else if(auto span = input[i]->castToSpan())
			{
				span->objects() = TRY(performReplacements(parent_style.extendWith(span->style()),
				    std::move(span->objects())));

				// drill down as deep as possible
				static constexpr bool (*last_span_obj_is_space)(const InlineObjVec&) = [](const auto& objs) {
					if(objs.empty())
						return false;
					const auto& obj = objs.back();

					if(auto s = obj->castToSpan())
						return last_span_obj_is_space(s->objects());
					else if(auto ss = obj->castToSeparator())
						return ss->hasWhitespace();
					else
						return false;
				};

				last_sep_was_space = last_span_obj_is_space(span->objects());
				ret.push_back(std::move(input[i]));
			}
			else
			{
				sap::internal_error("unsupported thing");
			}
		}

		return OkMove(ret);
	}



	ErrorOr<InlineObjVec> processWordSeparators(InlineObjVec input)
	{
		std::vector<zst::SharedPtr<InlineObject>> ret {};
		ret.reserve(input.size());

		bool first_obj = true;
		bool seen_whitespace = false;
		for(auto& uwu : input)
		{
			if(uwu->isSeparator())
			{
				ret.push_back(std::move(uwu));
				continue;
			}
			else if(auto span = uwu->castToSpan())
			{
				// note: don't make a new span here, preserve the identities.
				// if we ref a span, it needs to keep existing.
				auto objs = TRY(processWordSeparators(std::move(span->objects())));

				span->objects().clear();
				span->addObjects(std::move(objs));

				ret.push_back(std::move(uwu));
				seen_whitespace = false;
				first_obj = false;
				continue;
			}

			auto tree_text = uwu->castToText();
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

		return OkMove(ret);
	}
}
