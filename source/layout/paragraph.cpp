// paragraph.cpp
// Copyright (c) 2021, zhiayang
// SPDX-License-Identifier: Apache-2.0

#include "sap.h"

#include "pdf/page.h"
#include "pdf/font.h"
#include "pdf/text.h"
#include "pdf/units.h"

namespace sap
{
	void Paragraph::add(Word word)
	{
		word.m_paragraph = this;
		m_words.push_back(std::move(word));
	}

	zst::Result<std::optional<LayoutObject*>, int> Paragraph::layout(LayoutRegion* region, const Style* parent_style)
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

		// clear the old word positions. in theory, we should not try to layout something twice...
		if(m_word_positions.size() > 0)
			sap::warn("layout/paragraph", "layout() called twice on paragraph");

		m_word_positions.clear();
		sap::Scalar line_height {};

		region->addObjectAtCursor(this);

		size_t word_idx = 0;
		for(; word_idx < m_words.size(); word_idx++)
		{
			auto& word = m_words[word_idx];

			zpr::println("word = '{}', cursor = {}", word.text, region->cursor());

			// if there's not enough space, break.
			if(!region->haveSpaceAtCursor(word.size))
			{
				// break by moving x to 0, and y by the line height.
				region->advanceCursorBy({ -region->cursor().x(), line_height });
				line_height = {};

				if(!region->haveSpaceAtCursor(word.size))
				{
					auto overflow = util::make<Paragraph>();
					overflow->m_words.insert(overflow->m_words.end(),
						std::move_iterator(m_words.begin() + word_idx),
						std::move_iterator(m_words.end()));

					m_words.erase(m_words.begin() + word_idx, m_words.end());
					for(auto& w : overflow->m_words)
						w.m_paragraph = overflow;

					return zst::Ok(overflow);
				}
			}

			m_word_positions.push_back(region->cursor());
			region->advanceCursorBy(Offset2d(word.size.x(), Scalar(0)));

			line_height = dim::max(line_height, word.size.y());
		}

		return zst::Ok<std::optional<LayoutObject*>>(std::nullopt);
	}


	void Paragraph::render(const LayoutRegion* region, Position position, pdf::Page* page) const
	{
		(void) region;

		if(m_words.size() != m_word_positions.size())
			sap::error("layout/paragraph", "word position count mismatch");

		auto text = util::make<pdf::Text>();
		for(size_t i = 0; i < m_words.size(); i++)
		{
			auto word_pos = (position + m_word_positions[i]).into(pdf::Position2d_YDown{});

			text->moveAbs(page->convertVector2(word_pos));
			m_words[i].render(text);
		}

		page->addObject(text);
	}
}
