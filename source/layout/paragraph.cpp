// paragraph.cpp
// Copyright (c) 2021, zhiayang
// SPDX-License-Identifier: Apache-2.0

#include "sap.h"

#include "pdf/font.h"
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
		for(auto& word : m_words)
			word.computeMetrics(parent_style);

		return zst::Ok<std::optional<LayoutObject*>>(std::nullopt);
	}
}
