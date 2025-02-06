// destination.h
// Copyright (c) 2022, yuki / zhiayang
// SPDX-License-Identifier: Apache-2.0

#pragma once

#include "pdf/units.h"

namespace pdf
{
	struct File;
	struct Dictionary;

	struct Destination
	{
		size_t page;
		double zoom;
		pdf::Position2d position;
	};
}
