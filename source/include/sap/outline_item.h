// outline_item.h
// Copyright (c) 2022, zhiayang
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
}
