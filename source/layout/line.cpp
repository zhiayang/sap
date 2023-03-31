// line.cpp
// Copyright (c) 2022, zhiayang
// SPDX-License-Identifier: Apache-2.0

#include "pdf/text.h"

#include "layout/base.h"
#include "layout/line.h"
#include "layout/linebreak.h"

namespace sap::layout
{
	static std::tuple<Length, Length, Length> calculate_word_size(zst::wstr_view text, const Style* style)
	{
		auto x = style->font()->getWordSize(text, style->font_size().into()).x();

		auto& fm = style->font()->getFontMetrics();
		auto asc = style->font()->scaleMetricForFontSize(fm.typo_ascent, style->font_size().into());
		auto dsc = style->font()->scaleMetricForFontSize(fm.typo_descent, style->font_size().into());

		return { x.into(), asc.into(), dsc.into() };
	}

	SepWidths calculateSeparatorWidths(const tree::Separator* sep, const Style* style, bool is_end_of_line)
	{
		double multiplier = sep->isSentenceEnding() ? style->sentence_space_stretch() : 1.0;

		auto sep_str = is_end_of_line ? sep->endOfLine() : sep->middleOfLine();
		auto ret = style->font()->getWordSize(sep_str, style->font_size().into()).x();

		return {
			.actual = ret.into(),
			.preferred = (ret * multiplier).into(),
		};
	}

	LineMetrics computeLineMetrics(std::span<const std::unique_ptr<tree::InlineObject>> objs, const Style* parent_style)
	{
		LineMetrics ret {};

		struct WordChunk
		{
			Length width = 0;
			std::u32string text;
			const Style* style = nullptr;
		};

		WordChunk word_chunk {};
		auto cur_chunk_begin = objs.begin();

		for(auto it = objs.begin(); it != objs.end(); ++it)
		{
			if(auto word = dynamic_cast<const tree::Text*>(it->get()); word)
			{
				auto style = parent_style->extendWith(word->style());
				if(it != cur_chunk_begin && word_chunk.style != style)
				{
					ret.total_word_width += word_chunk.width;

					word_chunk.width = 0;
					word_chunk.style = style;
					word_chunk.text.clear();
					cur_chunk_begin = it;
				}

				word_chunk.text += word->contents();

				auto [chunk_x, chunk_asc, chunk_dsc] = calculate_word_size(word_chunk.text, style);

				ret.widths.push_back(chunk_x - word_chunk.width);

				word_chunk.width = chunk_x;

				ret.ascent_height = std::max(ret.ascent_height, chunk_asc * style->line_spacing());
				ret.descent_height = std::max(ret.descent_height, chunk_dsc * style->line_spacing());

				auto font_line_spacing = style->font()->getFontMetrics().default_line_spacing;
				auto style_line_spacing = style->line_spacing()
				                        * style->font()->scaleMetricForFontSize(font_line_spacing,
											style->font_size().into());

				ret.default_line_spacing = std::max(ret.default_line_spacing, style_line_spacing.into<Length>());
			}
			else if(auto sep = dynamic_cast<const tree::Separator*>(it->get()); sep)
			{
				ret.total_word_width += word_chunk.width;

				word_chunk.width = 0;
				word_chunk.style = nullptr;
				word_chunk.text.clear();

				cur_chunk_begin = it + 1;
				bool is_end_of_line = it + 1 == objs.end();


				auto sep_widths = [&]() -> SepWidths {
					if(it == objs.begin() && it + 1 == objs.end())
						sap::internal_error("??? line with only separator");

					auto calc = [sep, is_end_of_line, parent_style](auto iter) {
						return calculateSeparatorWidths(sep, parent_style->extendWith((*iter)->style()),
							is_end_of_line);
					};

					if(it == objs.begin())
						return calc(it + 1);

					else if(it + 1 == objs.end())
						return calc(it - 1);

					auto a = calc(it - 1);
					auto b = calc(it + 1);
					return {
						.actual = std::max(a.actual, b.actual),
						.preferred = std::max(a.preferred, b.preferred),
					};
				}();

				if(sep->isSpace() || sep->isSentenceEnding())
					ret.total_space_width += sep_widths.preferred;
				else
					ret.total_word_width += sep_widths.preferred;

				ret.widths.push_back(sep_widths.actual);
			}
			else
			{
				// NOTE: decide how inline objects should be laid out -- is (0, 0) at the baseline? does this mean that
				// images "drop down" from the line, instead of going up? weirdge idk.
				sap::internal_error("unsupported inline object!");
			}
		}

		ret.total_word_width += word_chunk.width;
		return ret;
	}


	Line::Line(const Style* style, //
		LayoutSize size,
		LineMetrics metrics,
		std::vector<std::unique_ptr<LayoutObject>> objs)
		: LayoutObject(style, size), m_metrics(std::move(metrics)), m_objects(std::move(objs))
	{
	}

