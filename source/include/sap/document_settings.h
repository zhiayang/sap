// document_settings.h
// Copyright (c) 2022, zhiayang
// SPDX-License-Identifier: Apache-2.0

#pragma once

#include "sap/units.h"
#include "sap/font_family.h"

namespace sap
{
	struct DocumentSettings
	{
		struct Margins
		{
			std::optional<DynLength> top;
			std::optional<DynLength> bottom;
			std::optional<DynLength> left;
			std::optional<DynLength> right;
		};

		std::optional<DynLength> font_size;
		std::optional<FontFamily> font_family;
		std::optional<DynLength2d> paper_size;
		std::optional<Margins> margins;

		std::optional<DynLength> paragraph_spacing;
		std::optional<double> line_spacing;
		std::optional<double> sentence_space_stretch;
	};
}
