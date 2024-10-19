// search.cpp
// Copyright (c) 2022, yuki / zhiayang
// SPDX-License-Identifier: Apache-2.0

#include "font/search.h"
#include "font/font_file.h"

namespace font
{
	namespace coretext
	{
		extern std::vector<FontHandle> get_all_fonts();
		extern std::vector<std::string> get_all_typefaces();
		extern std::optional<FontHandle> search_for_font(zst::str_view typeface_name, FontProperties properties);
		extern std::optional<FontHandle> search_for_font_with_postscript_name(zst::str_view postscript_name);

		extern std::optional<FontHandle> get_font_for_generic_name(zst::str_view name, FontProperties properties);
	}

	namespace fontconfig
	{
		extern std::vector<FontHandle> get_all_fonts();
		extern std::vector<std::string> get_all_typefaces();
		extern std::optional<FontHandle> search_for_font(zst::str_view typeface_name, FontProperties properties);
		extern std::optional<FontHandle> search_for_font_with_postscript_name(zst::str_view postscript_name);

		extern std::optional<FontHandle> get_font_for_generic_name(zst::str_view name, FontProperties properties);
	}


	template <typename T>
	static std::vector<FontHandle> sorted_by(const std::vector<FontHandle>& fonts, //
	    T FontProperties::*property, T desired, bool* found_exact)
	{
		auto copy = std::vector(fonts.begin(), fonts.end());
		std::sort(copy.begin(), copy.end(), [&](const auto& a, const auto& b) {
			if(a.properties.*property == desired || b.properties.*property == desired)
				*found_exact = true;

			return a.properties.*property < b.properties.*property;
		});

		return copy;
	}

	template <typename T>
	static std::optional<T> search_ascending(const std::vector<FontHandle>& fonts, T FontProperties::*property, T desired)
	{
		auto it = std::lower_bound(fonts.begin(), fonts.end(), desired, [property](auto& font, T a) {
			return font.properties.*property < a;
		});
		if(it == fonts.end())
			return std::nullopt;
		else
			return it->properties.*property;
	}

	template <typename T>
	static std::optional<T> search_descending(const std::vector<FontHandle>& fonts, T FontProperties::*property, T desired)
	{
		auto it = std::upper_bound(fonts.begin(), fonts.end(), desired, [property](T a, auto& font) {
			return a < font.properties.*property;
		});
		if(it == fonts.end())
			return std::nullopt;
		else
			return it->properties.*property;
	}

	template <typename Cmp>
	static void erase_if(std::vector<FontHandle>& fonts, Cmp&& cmp)
	{
		auto it = std::remove_if(fonts.begin(), fonts.end(), cmp);
		fonts.erase(it, fonts.end());
	}


	std::optional<FontHandle> getBestFontWithProperties(std::vector<FontHandle> fonts_, FontProperties props)
	{
		// do the css thing: https://w3c.github.io/csswg-drafts/css-fonts/#font-matching-algorithm
		// CSS Fonts Module Level 4, Section 5 - Font Matching Algorithm

		bool have_exact_stretch = false;
		auto matching_set = sorted_by(fonts_, &FontProperties::stretch, props.stretch, &have_exact_stretch);

		double best_stretch = props.stretch;
		if(not have_exact_stretch)
		{
			// If there is no face which contains the desired value ...
			if(props.stretch <= FontStretch::NORMAL)
			{
				if(auto x = search_descending(matching_set, &FontProperties::stretch, props.stretch); x.has_value())
					best_stretch = *x;
				else
					best_stretch = *search_ascending(matching_set, &FontProperties::stretch, props.stretch);
			}
			else
			{
				if(auto x = search_ascending(matching_set, &FontProperties::stretch, props.stretch); x.has_value())
					best_stretch = *x;
				else
					best_stretch = *search_descending(matching_set, &FontProperties::stretch, props.stretch);
			}
		}

		// ... faces with width values which do not include the desired width value are removed from the matching set.
		// note: we modify desired_stretch if we can't find an exact match
		erase_if(matching_set, [best_stretch](const auto& font) {
			return font.properties.stretch != best_stretch;
		});

		if(matching_set.empty())
			return std::nullopt;

		// ... font-style is tried next ...
		// note: we don't support different italic angles, only a boolean value. we also don't differentiate between
		// oblique and italic since not all backends give us that info.

		// if the style is italic:
		if(props.style == FontStyle::ITALIC)
		{
			auto tmp = std::find_if(matching_set.begin(), matching_set.end(), [](auto& font) {
				return font.properties.style == FontStyle::ITALIC;
			});

			if(tmp != matching_set.end())
			{
				// we have at least one italic font, so yeet the non-talic ones
				erase_if(matching_set, [](auto& font) {
					return font.properties.style != FontStyle::ITALIC;
				});
			}

			// otherwise, it doesn't matter.
		}
		else
		{
			// the same as above but in reverse
			auto tmp = std::find_if(matching_set.begin(), matching_set.end(), [](auto& font) {
				return font.properties.style == FontStyle::NORMAL;
			});

			if(tmp != matching_set.end())
			{
				// we have at least one normal font, so yeet the non-normal ones
				erase_if(matching_set, [](auto& font) {
					return font.properties.style != FontStyle::NORMAL;
				});
			}
		}

		if(matching_set.empty())
			return std::nullopt;

		// finally, match font weight. sort by it to make our lives easier
		bool have_exact_weight = false;
		matching_set = sorted_by(matching_set, &FontProperties::weight, props.weight, &have_exact_weight);

		FontWeight best_weight = props.weight;
		if(not have_exact_weight)
		{
			// if it's between 400 and 500 (normal and medium):
			if(FontWeight::NORMAL <= props.weight && props.weight <= FontWeight::MEDIUM)
			{
				// check in ascending order until we hit 500, then those less than desired, then those > 500.
				auto upper = search_ascending(matching_set, &FontProperties::weight, props.weight);
				if(upper.has_value())
				{
					if(*upper <= FontWeight::MEDIUM)
					{
						best_weight = *upper;
					}
					else
					{
						// since there's nothing between 400 and 500, we search < first, then >.
						// but we already found >, so just search <.
						auto lower = search_descending(matching_set, &FontProperties::weight, props.weight);
						if(lower.has_value())
							best_weight = *lower;
						else
							best_weight = *upper;
					}
				}
				else
				{
					// we must search below
					upper = search_descending(matching_set, &FontProperties::weight, props.weight);
					assert(upper.has_value());

					best_weight = *upper;
				}
			}
			else if(props.weight < FontWeight::NORMAL)
			{
				if(auto tmp = search_descending(matching_set, &FontProperties::weight, props.weight); tmp.has_value())
					best_weight = *tmp;
				else
					best_weight = *search_ascending(matching_set, &FontProperties::weight, props.weight);
			}
			else
			{
				if(auto tmp = search_ascending(matching_set, &FontProperties::weight, props.weight); tmp.has_value())
					best_weight = *tmp;
				else
					best_weight = *search_descending(matching_set, &FontProperties::weight, props.weight);
			}
		}

		erase_if(matching_set, [best_weight](const auto& font) {
			return font.properties.weight != best_weight;
		});

		// just always choose the first one.
		if(matching_set.empty())
			return std::nullopt;

		return matching_set[0];
	}


