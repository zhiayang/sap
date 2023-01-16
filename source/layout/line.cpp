// line.cpp
// Copyright (c) 2022, zhiayang
// SPDX-License-Identifier: Apache-2.0

#include "layout/base.h"
#include "layout/line.h"
#include "layout/linebreak.h"

namespace sap::layout
{
	static Size2d calculate_word_size(zst::wstr_view text, const Style* style)
	{
		return style->font()->getWordSize(text, style->font_size().into<pdf::PdfScalar>()).into<Size2d>();
	}

	struct LineMetrics
	{
		sap::Length total_space_width;
		sap::Length total_word_width;
		std::vector<sap::Length> widths;
		sap::Length line_height;
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

				auto chunk_size = calculate_word_size(word_chunk.text, style);
				ret.widths.push_back(chunk_size.x() - word_chunk.width);

				word_chunk.width = chunk_size.x();

				ret.line_height = std::max(ret.line_height, chunk_size.y() * style->line_spacing());
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
						return calculate_word_size(sep_char, parent_style->extendWith((*(it + 1))->style())).x();
					else if(it + 1 == objs.end())
						return calculate_word_size(sep_char, parent_style->extendWith((*(it - 1))->style())).x();

					return std::max(                                                                       //
					    calculate_word_size(sep_char, parent_style->extendWith((*(it - 1))->style())).x(), //
					    calculate_word_size(sep_char, parent_style->extendWith((*(it + 1))->style())).x());
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

	std::unique_ptr<Line> Line::fromInlineObjects(interp::Interpreter* cs,
	    LineCursor cursor,
	    const linebreak::BrokenLine& broken_line,
	    const Style* style,
	    std::span<const std::unique_ptr<tree::InlineObject>> objs)
	{
		cursor = cursor.newLine(0);
		auto start_position = cursor.position();

		auto line_metrics = compute_line_metrics(objs, style);
		auto line_height = line_metrics.line_height;

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
					cursor = cursor.moveRight(obj_width * space_width_factor);
				else
					cursor = cursor.moveRight(obj_width);
			}
			else
			{
				sap::internal_error("unsupported!");
			}
		}

		return std::unique_ptr<Line>(new Line(start_position, Size2d(total_width, line_metrics.line_height), style,
		    std::move(layout_objects)));
	}

	std::pair<LineCursor, std::unique_ptr<Line>> Line::fromBlockObjects(interp::Interpreter* cs, //
	    LineCursor cursor,
	    const Style* style,
	    std::span<const tree::BlockObject*> objs)
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
			auto new_cursor = cursor.moveRight(objs[i]->size().x());

			auto layout_obj = objs[i]->createLayoutObject(cs, cursor, style);
			if(not layout_obj.second.has_value())
				continue;

			layout_objects.push_back(std::move(*layout_obj.second));

			cursor = std::move(new_cursor);
		}

		cursor = cursor.newLine(line_height);

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
