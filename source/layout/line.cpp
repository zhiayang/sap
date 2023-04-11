// line.cpp
// Copyright (c) 2022, zhiayang
// SPDX-License-Identifier: Apache-2.0

#include "pdf/text.h"

#include "tree/wrappers.h"

#include "layout/base.h"
#include "layout/line.h"
#include "layout/linebreak.h"

namespace sap::layout
{
	struct WordSize
	{
		Length width;
		Length ascent;
		Length descent;
		Length default_line_spacing;
		Length cap_height;
	};

	static WordSize calculate_word_size(zst::wstr_view text, const Style* style)
	{
		auto x = style->font()->getWordSize(text, style->font_size().into()).x();

		auto& fm = style->font()->getFontMetrics();
		auto asc = style->font()->scaleMetricForFontSize(fm.hhea_ascent, style->font_size().into()).abs();
		auto dsc = style->font()->scaleMetricForFontSize(fm.hhea_descent, style->font_size().into()).abs();
		auto dls = style->font()->scaleMetricForFontSize(fm.default_line_spacing, style->font_size().into());
		auto ch = style->font()->scaleMetricForFontSize(fm.cap_height, style->font_size().into());

		return {
			.width = x.into(),
			.ascent = asc.into(),
			.descent = dsc.into(),
			.default_line_spacing = dls.into(),
			.cap_height = ch.into(),
		};
	}

	static Length calculate_preferred_sep_width(const tree::Separator* sep, const Style* style, bool is_end_of_line)
	{
		double multiplier = sep->isSentenceEnding() ? style->sentence_space_stretch() : 1.0;

		auto sep_str = is_end_of_line ? sep->endOfLine() : sep->middleOfLine();
		auto ret = style->font()->getWordSize(sep_str, style->font_size().into()).x();

		return (ret * multiplier).into();
	}

	Length calculatePreferredSeparatorWidth(const tree::Separator* sep,
	    bool is_end_of_line,
	    const Style* left_style,
	    const Style* right_style)
	{
		auto a = calculate_preferred_sep_width(sep, left_style, is_end_of_line);
		auto b = calculate_preferred_sep_width(sep, right_style, is_end_of_line);

		return (a + b) / 2.0;
	}

	static Length calculateRealSeparatorWidth(const tree::Separator* sep, const Style* style, bool is_end_of_line)
	{
		auto sep_str = is_end_of_line ? sep->endOfLine() : sep->middleOfLine();
		return style->font()->getWordSize(sep_str, style->font_size().into()).x().into();
	}

	LineMetrics computeLineMetrics(std::span<const zst::SharedPtr<tree::InlineObject>> objs, const Style* parent_style)
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

		auto reset_chunk = [&]() {
			ret.total_word_width += word_chunk.width;

			word_chunk.style = nullptr;
			word_chunk.width = 0;
			word_chunk.text.clear();
		};

