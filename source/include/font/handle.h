// handle.h
// Copyright (c) 2022, zhiayang
// SPDX-License-Identifier: Apache-2.0

#pragma once

namespace font
{
	enum class FontStyle
	{
		NORMAL = 0,
		ITALIC = 1,
	};

	enum class FontWeight : int
	{
		THIN = 100,
		EXTRA_LIGHT = 200,
		LIGHT = 300,
		NORMAL = 400,
		MEDIUM = 500,
		SEMIBOLD = 600,
		BOLD = 700,
		EXTRA_BOLD = 800,
		BLACK = 900,
	};

	namespace FontStretch
	{
		constexpr inline double ULTRA_CONDENSED = 0.5;
		constexpr inline double EXTRA_CONDENSED = 0.625;
		constexpr inline double CONDENSED = 0.75;
		constexpr inline double SEMI_CONDENSED = 0.875;
		constexpr inline double NORMAL = 1.0;
		constexpr inline double SEMI_EXPANDED = 1.125;
		constexpr inline double EXPANDED = 1.25;
		constexpr inline double EXTRA_EXPANDED = 1.5;
		constexpr inline double ULTRA_EXPANDED = 2.0;
	}

	struct FontProperties
	{
		FontStyle style = FontStyle::NORMAL;
		FontWeight weight = FontWeight::NORMAL;
		double stretch = FontStretch::NORMAL;

		bool operator==(const FontProperties&) const = default;
		size_t hash() const { return util::hasher::combine(0, this->style, this->weight, this->stretch); }
	};

	struct FontHandle
	{
		std::string display_name;
		std::string postscript_name;
		FontProperties properties;

		std::filesystem::path path;

		size_t hash() const
		{
			return util::hasher::combine(0, this->display_name, this->postscript_name, this->properties, this->path);
		}

		bool operator==(const FontHandle&) const = default;
	};

	struct FontFamily
	{
		FontHandle normal;
		FontHandle bold;
		FontHandle italic;
		FontHandle bold_italic;
	};
}
