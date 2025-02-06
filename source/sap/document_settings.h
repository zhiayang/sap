// document_settings.h
// Copyright (c) 2022, yuki / zhiayang
// SPDX-License-Identifier: Apache-2.0

#pragma once

#include "sap/units.h"
#include "sap/style.h"
#include "sap/font_family.h"

namespace sap
{
	struct DocumentSettings
	{
		static Length DEFAULT_FONT_SIZE;

		struct Margins
		{
			std::optional<DynLength> top;
			std::optional<DynLength> bottom;
			std::optional<DynLength> left;
			std::optional<DynLength> right;
		};

		std::optional<FontFamily> serif_font_family;
		std::optional<FontFamily> sans_font_family;
		std::optional<FontFamily> mono_font_family;

		std::optional<DynLength2d> paper_size;
		std::optional<Margins> margins;
		std::optional<Style> default_style;
	};
}
