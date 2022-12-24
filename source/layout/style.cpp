// style.cpp
// Copyright (c) 2022, zhiayang
// SPDX-License-Identifier: Apache-2.0

#include "sap/style.h"

namespace sap
{
	static Style g_defaultStyle {};

	const Style& defaultStyle()
	{
		return g_defaultStyle;
	}

	void setDefaultStyle(Style s)
	{
		g_defaultStyle = s;
	}

	using CombinedStylesHasher = decltype([](auto p) {
		return (std::uintptr_t) p.first ^ ((std::uintptr_t) p.second << 32) ^ ((std::uintptr_t) p.second >> 32);
	});
	static std::unordered_map<std::pair<const Style*, const Style*>, const Style*, CombinedStylesHasher> g_combined_styles;

	const Style* Style::combine(const Style* main, const Style* backup)
	{
		if(main == nullptr)
			return backup;
		if(backup == nullptr)
			return main;

		if(auto it = g_combined_styles.find({ main, backup }); it != nullptr)
			return it->second;

		auto style = util::make<Style>();
		style->set_font_set(main->font_set(backup))
		    .set_font_style(main->font_style(backup))
		    .set_font_size(main->font_size(backup))
		    .set_line_spacing(main->line_spacing(backup))
		    .set_pre_paragraph_spacing(main->pre_paragraph_spacing(backup))
		    .set_post_paragraph_spacing(main->post_paragraph_spacing(backup));

		g_combined_styles.insert({ { main, backup }, style });

		return style;
	}
}
