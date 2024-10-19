// line.cpp
// Copyright (c) 2022, yuki / zhiayang
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
		Length cap_height;
		Length line_spacing;
	};

	static WordSize calculate_word_size(zst::wstr_view text, const Style& style)
	{
		auto x = style.font()->getWordSize(text, style.font_size().into()).x();

		auto& fm = style.font()->getFontMetrics();
		auto asc = style.font()->scaleMetricForFontSize(fm.hhea_ascent, style.font_size().into()).abs();
		auto dsc = style.font()->scaleMetricForFontSize(fm.hhea_descent, style.font_size().into()).abs();
		auto dls = style.font()->scaleMetricForFontSize(fm.default_line_spacing, style.font_size().into());
		auto ch = style.font()->scaleMetricForFontSize(fm.cap_height, style.font_size().into());

		return {
			.width = x.into(),
			.ascent = asc.into(),
			.descent = dsc.into(),
			.cap_height = ch.into(),
			.line_spacing = dls.into(),
		};
	}

	static Length calculate_preferred_sep_width(const tree::Separator* sep, const Style& style, bool is_end_of_line)
	{
		double multiplier = sep->isSentenceEnding() ? style.sentence_space_stretch() : 1.0;

		auto sep_str = is_end_of_line ? sep->endOfLine() : sep->middleOfLine();
		auto ret = style.font()->getWordSize(sep_str, style.font_size().into()).x();

		return (ret * multiplier).into();
	}

	Length calculatePreferredSeparatorWidth(const tree::Separator* sep,
	    bool is_end_of_line,
	    const Style& left_style,
	    const Style& right_style)
	{
		auto a = calculate_preferred_sep_width(sep, left_style, is_end_of_line);
		auto b = calculate_preferred_sep_width(sep, right_style, is_end_of_line);

		return (a + b) / 2.0;
	}

	static Length calculateRealSeparatorWidth(const tree::Separator* sep, const Style& style, bool is_end_of_line)
	{
		auto sep_str = is_end_of_line ? sep->endOfLine() : sep->middleOfLine();
		return style.font()->getWordSize(sep_str, style.font_size().into()).x().into();
	}

	Line::Line(const Style& style, //
	    LayoutSize size,
	    sap::Length line_spacing,
	    std::vector<std::unique_ptr<LayoutObject>> objs)
	    : LayoutObject(style, size), m_line_spacing(std::move(line_spacing)), m_objects(std::move(objs))
	{
	}


	struct WordChunk
	{
		std::u32string text;
		size_t num_objs = 0;

		Style style;
		Length width = 0;
		Length raise = 0;

		const tree::InlineSpan* span = nullptr;
	};

	struct NestedSpan;
	struct LineMetrics
	{
		sap::Length total_space_width;
		sap::Length total_word_width;

		Length ascent;
		Length descent;
		Length cap_height;
		Length line_spacing;

		struct Sep
		{
			Length real_width;
			Length preferred_width;

			Style style;
			std::u32string text;
		};

		enum ItemType : unsigned char
		{
			ITEM_WORD_CHUNK,
			ITEM_SEPARATOR,
			ITEM_SPAN,
		};

		// note: all of these should be iterated IN REVERSE
		std::vector<ItemType> item_types;

		std::vector<WordChunk> word_chunks;
		std::vector<Sep> separators;

		std::vector<NestedSpan> spans;
	};

	struct NestedSpan
	{
		std::span<const zst::SharedPtr<tree::InlineObject>> objs;
		Length width;
		Style style;
		LineMetrics metrics;
		bool fixed_width = false;
	};



	static LineMetrics process_line_objects( //
	    std::span<const zst::SharedPtr<tree::InlineObject>> objs,
	    const Style& parent_style,
	    bool is_last_span)
	{
		WordChunk word_chunk {};
		LineMetrics metrics {};

		auto flush_word_chunk_if_necessary = [&](size_t cur_obj_idx, const Style& obj_style, bool force_flush = false) {
			// if there's any style discontinuity (including raises), then we need to reset
			// the current word chunk and make another one.
			if(auto obj = objs[cur_obj_idx];
			    not force_flush
			    && (cur_obj_idx == 0
			        || (obj_style == word_chunk.style && obj->raiseHeight() == word_chunk.raise
			            && obj->parentSpan() == word_chunk.span)))
			{
				// not necessary; just return.
				return;
			}

			// if the word is empty, then there's also nothing to do.
			if(word_chunk.text.empty())
				return;
#if 0
			zpr::println("flush word '{}' because {}", word_chunk.text,
			    force_flush                     ? "forced"
			    : obj_style != word_chunk.style ? "styles"
			    : objs[cur_obj_idx]->raiseHeight() != word_chunk.raise
			        ? "raise"
			        : "span");
#endif
			auto word_size = calculate_word_size(word_chunk.text, word_chunk.style);
			word_chunk.width = word_size.width;

			metrics.total_word_width += word_size.width;

			metrics.cap_height = std::max(metrics.cap_height, word_size.cap_height);
			metrics.ascent = std::max(metrics.ascent, word_size.ascent);
			metrics.descent = std::max(metrics.descent, word_size.descent);
			metrics.line_spacing = std::max(metrics.line_spacing,
			    word_size.line_spacing * word_chunk.style.line_spacing());

			metrics.item_types.push_back(LineMetrics::ITEM_WORD_CHUNK);
			metrics.word_chunks.push_back(std::move(word_chunk));

			word_chunk = WordChunk();
		};


		for(size_t obj_idx = 0; obj_idx < objs.size(); obj_idx++)
		{
			auto* obj = objs[obj_idx].get();
			auto style = parent_style.extendWith(obj->style());

			if(auto tree_word = obj->castToText())
			{
				flush_word_chunk_if_necessary(obj_idx, style);

				word_chunk.style = std::move(style);
				word_chunk.raise = tree_word->raiseHeight();
				word_chunk.span = tree_word->parentSpan();

				word_chunk.text += tree_word->contents();
				word_chunk.num_objs += 1;
			}
			else if(auto tree_sep = obj->castToSeparator())
			{
				// if this separator doesn't take up space, then we don't do anything with it.
				bool is_end_of_line = is_last_span && obj_idx + 1 == objs.size();
				auto sep_str = is_end_of_line ? tree_sep->endOfLine() : tree_sep->middleOfLine();
				if(sep_str.empty())
				{
					word_chunk.num_objs += 1;
					continue;
				}

				flush_word_chunk_if_necessary(obj_idx, style, /* force: */ is_end_of_line || tree_sep->hasWhitespace());

				auto real_sep_width = calculateRealSeparatorWidth(tree_sep, style, is_end_of_line);

				auto preferred_sep_width = [&]() -> Length {
					if(obj_idx == 0 && obj_idx + 1 == objs.size())
						sap::internal_error("??? line with only separator");

					auto calc_one = [tree_sep, is_end_of_line, &parent_style, &objs](size_t k) {
						return calculate_preferred_sep_width(tree_sep, parent_style.extendWith(objs[k]->style()),
						    is_end_of_line);
					};

					if(obj_idx == 0)
						return calc_one(obj_idx + 1);

					else if(obj_idx + 1 == objs.size())
						return calc_one(obj_idx - 1);

					auto left_style = parent_style.extendWith(objs[obj_idx - 1]->style());
					auto right_style = parent_style.extendWith(objs[obj_idx + 1]->style());
					return calculatePreferredSeparatorWidth(tree_sep, is_end_of_line, left_style, right_style);
				}();

				metrics.item_types.push_back(LineMetrics::ITEM_SEPARATOR);
				metrics.separators.push_back(LineMetrics::Sep {
				    .real_width = real_sep_width,
				    .preferred_width = preferred_sep_width,
				    .style = std::move(style),
				    .text = sep_str.str(),
				});

				if(tree_sep->hasWhitespace())
					metrics.total_space_width += preferred_sep_width;
				else
					metrics.total_word_width += preferred_sep_width;
			}
			else if(auto tree_span = obj->castToSpan())
			{
				bool has_fixed_width = tree_span->hasOverriddenWidth();
				flush_word_chunk_if_necessary(obj_idx, style, /* force flush: */ has_fixed_width);

				auto span_metrics = process_line_objects(tree_span->objects(), style,
				    /* is last span: */ obj_idx + 1 == objs.size());

				metrics.cap_height = std::max(metrics.cap_height, span_metrics.cap_height);
				metrics.ascent = std::max(metrics.ascent, span_metrics.ascent);
				metrics.descent = std::max(metrics.descent, span_metrics.descent);
				metrics.line_spacing = std::max(metrics.line_spacing, span_metrics.line_spacing);

				if(has_fixed_width)
				{
					metrics.item_types.push_back(LineMetrics::ITEM_SPAN);
					metrics.spans.push_back({
					    .objs = tree_span->objects(),
					    .width = *tree_span->getOverriddenWidth(),
					    .style = std::move(style),
					    .metrics = std::move(span_metrics),
					    .fixed_width = true,
					});

					metrics.total_word_width += *tree_span->getOverriddenWidth();
				}
				else
				{
					metrics.total_word_width += span_metrics.total_word_width;
					metrics.total_space_width += span_metrics.total_space_width;

					metrics.item_types.push_back(LineMetrics::ITEM_SPAN);
					metrics.spans.push_back({
					    .objs = tree_span->objects(),
					    .width = 0,
					    .style = std::move(style),
					    .metrics = std::move(span_metrics),
					    .fixed_width = false,
					});
				}
			}
			else
			{
				sap::internal_error("unsupported!");
			}
		}

		if(not objs.empty())
		{
			flush_word_chunk_if_necessary(objs.size() - 1, parent_style.extendWith(objs.back()->style()),
			    /* force: */ true);
		}

		assert(metrics.item_types.size()
		       == metrics.separators.size() //
		              + metrics.word_chunks.size() + metrics.spans.size());

		std::reverse(metrics.item_types.begin(), metrics.item_types.end());
		std::reverse(metrics.separators.begin(), metrics.separators.end());
		std::reverse(metrics.word_chunks.begin(), metrics.word_chunks.end());
		std::reverse(metrics.spans.begin(), metrics.spans.end());
		return metrics;
	}



	static Length place_line_objects(LineMetrics& metrics,
	    std::span<const zst::SharedPtr<tree::InlineObject>> objs,
	    const Length _width,
	    const Style& parent_style,
	    Length& current_offset,
	    bool is_last_line,
	    bool is_last_span,
	    std::optional<double> outer_space_width_factor,
	    std::vector<std::unique_ptr<LayoutObject>>& layout_objects,
	    std::optional<LineAdjustment> line_adjustment)
	{
		const auto line_width = _width;
		const auto total_word_width = [&]() {
			if(auto horz_align = parent_style.horz_alignment();
			    line_adjustment.has_value() && horz_align != Alignment::Centre)
			{
				auto left = (horz_align == Alignment::Right ? 0 : line_adjustment->left_protrusion);
				auto right = (horz_align == Alignment::Left ? 0 : line_adjustment->right_protrusion);
				return metrics.total_word_width - left - right;
			}
			else
			{
				return metrics.total_word_width;
			}
		}();

		const auto space_width_factor = [&]() {
			if(outer_space_width_factor.has_value())
				return *outer_space_width_factor;

			return (line_width - total_word_width).abs() / metrics.total_space_width;
		}();

		if(line_adjustment.has_value())
			current_offset -= line_adjustment->left_protrusion;

		/*
		    to (try and) preserve the "structure" of the text, we create LayoutSpans for
		    runs of objects that come from the same span. this allows them to get various
		    attributes that were set on their original tree-span, even if the words
		    were broken over a few lines.
		*/

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
			        .ascent = metrics.ascent,
			        .descent = metrics.descent,
			    });

			ps->setLinkDestination(span->linkDestination());

			span->addGeneratedLayoutSpan(ps.get());
			layout_objects.push_back(std::move(ps));
		};

		auto add_to_layout_span = [&](tree::InlineSpan* parent_span, Length width) {
			if(parent_span == nullptr)
			{
				cur_span = nullptr;
				cur_span_width = 0;

				cur_span_offset = current_offset + width;
				return;
			}

			if(parent_span == cur_span)
			{
				cur_span_width += width;
			}
			else if(cur_span == nullptr)
			{
				cur_span = parent_span;

				cur_span_offset = current_offset;
				cur_span_width = width;
			}
			else
			{
				assert(parent_span != cur_span);

				make_layout_span(cur_span_offset, cur_span_width, cur_span);
				cur_span = parent_span;

				cur_span_offset = current_offset;
				cur_span_width = width;
			}
		};

		Length actual_width = 0;

		for(size_t obj_idx = 0; obj_idx < objs.size();)
		{
			auto tree_obj = objs[obj_idx];

			/*
			    if we're out of things, bail. this can happen if the line was empty (eg. it had a bunch
			    of words that turned out to have no text) so it's not necessarily a bug.
			*/
			if(metrics.item_types.empty())
				break;

			auto item_type = metrics.item_types.back();
			metrics.item_types.pop_back();

			// note that the metrics are reversed, so we pop them from the back.
			if(item_type == LineMetrics::ITEM_WORD_CHUNK)
			{
				auto word_chunk = std::move(metrics.word_chunks.back());
				metrics.word_chunks.pop_back();

				auto layout_word = std::make_unique<Word>(std::move(word_chunk.text), std::move(word_chunk.style),
				    current_offset, word_chunk.raise,
				    LayoutSize {
				        .width = word_chunk.width,
				        .ascent = metrics.ascent,
				        .descent = metrics.descent,
				    });

				// set the generated layout object for all the InlineObjects to the word we just made.
				// TODO: this might be problematic if we're doing multiple transformations on TIOs that end up
				// applying multiple times to the LayoutObject.
				for(size_t k = obj_idx; k < word_chunk.num_objs; k++)
					objs[k]->setGeneratedLayoutObject(layout_word.get());

				// we are guaranteed that all the objects in this current word_chunk are contiguous and have
				// the same style and the same parent span. this means we can manually add them to the current
				// span and just increase the combined span width by the total word width.
				add_to_layout_span(tree_obj->parentSpan(), word_chunk.width);

				layout_objects.push_back(std::move(layout_word));

				current_offset += word_chunk.width;
				actual_width += word_chunk.width;

				obj_idx += word_chunk.num_objs;
			}
			else if(item_type == LineMetrics::ITEM_SEPARATOR)
			{
				auto sep = std::move(metrics.separators.back());
				metrics.separators.pop_back();

				Length actual_sep_width = 0;

				if(parent_style.horz_alignment() == Alignment::Justified
				    && (not is_last_line || space_width_factor <= 1.1))
				{
					actual_sep_width = sep.preferred_width * space_width_factor;
				}
				else
				{
					actual_sep_width = sep.preferred_width;
				}

				if(not sep.text.empty())
				{
					auto layout_sep = std::make_unique<Word>( //
					    sep.text,                             //
					    std::move(sep.style),                 //
					    current_offset,                       //
					    tree_obj->raiseHeight(),              //
					    LayoutSize {
					        .width = sep.real_width,
					        .ascent = metrics.ascent,
					        .descent = metrics.descent,
					    });

					tree_obj->setGeneratedLayoutObject(layout_sep.get());
					layout_objects.push_back(std::move(layout_sep));
				}

				add_to_layout_span(tree_obj->parentSpan(), actual_sep_width);

				current_offset += actual_sep_width;
				actual_width += actual_sep_width;

				obj_idx += 1;
			}
			else if(item_type == LineMetrics::ITEM_SPAN)
			{
				auto nested_span = std::move(metrics.spans.back());
				metrics.spans.pop_back();

				if(nested_span.fixed_width)
				{
					Length cur_ofs = current_offset;
					auto span_width = nested_span.width;

					make_layout_span(cur_ofs, span_width, tree_obj->castToSpan());

					place_line_objects(nested_span.metrics,             //
					    nested_span.objs,                               //
					    span_width,                                     //
					    std::move(nested_span.style),                   //
					    cur_ofs,                                        //
					    is_last_line,                                   //
					    /* is last span: */ obj_idx + 1 == objs.size(), //
					    /* outer_space_width_factor: */ std::nullopt,   // let the inner span calculate the space width
					                                                    // factor, since it's fixed width
					    layout_objects,                                 //
					    std::nullopt                                    //
					);

					current_offset += span_width;
					actual_width += span_width;
				}
				else
				{
					auto old_ofs = current_offset;

					// don't make a copy here
					auto span_width = place_line_objects(nested_span.metrics, //
					    nested_span.objs,                                     //
					    line_width - current_offset,                          //
					    std::move(nested_span.style),                         //
					    current_offset,                                       //
					    is_last_line,                                         //
					    /* is last span: */ obj_idx + 1 == objs.size(),       //
					    space_width_factor,                                   // propagate our space width factor
					    layout_objects,                                       //
					    std::nullopt                                          //
					);

					actual_width += span_width;
					make_layout_span(old_ofs, span_width, tree_obj->castToSpan());
				}

				obj_idx += 1;
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
		return cursor.newLine(m_layout_size.descent);
	}



	std::unique_ptr<Line> Line::fromInlineObjects(interp::Interpreter* cs,
	    const Style& parent_style,
	    std::span<const zst::SharedPtr<tree::InlineObject>> objs,
	    Size2d available_space,
	    bool is_first_line,
	    bool is_last_line,
	    std::optional<LineAdjustment> line_adjustment)
	{
		std::vector<std::unique_ptr<LayoutObject>> layout_objects {};
		Length current_offset = 0;

		auto line_metrics = process_line_objects(objs, parent_style, /* is last span: */ true);

		auto actual_width = place_line_objects(line_metrics, //
		    objs,                                            //
		    available_space.x(),                             //
		    parent_style,                                    //
		    current_offset,                                  //
		    is_last_line,                                    //
		    /* is last span: */ true,                        //
		    /* outer_space_width_factor: */ std::nullopt,    //
		    layout_objects,                                  //
		    line_adjustment);

		auto layout_size = LayoutSize {
			.width = actual_width,
			.ascent = line_metrics.ascent,
			.descent = line_metrics.descent,
		};

		return std::unique_ptr<Line>(new Line(parent_style, layout_size, line_metrics.line_spacing,
		    std::move(layout_objects)));
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
						.word_end = 0,
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
