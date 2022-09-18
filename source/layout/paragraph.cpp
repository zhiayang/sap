// paragraph.cpp
// Copyright (c) 2021, zhiayang
// SPDX-License-Identifier: Apache-2.0

#include "sap.h"

#include "pdf/page.h"
#include "pdf/font.h"
#include "pdf/text.h"
#include "pdf/units.h"

namespace sap::layout
{
	void Paragraph::add(Word word)
	{
		word.m_paragraph = this;
		m_words.push_back(std::move(word));
	}

	zst::Result<std::optional<LayoutObject*>, int> Paragraph::layout(interp::Interpreter* cs, LayoutRegion* region,
		const Style* parent_style)
	{
		// first, all words need their metrics computed. forward the default style down to them.
		auto combined = Style::combine(m_style, parent_style);
		for(auto& word : m_words)
			word.computeMetrics(combined);

		/*
		    now, start placing words. we take linespacing into account, but for words where the height
		    is larger than the line spacing, that particular line advances by the word height. otherwise,
		    the linespacing is not changed.

		    of course, this is currently a very 3head algorithm, and obviously we'll have a better one
		    in due time. won't be a very good typesetting program if the text layout is trash.
		*/

		region->addObjectAtCursor(this);

		Position cursor {};
		auto region_width = region->spaceAtCursor().x();
		auto region_height = region->spaceAtCursor().y();

		/*
		    Currently, this algorithm only handles justified text, and will fill the entire horizontal
		    extent of the layout region with text, except for possibly the last line.

		    the general idea is as follows: as we iterate through each word, we increase the required
		    space, as well as the number of words so far. Note that words do not account for interword spaces
		    in their size (that is for us to find out and potentially squeeze/stretch).

		    we assume that the word-breaking algorithm simply splits on horizontal whitespace (tabs, spaces),
		    and that we treat those the same way. So, given N words, we will have N-1 inter-word spaces.

		    We define a minimum and maximum spacing ratio, which is how much the `space_length` will be multiplied
		    by at the end. For example, a ratio of 1.1 means that inter-word spaces will be expanded by 10%.

		    For each word, we initially check the remaining space, and then calculate the required expansion ratio
		    to fill that space. If it is too large, then we add another word. If adding that word makes the ratio
		    worse than the current ratio (eg. the next word is super long), then we break here. (TODO: here, we should
		    hyphenate!)

		    This basically continues till we either consume all words or run out of vertical space in the
		    layout region.
		*/
		Scalar word_length {};    // the amount of space required for words
		Scalar space_length {};   // ... for inter-word spaces
		size_t words_in_line = 0; // the number of words in the line so far.

		constexpr double MIN_RATIO = 0.9;

		auto finalise_line = [&](size_t word_idx, double ratio) {
			sap::Scalar line_height {};
			for(size_t i = 0; i < words_in_line; i++)
			{
				auto& word = m_words[word_idx - words_in_line + i];
				word.m_position = cursor;
				word.m_post_space_ratio = ratio;
				cursor.x() += word.size.x() + (ratio * word.spaceWidth());

				line_height = dim::max(line_height, word.size.y());

				if(i + 1 == words_in_line)
					word.m_linebreak_after = true;
			}

			cursor.y() += line_height;
			cursor.x() = Scalar(0);
		};

		Paragraph* overflow = nullptr;
		size_t word_idx = 0;
		while(word_idx < m_words.size())
		{
			auto& word = m_words[word_idx];
			word.m_linebreak_after = false;
			word.m_next_word = nullptr;

			// if we have no words so far, just take a word always (don't carelessly divide by 0)
			if(words_in_line == 0)
			{
				// check if we have enough vertical and horizontal space for this word. In theory,
				// we should always have horizontal space (since we're at the start of the line), but
				// we might not have vertical space.
				if(region_height - cursor.y() < word.size.y())
				{
					// no space -- break the rest of the words, and quit.
					overflow = util::make<Paragraph>();
					overflow->m_words.insert(overflow->m_words.end(), std::move_iterator(m_words.begin() + word_idx),
						std::move_iterator(m_words.end()));

					m_words.erase(m_words.begin() + word_idx, m_words.end());
					for(auto& w : overflow->m_words)
						w.m_paragraph = overflow;

					break;
				}

				// ok, we have vertical space. assume we have horizontal space (TODO: if we don't, that means
				// a single word is very long -- we want to hyphenate in these cases!)
				word_length += word.size.x();
				words_in_line++;
			}
			else
			{
				// `region_width - word_length` is how much "real" whitespace we have in the line,
				// and `space_length` is how much "space character whitespace" we have.
				auto current_ratio = (region_width - word_length) / space_length;

				// the space between words is, for now, the maximum of the prev -> cur, and cur -> next
				// (if the current word is of a different font size, then the distinction becomes important)
				auto space_between = dim::max(m_words[word_idx - 1].spaceWidth(), word.spaceWidth());

				// check the ratio again with an additional word
				auto new_ratio = (region_width - word_length - word.size.x()) / (space_length + space_between);

				// if the new ratio is not worse than the current one,
				// and it doesn't squeeze the space too much, add it.
				if(MIN_RATIO <= new_ratio && std::abs(new_ratio - 1.0) <= std::abs(current_ratio - 1.0) &&
					word_length + word.size.x() < region_width)
				{
					word_length += word.size.x();
					space_length += space_between;
					words_in_line++;
				}
				else
				{
					// otherwise, break to the next line. in breaking to the next line, we need to set the positions
					// of all the words on this line now that we have the ratio and stuff.
					finalise_line(word_idx, current_ratio);

					words_in_line = 0;
					word_length = Scalar(0);
					space_length = Scalar(0);

					// don't increment the thing
					continue;
				}
			}

			word_idx += 1;
		}

		if(words_in_line > 0)
			finalise_line(word_idx, 1.0);

		// set the next words
		for(size_t i = 0; i < m_words.size(); i++)
		{
			if(i + 1 < m_words.size())
				m_words[i].m_next_word = &m_words[i + 1];
		}

		region->advanceCursorBy(cursor);

		if(overflow)
			return zst::Ok(std::optional(overflow));
		else
			return zst::Ok(std::nullopt);

		/*
		    TODO: This algorithm is actually wrong

		    it doesn't account for kerning between
		    (a) the last glyph of a word and the proceeding space
		    (b) the space before a word, and its first glyph

		    the difference is usually small, but we should strive to follow the font whenever possible.
		    we probably want to do the computation here, at layout time, so we can accurately compute
		    the *real* width of the line, after all the adjustments.
		*/
	}