	static bool typeface_is_generic_name(zst::str_view typeface)
	{
		return typeface == GENERIC_SERIF || typeface == GENERIC_SANS_SERIF || typeface == GENERIC_MONOSPACE
		    || typeface == GENERIC_EMOJI;
	}

	static std::optional<FontHandle> get_font_for_generic_name(zst::str_view typeface, FontProperties properties)
	{
#if defined(USE_CORETEXT) && USE_CORETEXT != 0
		return coretext::get_font_for_generic_name(typeface, properties);
#elif defined(USE_FONTCONFIG) && USE_FONTCONFIG != 0
		return fontconfig::get_font_for_generic_name(typeface, properties);
#endif
		return {};
	}

	std::optional<FontHandle> findFont(const std::vector<std::string>& typefaces, FontProperties properties)
	{
		for(auto& typeface : typefaces)
		{
			std::optional<FontHandle> result {};
			if(typeface_is_generic_name(typeface))
				result = get_font_for_generic_name(typeface, properties);
			else
				result = searchForFont(typeface, properties);

			if(result.has_value())
				return *result;
		}

		return std::nullopt;
	}



	std::vector<FontHandle> getAllFonts()
	{
#if defined(USE_CORETEXT) && USE_CORETEXT != 0
		return coretext::get_all_fonts();
#elif defined(USE_FONTCONFIG) && USE_FONTCONFIG != 0
		return fontconfig::get_all_fonts();
#endif

		return {};
	}

	std::vector<std::string> getAllTypefaces()
	{
#if defined(USE_CORETEXT) && USE_CORETEXT != 0
		return coretext::get_all_typefaces();
#elif defined(USE_FONTCONFIG) && USE_FONTCONFIG != 0
		return fontconfig::get_all_typefaces();
#endif

		return {};
	}

	std::optional<FontHandle> searchForFont(zst::str_view typeface_name, FontProperties properties)
	{
#if defined(USE_CORETEXT) && USE_CORETEXT != 0
		if(auto x = coretext::search_for_font(typeface_name, properties); x.has_value())
			return x;

#elif defined(USE_FONTCONFIG) && USE_FONTCONFIG != 0
		if(auto x = fontconfig::search_for_font(typeface_name, properties); x.has_value())
			return x;
#endif

		return {};
	}

	std::optional<FontHandle> searchForFontWithPostscriptName(zst::str_view postscript_name)
	{
#if defined(USE_CORETEXT) && USE_CORETEXT != 0
		if(auto x = coretext::search_for_font_with_postscript_name(postscript_name); x.has_value())
			return x;

#elif defined(USE_FONTCONFIG) && USE_FONTCONFIG != 0
		if(auto x = fontconfig::search_for_font_with_postscript_name(postscript_name); x.has_value())
			return x;
#endif

		return {};
	}
}
