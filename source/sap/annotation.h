// annotation.h
// Copyright (c) 2022, yuki
// SPDX-License-Identifier: Apache-2.0

#pragma once

#include "layout/cursor.h"

namespace sap
{
	struct OutlineItem
	{
		std::string title;
		layout::AbsolutePagePos position;
		std::vector<OutlineItem> children;
	};

	struct LinkAnnotation
	{
		layout::AbsolutePagePos position;
		Size2d size;

		layout::AbsolutePagePos destination;
	};
}