	void Paragraph::render(interp::Interpreter* cs, const LayoutRegion* region, Position abs_para_position, pdf::Page* page) const
	{
		(void) region;

		/*
		    here, `abs_para_position` (as the name suggests) is the absolute position of the paragraph
		    within the page. The word positions are stored relative to the start of the paragraph,
		    which means it's a simple addition to get the absolute position of the word.
		*/
		auto text = util::make<pdf::Text>();
		text->moveAbs(page->convertVector2(abs_para_position.into(pdf::Position2d_YDown {})));

		/*
		    not sure if this is legit, but words basically don't actually use their own `m_position` when
		    rendering; we just pass the words to the PDF, and the viewer uses the font metrics to adjust.

		    we do use the computed position when advancing to the next line, to account for line spacing
		    differences if there's more than one font-size on a line.
		*/
		Position current_pos {};
		for(size_t i = 0; i < m_words.size(); i++)
		{
			current_pos = m_words[i].m_position;
			m_words[i].render(text);

			if(m_words[i].m_linebreak_after && i + 1 < m_words.size())
			{
				auto skip = m_words[i + 1].m_position.y() - current_pos.y();
				text->nextLine(pdf::Offset2d(0, -1.0 * skip.into(pdf::Scalar {}).value()));
			}
		}

		page->addObject(text);
	}
}
