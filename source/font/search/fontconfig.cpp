// fontconfig.cpp
// Copyright (c) 2022, zhiayang
// SPDX-License-Identifier: Apache-2.0

#ifdef USE_FONTCONFIG

#include <fontconfig/fontconfig.h> // for FcChar8, FcPatternDestroy, FcConf...

namespace font
{
	std::optional<std::string> findFontWithFontConfig(std::vector<std::string> families, const std::string& style,
	    std::vector<std::string> fontformats)
	{
		FcConfig* config = FcInitLoadConfigAndFonts();
		FcPattern* pattern = FcPatternCreate();

		for(const auto& family : families)
		{
			FcValue value { .type = FcTypeString, .u = { .s = (const FcChar8*) family.c_str() } };
			FcPatternAdd(pattern, FC_FAMILY, value, FcTrue);
		}

		// For now only support files ending in .?tf (specifically, not .ttc). This covers .otf and .ttf files.
		// This is more specific than fontformat=CFF,TrueType since that's about the format of the inner font,
		// but ttc files are "collections", which we don't support.
		{
			FcValue value { .type = FcTypeString, .u = { .s = (const FcChar8*) "*.?tf" } };
			FcPatternAdd(pattern, FC_FILE, value, FcTrue);
		}

		{
			FcValue value { .type = FcTypeString, .u = { .s = (const FcChar8*) style.c_str() } };
			FcPatternAdd(pattern, FC_STYLE, value, FcTrue);
		}

		for(const auto& fontformat : fontformats)
		{
			FcValue value { .type = FcTypeString, .u = { .s = (const FcChar8*) fontformat.c_str() } };
			FcPatternAdd(pattern, FC_FONTFORMAT, value, FcTrue);
		}

		// Fill in defaults (user config defaults)
		if(FcConfigSubstitute(config, pattern, FcMatchPattern) == FcFalse)
		{
			FcPatternDestroy(pattern);
			FcConfigDestroy(config);
			return std::nullopt;
		}
		// Fill in defaults for fields that are still blank
		FcDefaultSubstitute(pattern);

		// Fill in the fields that are fuzzy matches with the actual font's details
		FcResult result;
		FcPattern* best_pattern = FcFontMatch(config, pattern, &result);
		FcPatternDestroy(pattern);
		if(result != FcResultMatch)
		{
			FcPatternDestroy(best_pattern);
			FcConfigDestroy(config);
			return std::nullopt;
		}

		// One of those details is the filename, which is what we want
		FcValue value;
		FcPatternGet(best_pattern, FC_FILE, 0, &value);
		if(value.type != FcTypeString)
		{
			FcPatternDestroy(best_pattern);
			FcConfigDestroy(config);
			return std::nullopt;
		}

		// Return the path
		std::string path((const char*) value.u.s);
		FcPatternDestroy(best_pattern);
		FcConfigDestroy(config);
		return path;
	}
}
#endif
