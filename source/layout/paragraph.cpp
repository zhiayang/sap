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
		this->words.push_back(std::move(word));
	}
}
