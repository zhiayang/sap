// page.cpp
// Copyright (c) 2021, zhiayang
// SPDX-License-Identifier: Apache-2.0

#include "sap.h"
#include "pdf/page.h"

namespace sap
{
	std::optional<Paragraph> Page::add(Paragraph para)
	{
		// para.computeMetrics(

		return { };
	}

	pdf::Page* Page::finalise()
	{
		return util::make<pdf::Page>();
	}
}
