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
		m_words.push_back(std::move(word));
	}

	void Paragraph::computeMetrics(const Style* parent_style)
	{
		for(auto& word : m_words)
			word.computeMetrics(parent_style);
	}
}
