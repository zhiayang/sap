// fontconfig.cpp
// Copyright (c) 2022, zhiayang
// SPDX-License-Identifier: Apache-2.0

#if defined(USE_FONTCONFIG) && USE_FONTCONFIG != 0

#include "defs.h"
#include "util.h"

#include "font/search.h"

#include <fontconfig/fontconfig.h> // for FcChar8, FcPatternDestroy, FcConf...

namespace font::fontconfig
{
	using sap::Ok;
	using sap::Err;
	using sap::ErrFmt;
	using sap::StrErrorOr;


	static FcConfig* get_fontconfig_config()
	{
		static FcConfig* config = FcInitLoadConfigAndFonts();
		return config;
	}

	template <typename T>
	static StrErrorOr<T> get_value(FcPattern* pattern, const char* key)
	{
		if constexpr(std::is_same_v<T, bool>)
		{
			FcBool value = false;
			auto res = FcPatternGetBool(pattern, key, 0, &value);
			if(res != FcResultMatch)
				return ErrFmt("error: {}", (int) res);

			return Ok<bool>(value);
		}
		else if constexpr(std::is_same_v<T, int>)
		{
			int value = 0;
			auto res = FcPatternGetInteger(pattern, key, 0, &value);
			if(res != FcResultMatch)
				return ErrFmt("error: {}", (int) res);

			return Ok<int>(value);
		}
		else if constexpr(std::is_same_v<T, double>)
		{
			double value = 0;
			auto res = FcPatternGetDouble(pattern, key, 0, &value);
			if(res != FcResultMatch)
				return ErrFmt("error: {}", (int) res);

			return Ok<double>(value);
		}
		else if constexpr(std::is_same_v<T, std::string>)
		{
			FcChar8* value = 0;
			auto res = FcPatternGetString(pattern, key, 0, &value);
			if(res != FcResultMatch)
				return ErrFmt("error: {}", (int) res);

			return Ok(std::string((char*) value));
		}
		else
		{
			static_assert(not std::is_same_v<T, T>, "unknown type");
		}
	}


	static StrErrorOr<FontHandle> get_handle_from_pattern(FcPattern* pattern)
	{
		auto display_name = TRY(get_value<std::string>(pattern, FC_FULLNAME));
		auto postscript_name = TRY(get_value<std::string>(pattern, FC_POSTSCRIPT_NAME));
		auto path = TRY(get_value<std::string>(pattern, FC_FILE));

		FontProperties properties {};

		auto fc_width = TRY(get_value<int>(pattern, FC_WIDTH));

		switch(fc_width)
		{
			case FC_WIDTH_ULTRACONDENSED: properties.stretch = FontStretch::ULTRA_CONDENSED; break;
			case FC_WIDTH_EXTRACONDENSED: properties.stretch = FontStretch::EXTRA_CONDENSED; break;
			case FC_WIDTH_CONDENSED: properties.stretch = FontStretch::CONDENSED; break;
			case FC_WIDTH_SEMICONDENSED: properties.stretch = FontStretch::SEMI_CONDENSED; break;
			case FC_WIDTH_NORMAL: properties.stretch = FontStretch::NORMAL; break;
			case FC_WIDTH_SEMIEXPANDED: properties.stretch = FontStretch::SEMI_EXPANDED; break;
			case FC_WIDTH_EXPANDED: properties.stretch = FontStretch::EXPANDED; break;
			case FC_WIDTH_EXTRAEXPANDED: properties.stretch = FontStretch::EXTRA_EXPANDED; break;
			case FC_WIDTH_ULTRAEXPANDED: properties.stretch = FontStretch::ULTRA_EXPANDED; break;
			default: properties.stretch = fc_width / 100.0; break;
		}

		if(auto fc_slant = TRY(get_value<int>(pattern, FC_SLANT)); fc_slant == FC_SLANT_ROMAN)
			properties.style = FontStyle::NORMAL;
		else
			properties.style = FontStyle::ITALIC;

		auto fc_weight = TRY(get_value<int>(pattern, FC_WEIGHT));
		properties.weight = static_cast<FontWeight>(FcWeightToOpenTypeDouble(fc_weight));

		return Ok(FontHandle {
		    .display_name = std::move(display_name),
		    .postscript_name = std::move(postscript_name),
		    .properties = std::move(properties),
		    .path = std::move(path),
		});
	}

	static FcPattern* get_default_pattern()
	{
		return FcPatternCreate();
	}

	static FcObjectSet* get_default_objectset()
	{
		auto ret = FcObjectSetCreate();

		FcObjectSetAdd(ret, FC_FAMILY);
		FcObjectSetAdd(ret, FC_FULLNAME);
		FcObjectSetAdd(ret, FC_POSTSCRIPT_NAME);
		FcObjectSetAdd(ret, FC_STYLE);
		FcObjectSetAdd(ret, FC_SLANT);
		FcObjectSetAdd(ret, FC_WEIGHT);
		FcObjectSetAdd(ret, FC_FONTFORMAT);

		return ret;
	}


