// style.cpp
// Copyright (c) 2022, zhiayang
// SPDX-License-Identifier: Apache-2.0

#include "sap/style.h"

namespace sap
{
	using CombinedStylesHasher = decltype([](auto p) {
		return (std::uintptr_t) p.first ^ ((std::uintptr_t) p.second << 32) ^ ((std::uintptr_t) p.second >> 32);
	});
	Style Style::s_empty_style;

	static std::unordered_map<std::pair<const Style*, const Style*>, const Style*, CombinedStylesHasher> g_combined_styles;

	const Style* Style::extend(const Style* main) const
	{
		if(main == nullptr)
		{
			sap::internal_error("null style pointer bad");
		}

		if(auto it = g_combined_styles.find({ main, this }); it != g_combined_styles.end())
			return it->second;

		auto style = Style();
		style.set_fontset(main->fontset(this))
		    .set_font_style(main->font_style(this))
		    .set_font_size(main->font_size(this))
		    .set_line_spacing(main->line_spacing(this))
		    .set_pre_paragraph_spacing(main->pre_paragraph_spacing(this))
		    .set_post_paragraph_spacing(main->post_paragraph_spacing(this));

		if(style == *main)
		{
			g_combined_styles.insert({ { main, this }, main });
			return main;
		}

		auto style_ptr = util::make<Style>();
		*style_ptr = std::move(style);

		g_combined_styles.insert({ { main, this }, style_ptr });
		return style_ptr;
	}
}