		for(auto it = objs.begin(); it != objs.end(); ++it)
		{
			if(auto word = (*it)->castToText())
			{
				auto style = parent_style->extendWith(word->style());
				if(it != cur_chunk_begin && word_chunk.style != style)
				{
					reset_chunk();
					word_chunk.style = style;
					cur_chunk_begin = it;
				}

				word_chunk.text += word->contents();

				auto word_size = calculate_word_size(word_chunk.text, style);

				ret.widths.push_back(word_size.width - word_chunk.width);
				word_chunk.width = word_size.width;

				ret.cap_height = std::max(ret.cap_height, word_size.cap_height);
				ret.ascent_height = std::max(ret.ascent_height, word_size.ascent);
				ret.descent_height = std::max(ret.descent_height, word_size.descent);

				ret.default_line_spacing = std::max(ret.default_line_spacing,
				    word_size.default_line_spacing * style->line_spacing());
			}
			else if(auto span = (*it)->castToSpan())
			{
				reset_chunk();
				cur_chunk_begin = it + 1;

				auto style = parent_style->extendWith(span->style());
				auto span_metrics = computeLineMetrics(span->objects(), style);

				ret.cap_height = std::max(ret.cap_height, span_metrics.cap_height);
				ret.ascent_height = std::max(ret.ascent_height, span_metrics.ascent_height);
				ret.descent_height = std::max(ret.descent_height, span_metrics.descent_height);
				ret.default_line_spacing = std::max(ret.default_line_spacing, span_metrics.default_line_spacing);

				if(span->hasOverriddenWidth())
				{
					auto span_width = *span->getOverriddenWidth();

					ret.total_word_width += span_width;

					ret.widths.push_back(span_width);
					ret.nested_span_metrics.push_back(std::move(span_metrics));
				}
				else
				{
					ret.total_word_width += span_metrics.total_word_width;
					ret.total_space_width += span_metrics.total_space_width;

					std::move(span_metrics.widths.begin(), span_metrics.widths.end(), std::back_inserter(ret.widths));

					std::move(span_metrics.preferred_sep_widths.begin(), span_metrics.preferred_sep_widths.end(),
					    std::back_inserter(ret.preferred_sep_widths));

					std::move(span_metrics.nested_span_metrics.begin(), span_metrics.nested_span_metrics.end(),
					    std::back_inserter(ret.nested_span_metrics));
				}
			}
			else if(auto sep = (*it)->castToSeparator())
			{
				reset_chunk();
				cur_chunk_begin = it + 1;

				bool is_end_of_line = it + 1 == objs.end();

				auto real_sep_width = calculateRealSeparatorWidth(sep, parent_style->extendWith((*it)->style()),
				    is_end_of_line);

				auto pref_sep_width = [&]() -> Length {
					if(it == objs.begin() && it + 1 == objs.end())
						sap::internal_error("??? line with only separator");

					auto calc_one = [sep, is_end_of_line, parent_style](auto iter) {
						return calculate_preferred_sep_width(sep, parent_style->extendWith((*iter)->style()),
						    is_end_of_line);
					};

					if(it == objs.begin())
						return calc_one(it + 1);

					else if(it + 1 == objs.end())
						return calc_one(it - 1);

					auto left_style = parent_style->extendWith((*(it - 1))->style());
					auto right_style = parent_style->extendWith((*(it + 1))->style());
					return calculatePreferredSeparatorWidth(sep, is_end_of_line, left_style, right_style);
				}();

				if(sep->isSpace() || sep->isSentenceEnding())
					ret.total_space_width += pref_sep_width;
				else
					ret.total_word_width += pref_sep_width;

				ret.widths.push_back(real_sep_width);
				ret.preferred_sep_widths.push_back(pref_sep_width);
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


	static Length compute_word_offsets_for_span(size_t& metrics_idx,
	    size_t& sep_idx,
	    size_t& nested_span_idx,
	    std::span<const zst::SharedPtr<tree::InlineObject>> objs,
	    Length width,
	    const Style* parent_style,
	    Length& current_offset,
	    const LineMetrics& metrics,
	    bool is_last_line,
	    bool is_last_span,
	    std::optional<double> outer_space_width_factor,
	    std::vector<std::unique_ptr<LayoutObject>>& layout_objects)
	{
		const auto space_width_factor = outer_space_width_factor.value_or(
		    (width - metrics.total_word_width) / metrics.total_space_width);

		// for centred and right-aligned, we need to calculate the left offset.
		if(parent_style->alignment() == Alignment::Right || parent_style->alignment() == Alignment::Centre)
		{
			auto extra_space = width - (metrics.total_word_width + metrics.total_space_width);
			if(parent_style->alignment() == Alignment::Right)
				current_offset += extra_space;
			else
				current_offset += extra_space / 2;
		}

		/*
		    to (try and) preserve the "structure" of the text, we create LayoutSpans for
		    runs of objects that come from the same span. this allows them to get various
		    attributes that were set on their original tree-span, even if the words
		    were broken over a few lines.
		*/

		Length actual_width = 0;

		Length cur_span_width = 0;
		Length cur_span_offset = current_offset;
		tree::InlineSpan* cur_span = nullptr;

		auto make_layout_span = [&metrics, &layout_objects](Length offset, Length width, const tree::InlineSpan* span) {
			if(not span)
				return;

			// make a new span.
			auto ps = std::make_unique<LayoutSpan>(offset, span->raiseHeight(),
			    LayoutSize {
			        .width = width,
			        .ascent = metrics.ascent_height,
			        .descent = metrics.descent_height,
			    },
			    metrics);

			ps->setLinkDestination(span->linkDestination());

			span->addGeneratedLayoutSpan(ps.get());
			layout_objects.push_back(std::move(ps));
		};

		auto add_to_layout_span = [&](const tree::InlineObject* obj, Length width) {
			if(obj->parentSpan() == nullptr)
			{
				cur_span = nullptr;
				cur_span_width = 0;

				cur_span_offset = current_offset + width;
				return;
			}

			if(obj->parentSpan() == cur_span)
			{
				cur_span_width += width;
			}
			else if(cur_span == nullptr)
			{
				cur_span = obj->parentSpan();

				cur_span_offset = current_offset;
				cur_span_width = width;
			}
			else
			{
				assert(obj->parentSpan() != cur_span);

				make_layout_span(cur_span_offset, cur_span_width, cur_span);
				cur_span = obj->parentSpan();

				cur_span_offset = current_offset;
				cur_span_width = width;
			}
		};

		for(size_t i = 0; i < objs.size(); i++)
		{
			auto obj_width = metrics.widths[metrics_idx];

			auto* obj = objs[i].get();
			auto style = parent_style->extendWith(obj->style());

			if(auto tree_word = obj->castToText())
			{
				auto word_size = LayoutSize {
					.width = obj_width,
					.ascent = metrics.ascent_height,
					.descent = metrics.descent_height,
				};

				auto word = std::make_unique<Word>(tree_word->contents(), style, current_offset,
				    tree_word->raiseHeight(), word_size);

				tree_word->setGeneratedLayoutObject(word.get());

				layout_objects.push_back(std::move(word));

				add_to_layout_span(obj, obj_width);

				current_offset += obj_width;
				actual_width += obj_width;
				metrics_idx += 1;
			}
			else if(auto tree_sep = obj->castToSeparator())
			{
				auto preferred_sep_width = metrics.preferred_sep_widths[sep_idx++];

				auto sep_size = LayoutSize {
					.width = obj_width,
					.ascent = metrics.ascent_height,
					.descent = metrics.descent_height,
				};

				Length actual_sep_width = 0;

				if(style->alignment() == Alignment::Justified
				    && (not is_last_line || (0.9 <= space_width_factor && space_width_factor <= 1.1)))
				{
					actual_sep_width = preferred_sep_width * space_width_factor;
				}
				else
				{
					actual_sep_width = preferred_sep_width;
				}

				bool is_end_of_line = is_last_span && i + 1 == objs.size();

				auto sep = std::make_unique<Word>(is_end_of_line ? tree_sep->endOfLine() : tree_sep->middleOfLine(),
				    style->extendWith(tree_sep->style()), current_offset, tree_sep->raiseHeight(), sep_size);

				tree_sep->setGeneratedLayoutObject(sep.get());
				layout_objects.push_back(std::move(sep));

				add_to_layout_span(obj, actual_sep_width);

				current_offset += actual_sep_width;
				actual_width += actual_sep_width;
				metrics_idx += 1;
			}
			else if(auto tree_span = obj->castToSpan())
			{
				if(tree_span->hasOverriddenWidth())
				{
					// make a copy, because we manually adjust current_offset
					// by the overridden width later.
					size_t inner_metrics_idx = 0;
					size_t inner_sep_idx = 0;
					size_t inner_nested_span_idx = 0;
					Length cur_ofs = current_offset;

					make_layout_span(cur_ofs, obj_width, tree_span);

					compute_word_offsets_for_span(                    //
					    inner_metrics_idx,                            //
					    inner_sep_idx,                                //
					    inner_nested_span_idx,                        //
					    tree_span->objects(),                         //
					    obj_width,                                    //
					    style,                                        //
					    cur_ofs,                                      //
					    metrics.nested_span_metrics[nested_span_idx], //
					    is_last_line,                                 //
					    /* is last span: */ i + 1 == objs.size(),     //
					    /* outer_space_width_factor: */ std::nullopt, // let the inner span calculate the space width
					                                                  // factor, since it's fixed width
					    layout_objects);

					nested_span_idx += 1;

					auto span_width = *tree_span->getOverriddenWidth();

					current_offset += span_width;
					actual_width += span_width;
					metrics_idx += 1;
				}
				else
				{
					auto old_ofs = current_offset;

					// don't make a copy here
					auto span_width = compute_word_offsets_for_span( //
					    metrics_idx,                                 //
					    sep_idx,                                     //
					    nested_span_idx,                             //
					    tree_span->objects(),                        //
					    width - current_offset,                      //
					    style,                                       //
					    current_offset,                              //
					    metrics,                                     //
					    is_last_line,                                //
					    /* is last span: */ i + 1 == objs.size(),    //
					    space_width_factor,                          // propagate our space width factor
					    layout_objects);

					actual_width += span_width;
					make_layout_span(old_ofs, span_width, tree_span);

					// don't modify metrics_idx here
				}
			}
			else
			{
				sap::internal_error("unsupported!");
			}
		}

		make_layout_span(cur_span_offset, cur_span_width, cur_span);
		return actual_width;
	}


	layout::PageCursor Line::compute_position_impl(layout::PageCursor cursor)
	{
		// for now, lines only contain words; words are already positioned with their relative thingies.
		this->positionRelatively(cursor.position());
		return cursor.newLine(m_metrics.descent_height);
	}



	std::unique_ptr<Line> Line::fromInlineObjects(interp::Interpreter* cs,
	    const Style* parent_style,
	    std::span<const zst::SharedPtr<tree::InlineObject>> objs,
	    const LineMetrics& line_metrics,
	    Size2d available_space,
	    bool is_first_line,
	    bool is_last_line)
	{
		std::vector<std::unique_ptr<LayoutObject>> layout_objects {};

		size_t metrics_idx = 0;
		size_t sep_idx = 0;
		size_t nested_span_idx = 0;
		Length current_offset = 0;

		auto actual_width = compute_word_offsets_for_span( //
		    metrics_idx,                                   //
		    sep_idx,                                       //
		    nested_span_idx,                               //
		    objs,                                          //
		    available_space.x(),                           //
		    parent_style,                                  //
		    current_offset,                                //
		    line_metrics,                                  //
		    is_last_line,                                  //
		    /* is last span: */ true,                      //
		    /* outer_space_width_factor: */ std::nullopt,  //
		    layout_objects);

		auto layout_size = LayoutSize {
			.width = actual_width,
			.ascent = line_metrics.ascent_height,
			.descent = line_metrics.descent_height,
		};

		return std::unique_ptr<Line>(new Line(parent_style, layout_size, line_metrics, std::move(layout_objects)));
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
			if(auto word = dynamic_cast<const Word*>(obj.get()))
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

				prev_word->word_end = word->relativeOffset() + word->layoutSize().width;
			}
			else if(auto ps = dynamic_cast<const LayoutSpan*>(obj.get()))
			{
				// do nothing
				auto ps_pos = line_pos;
				ps_pos.pos.x() += ps->relativeOffset();

				const_cast<LayoutSpan*>(ps)->positionAbsolutely(ps_pos);
				ps->render(layout, pages);
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
