// line.cpp
// Copyright (c) 2022, zhiayang
// SPDX-License-Identifier: Apache-2.0

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

	struct LineMetrics
	{
		sap::Length total_space_width;
		sap::Length total_word_width;
		std::vector<sap::Length> widths;

		sap::Length ascent_height;
		sap::Length descent_height;
		sap::Length default_line_spacing;
	};

	static LineMetrics compute_line_metrics(std::span<const std::unique_ptr<tree::InlineObject>> objs, const Style* parent_style)
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

				auto style_line_spacing = style->font()->scaleMetricForFontSize(
				    style->font()->getFontMetrics().default_line_spacing, style->font_size().into());

				ret.default_line_spacing = std::max(ret.default_line_spacing, style_line_spacing.into<Length>());
			}
			else if(auto sep = dynamic_cast<const tree::Separator*>(it->get()); sep)
			{
				ret.total_word_width += word_chunk.width;

				word_chunk.width = 0;
				word_chunk.style = nullptr;
				word_chunk.text.clear();

				cur_chunk_begin = it + 1;

				auto sep_char = (it + 1 == objs.end() ? sep->endOfLine() : sep->middleOfLine());

				auto sep_width = [&]() {
					if(it == objs.begin() && it + 1 == objs.end())
						sap::internal_error("??? line with only separator");

					if(it == objs.begin())
						return std::get<0>(calculate_word_size(sep_char, parent_style->extendWith((*(it + 1))->style())));
					else if(it + 1 == objs.end())
						return std::get<0>(calculate_word_size(sep_char, parent_style->extendWith((*(it - 1))->style())));

					return std::max(                                                                                //
					    std::get<0>(calculate_word_size(sep_char, parent_style->extendWith((*(it - 1))->style()))), //
					    std::get<0>(calculate_word_size(sep_char, parent_style->extendWith((*(it + 1))->style()))));
				}();

				if(sep->isSpace())
					ret.total_space_width += sep_width;
				else
					ret.total_word_width += sep_width;

				ret.widths.push_back(sep_width);
			}
			else
			{
				ret.total_word_width += word_chunk.width;

				word_chunk.width = 0;
				word_chunk.style = nullptr;
				word_chunk.text.clear();

				cur_chunk_begin = it + 1;

				ret.widths.push_back(it->get()->size().x());
			}
		}

		ret.total_word_width += word_chunk.width;
		return ret;
	}


	Line::Line(RelativePos pos, Size2d size, const Style* style, std::vector<std::unique_ptr<LayoutObject>> objs)
	    : LayoutObject(pos, size)
	    , m_objects(std::move(objs))
	{
		this->setStyle(style);
	}

	std::pair<PageCursor, std::unique_ptr<Line>> Line::fromInlineObjects(interp::Interpreter* cs,
	    PageCursor cursor,
	    const linebreak::BrokenLine& broken_line,
	    const Style* style,
	    std::span<const std::unique_ptr<tree::InlineObject>> objs,
	    bool is_last_line)
	{
		auto line_metrics = compute_line_metrics(objs, style);
		auto line_height = line_metrics.ascent_height + line_metrics.descent_height;

		cursor = cursor.newLine(line_metrics.ascent_height);
		auto start_position = cursor.position();

		auto total_width = line_metrics.total_word_width + line_metrics.total_space_width;

		auto desired_space_width = cursor.widthAtCursor() - line_metrics.total_word_width;
		double space_width_factor = desired_space_width / line_metrics.total_space_width;

		const Style* prev_word_style = Style::empty();

		std::vector<std::unique_ptr<LayoutObject>> layout_objects {};

		if(style->alignment() == Alignment::Centre)
		{
			auto left_offset = (cursor.widthAtCursor() - total_width) / 2;
			cursor = cursor.moveRight(left_offset);
		}
		else if(style->alignment() == Alignment::Right)
		{
			auto left_offset = cursor.widthAtCursor() - total_width;
			cursor = cursor.moveRight(left_offset);
		}


		for(size_t i = 0; i < objs.size(); i++)
		{
			auto obj_width = line_metrics.widths[i];
			auto new_cursor = cursor.moveRight(obj_width);

			if(auto tree_word = dynamic_cast<const tree::Text*>(objs[i].get()); tree_word != nullptr)
			{
				auto word = std::make_unique<Word>(tree_word->contents(), style->extendWith(tree_word->style()),
				    cursor.position(), Size2d(obj_width, line_height));

				prev_word_style = word->style();
				cursor = std::move(new_cursor);

				layout_objects.push_back(std::move(word));
			}
			else if(auto tree_sep = dynamic_cast<const tree::Separator*>(objs[i].get()); tree_sep != nullptr)
			{
				auto sep = std::make_unique<Word>(i + 1 == objs.size() ? tree_sep->endOfLine() : tree_sep->middleOfLine(),
				    style->extendWith(tree_sep->style()), cursor.position(), Size2d(obj_width, line_height));

				prev_word_style = sep->style();
				layout_objects.push_back(std::move(sep));

				if(style->alignment() == Alignment::Justified)
				{
					auto justified_shift = obj_width * space_width_factor;
					if(is_last_line)
					{
						if(0.9 <= space_width_factor && space_width_factor <= 1.1)
							cursor = cursor.moveRight(justified_shift);
						else
							cursor = cursor.moveRight(obj_width);
					}
					else
					{
						cursor = cursor.moveRight(justified_shift);
					}
				}
				else
				{
					cursor = cursor.moveRight(obj_width);
				}
			}
			else
			{
				sap::internal_error("unsupported!");
			}
		}

		auto vert_offset = line_metrics.default_line_spacing - line_metrics.ascent_height;
		return {
			cursor.newLine(vert_offset),
			std::unique_ptr<Line>(new Line(start_position, Size2d(total_width, line_height), style, std::move(layout_objects))),
		};
	}

	std::pair<PageCursor, std::optional<std::unique_ptr<Line>>> Line::fromBlockObjects(interp::Interpreter* cs, //
	    PageCursor cursor,
	    const Style* style,
	    std::span<tree::BlockObject*> objs)
	{
		cursor = cursor.newLine(0);
		auto start_position = cursor.position();

		auto tallest_obj = std::max_element(objs.begin(), objs.end(), [](auto& a, auto& b) {
			return a->size().y() < b->size().y();
		});

		auto line_height = (*tallest_obj)->size().y();
		auto total_width = std::accumulate(objs.begin(), objs.end(), Length(0), [](Length a, const auto& b) {
			return a + b->size().x();
		});

		std::vector<std::unique_ptr<LayoutObject>> layout_objects {};
		if(style->alignment() == Alignment::Centre)
		{
			auto left_offset = (cursor.widthAtCursor() - total_width) / 2;
			cursor = cursor.moveRight(left_offset);
		}
		else if(style->alignment() == Alignment::Right)
		{
			auto left_offset = cursor.widthAtCursor() - total_width;
			cursor = cursor.moveRight(left_offset);
		}


		for(size_t i = 0; i < objs.size(); i++)
		{
			auto layout_obj = objs[i]->createLayoutObject(cs, cursor, style);
			auto new_cursor = layout_obj.first;

			if(not layout_obj.second.has_value())
				continue;

			layout_objects.push_back(std::move(*layout_obj.second));
			cursor = std::move(new_cursor);
		}

		if(layout_objects.empty())
			return { cursor, std::nullopt };

		return {
			cursor,
			std::unique_ptr<Line>(new Line(start_position, //
			    Size2d(total_width, line_height),          //
			    style, std::move(layout_objects))),
		};
	}

	void Line::render(const LayoutBase* layout, std::vector<pdf::Page*>& pages) const
	{
		for(auto& obj : m_objects)
			obj->render(layout, pages);
	}
}