	std::unique_ptr<Line> Line::fromInlineObjects(interp::Interpreter* cs,
		const Style* style,
		std::span<const std::unique_ptr<tree::InlineObject>> objs,
		const LineMetrics& line_metrics,
		Size2d available_space,
		bool is_first_line,
		bool is_last_line)
	{
		auto total_width = line_metrics.total_word_width + line_metrics.total_space_width;

		auto desired_space_width = available_space.x() - line_metrics.total_word_width;
		double space_width_factor = desired_space_width / line_metrics.total_space_width;

		std::vector<std::unique_ptr<LayoutObject>> layout_objects {};

		const Style* prev_word_style = Style::empty();
		Length current_offset = 0;

		for(size_t i = 0; i < objs.size(); i++)
		{
			auto obj_width = line_metrics.widths[i];

			if(auto tree_word = dynamic_cast<const tree::Text*>(objs[i].get()); tree_word != nullptr)
			{
				auto word_size = LayoutSize {
					.width = obj_width,
					.ascent = line_metrics.ascent_height,
					.descent = line_metrics.descent_height,
				};

				auto word = std::make_unique<Word>(tree_word->contents(), style->extendWith(tree_word->style()),
					current_offset, word_size);

				tree_word->m_generated_layout_object = word.get();

				prev_word_style = word->style();
				layout_objects.push_back(std::move(word));

				current_offset += obj_width;
			}
			else if(auto tree_sep = dynamic_cast<const tree::Separator*>(objs[i].get()); tree_sep != nullptr)
			{
				auto sep_size = LayoutSize {
					.width = obj_width,
					.ascent = line_metrics.ascent_height,
					.descent = line_metrics.descent_height,
				};

				bool is_end_of_line = i + 1 == objs.size();
				auto sep_widths = calculateSeparatorWidths(tree_sep, style->extendWith(tree_sep->style()),
					is_end_of_line);

				auto orig_offset = current_offset;

				if(style->alignment() == Alignment::Justified
					&& (not is_last_line || (0.9 <= space_width_factor && space_width_factor <= 1.1)))
				{
					current_offset += sep_widths.preferred * space_width_factor;
				}
				else
				{
					current_offset += sep_widths.preferred;
				}

				auto sep = std::make_unique<Word>(is_end_of_line ? tree_sep->endOfLine() : tree_sep->middleOfLine(),
					style->extendWith(tree_sep->style()), orig_offset, sep_size);

				tree_sep->m_generated_layout_object = sep.get();

				prev_word_style = sep->style();
				layout_objects.push_back(std::move(sep));
			}
			else
			{
				sap::internal_error("unsupported!");
			}
		}

		auto layout_size = LayoutSize {
			.width = total_width,
			.ascent = line_metrics.ascent_height,
			.descent = line_metrics.default_line_spacing - line_metrics.ascent_height,
		};

		return std::unique_ptr<Line>(new Line(style, layout_size, line_metrics, std::move(layout_objects)));
	}


	layout::PageCursor Line::positionChildren(layout::PageCursor cursor)
	{
		// for now, lines only contain words; words are already positioned with their relative thingies.
		using enum Alignment;

		auto horz_space = cursor.widthAtCursor();
		auto self_width = m_layout_size.width;

		switch(m_style->alignment())
		{
			case Left:
			case Justified: break;

			case Right: {
				cursor = cursor.moveRight(horz_space - self_width);
				break;
			}

			case Centre: {
				cursor = cursor.moveRight((horz_space - self_width) / 2);
				break;
			}
		}

		this->positionRelatively(cursor.position());
		return cursor.newLine(m_layout_size.descent);
	}


	void Line::render_impl(const LayoutBase* layout, std::vector<pdf::Page*>& pages) const
	{
		// optimise for words, so we don't make new PDF text objects (TJ) for every word
		struct PrevWord
		{
			pdf::Text* pdf_text;
			Length word_end;
		};

		std::optional<PrevWord> prev_word {};

		auto line_pos = this->resolveAbsPosition(layout);

		for(auto& obj : m_objects)
		{
			if(auto word = dynamic_cast<const Word*>(obj.get()); word != nullptr)
			{
				bool is_first = not prev_word.has_value();
				if(is_first)
				{
					prev_word = PrevWord {
						.pdf_text = util::make<pdf::Text>(),
						.word_end = {},
					};
				}

				auto offset_from_prev = word->relativeOffset() - prev_word->word_end;
				word->render(line_pos, pages, prev_word->pdf_text, is_first, offset_from_prev);

				prev_word->word_end = word->relativeOffset() + word->layoutSize().width /* - word->extraWidth()*/;
			}
			else
			{
				zpr::println("warning: non-word in line");

				// if we encountered a non-word, reset the text (we can't go back to the same text!)
				prev_word = std::nullopt;
				obj->render(layout, pages);
			}
		}
	}
}
