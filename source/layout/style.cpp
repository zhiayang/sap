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

	const Style* Style::extendWith(const Style* main) const
	{
		if(main == nullptr)
		{
			sap::internal_error("null style pointer bad");
		}

		if(auto it = g_combined_styles.find({ main, this }); it != g_combined_styles.end())
			return it->second;

		auto flat_value_or = [](const auto& a, const auto& b) -> auto
		{
			if(a.has_value())
				return a;
			return b;
		};

		auto style = Style();
		style.set_font_family(flat_value_or(main->m_font_family, m_font_family))
		    .set_font_style(flat_value_or(main->m_font_style, m_font_style))
		    .set_font_size(flat_value_or(main->m_font_size, m_font_size))
		    .set_line_spacing(flat_value_or(main->m_line_spacing, m_line_spacing))
		    .set_paragraph_spacing(flat_value_or(main->m_paragraph_spacing, m_paragraph_spacing))
		    .set_alignment(flat_value_or(main->m_alignment, m_alignment))
		    .set_root_font_size(flat_value_or(main->m_root_font_size, m_root_font_size)) //
		    ;

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