	static FcFontSet* get_fc_fontset()
	{
		auto fc_config = get_fontconfig_config();
		auto fc_pattern = get_default_pattern();
		auto fc_object_set = get_default_objectset();

		auto ret = FcFontList(fc_config, fc_pattern, fc_object_set);

		FcPatternDestroy(fc_pattern);
		FcObjectSetDestroy(fc_object_set);

		return ret;
	}

	static std::optional<FontHandle> select_best_from_fontset(FcFontSet* list, FontProperties properties)
	{
		std::vector<FontHandle> candidates {};
		for(int i = 0; i < list->nfont; i++)
		{
			if(auto handle = get_handle_from_pattern(list->fonts[i]); handle.ok())
				candidates.push_back(handle.take_value());
		}

		FcFontSetDestroy(list);
		return getBestFontWithProperties(std::move(candidates), std::move(properties));
	}


	std::vector<FontHandle> get_all_fonts()
	{
		auto list = get_fc_fontset();

		std::vector<FontHandle> ret;
		for(int i = 0; i < list->nfont; i++)
		{
			if(auto handle = get_handle_from_pattern(list->fonts[i]); handle.ok())
				ret.push_back(handle.take_value());
		}

		FcFontSetDestroy(list);
		return ret;
	}

	std::vector<std::string> get_all_typefaces()
	{
		auto list = get_fc_fontset();

		std::vector<std::string> typefaces;
		for(int i = 0; i < list->nfont; i++)
		{
			if(auto f = get_value<std::string>(list->fonts[i], FC_FAMILY); f.ok())
				typefaces.push_back(f.take_value());
		}

		FcFontSetDestroy(list);
		return typefaces;
	}

	std::optional<FontHandle> search_for_font_with_postscript_name(zst::str_view postscript_name)
	{
		auto fc_config = get_fontconfig_config();
		auto fc_pattern = get_default_pattern();

		// note: fontconfig takes a copy of the string, so it's ok that it's a temporary
		FcPatternAddString(fc_pattern, FC_POSTSCRIPT_NAME, (const FcChar8*) postscript_name.str().c_str());

		FcDefaultSubstitute(fc_pattern);
		FcConfigSubstitute(fc_config, fc_pattern, FcMatchPattern);

		FcResult fc_result {};
		auto list = FcFontSort(fc_config, fc_pattern, false, nullptr, &fc_result);
		auto _ = util::Defer([&]() {
			FcPatternDestroy(fc_pattern);
			FcFontSetDestroy(list);
		});

		if(fc_result != FcResultMatch)
		{
			sap::warn("fontconfig", "FcFontSort returned no matches");
			return std::nullopt;
		}


		for(int i = 0; i < list->nfont; i++)
		{
			auto ps_name = get_value<std::string>(list->fonts[i], FC_POSTSCRIPT_NAME);
			if(not ps_name.ok() || *ps_name != postscript_name)
				continue;

			auto handle = get_handle_from_pattern(list->fonts[i]);
			if(not handle.ok())
				continue;

			return *handle;
		}

		return std::nullopt;
	}

	static std::optional<FontHandle> search_for_font(zst::str_view typeface_name, FontProperties properties, bool is_generic)
	{
		auto fc_config = get_fontconfig_config();
		auto fc_pattern = get_default_pattern();

		// note: fontconfig takes a copy of the string, so it's ok that it's a temporary
		FcPatternAddString(fc_pattern, FC_FAMILY, (const FcChar8*) typeface_name.str().c_str());

		FcDefaultSubstitute(fc_pattern);
		FcConfigSubstitute(fc_config, fc_pattern, FcMatchPattern);

		FcResult fc_result {};
		auto list = FcFontSort(fc_config, fc_pattern, false, nullptr, &fc_result);
		if(fc_result != FcResultMatch)
		{
			sap::warn("fontconfig", "FcFontSort returned no matches");
			return std::nullopt;
		}

		FcPatternDestroy(fc_pattern);

		auto actual_matches = FcFontSetCreate();
		for(int i = 0; i < list->nfont; i++)
		{
			auto name = get_value<std::string>(list->fonts[i], FC_FAMILY);
			if(not name.ok())
				continue;

			if(is_generic)
			{
				if(i > 0 && *get_value<std::string>(list->fonts[i - 1], FC_FAMILY) != *name)
					break;
			}
			else if(*name != typeface_name)
			{
				break;
			}

			FcFontSetAdd(actual_matches, list->fonts[i]);
		}

		FcFontSetDestroy(list);
		return select_best_from_fontset(actual_matches, std::move(properties));
	}



	std::optional<FontHandle> search_for_font(zst::str_view typeface_name, FontProperties properties)
	{
		return search_for_font(typeface_name, std::move(properties), /* is_generic: */ false);
	}

	std::optional<FontHandle> get_font_for_generic_name(zst::str_view name, FontProperties properties)
	{
		if(name == GENERIC_SERIF)
			return search_for_font("serif", properties, /* is_generic: */ true);
		else if(name == GENERIC_SANS_SERIF)
			return search_for_font("sans-serif", properties, /* is_generic: */ true);
		else if(name == GENERIC_MONOSPACE)
			return search_for_font("monospace", properties, /* is_generic: */ true);
		else if(name == GENERIC_EMOJI)
			return search_for_font("emoji", properties, /* is_generic: */ true);
		else
			return std::nullopt;
	}
}

#endif
