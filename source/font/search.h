// search.h
// Copyright (c) 2022, yuki / zhiayang
// SPDX-License-Identifier: Apache-2.0

#pragma once

#include <string>
#include <optional>
#include <filesystem>

#include "font/handle.h"

namespace font
{
	/*
	    returns all the installed fonts on the system.
	*/
	std::vector<FontHandle> getAllFonts();

	/*
	    Returns the names of all typefaces installed on the system. This is distinct from getAllFonts(),
	    which might return separate FontHandles for each variation (eg. bold, italic) of a typeface.

	    This can also be thought of as returning a list of font family names.
	*/
	std::vector<std::string> getAllTypefaces();

	/*
	    search for a font in the given typeface (family) with the given properties
	*/
	std::optional<FontHandle> searchForFont(zst::str_view typeface_name, FontProperties properties);

	/*
	    search for a specific font by its unique postscript name
	*/
	std::optional<FontHandle> searchForFontWithPostscriptName(zst::str_view postscript_name);

	/*
	    select the best font matching the properties
	*/
	std::optional<FontHandle> getBestFontWithProperties(std::vector<FontHandle> fonts, FontProperties properties);


	constexpr inline const char* GENERIC_SERIF = "serif";
	constexpr inline const char* GENERIC_SANS_SERIF = "sans-serif";
	constexpr inline const char* GENERIC_MONOSPACE = "monospace";
	constexpr inline const char* GENERIC_EMOJI = "emoji";

	std::optional<FontHandle> findFont(const std::vector<std::string>& typefaces, FontProperties properties);
}
