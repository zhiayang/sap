// style.h
// Copyright (c) 2022, zhiayang
// SPDX-License-Identifier: Apache-2.0

#pragma once

#include <cassert>

#include "defs.h"
#include "pool.h"
#include "sap/units.h"

namespace pdf
{
	struct Font;
}

namespace sap
{
	struct Style;
	const Style& defaultStyle();
	void setDefaultStyle(Style s);

	struct Style
	{
		inline Style() : m_parent(nullptr) { }
		explicit inline Style(const Style* parent) : m_parent(parent) { }

#define DEFINE_ACCESSOR(field_type, field_name, method_name)                   \
	inline field_type method_name(const Style* default_parent = nullptr) const \
	{                                                                          \
		if(default_parent == this)                                             \
			default_parent = nullptr;                                          \
		auto par = (m_parent == this) ? nullptr : m_parent;                    \
		if(field_name.has_value())                                             \
			return *field_name;                                                \
		else if(m_parent)                                                      \
			return par->method_name(default_parent);                           \
		else if(default_parent)                                                \
			return default_parent->method_name();                              \
		else                                                                   \
			return defaultStyle().method_name();                               \
	}

		// font is special because it is already a nullable pointer
		inline const pdf::Font* font(const Style* default_parent = nullptr) const
		{
			if(default_parent == this)
				default_parent = nullptr;
			auto par = (m_parent == this) ? nullptr : m_parent;

			if(m_font)
				return m_font;
			else if(m_parent)
				return par->font(default_parent);
			else if(default_parent)
				return default_parent->font();
			else
				return defaultStyle().font();
		}

		DEFINE_ACCESSOR(Scalar, m_font_size, font_size);
		DEFINE_ACCESSOR(Scalar, m_line_spacing, line_spacing);
		DEFINE_ACCESSOR(Scalar, m_pre_para_spacing, pre_paragraph_spacing);
		DEFINE_ACCESSOR(Scalar, m_post_para_spacing, post_paragraph_spacing);
#undef DEFINE_ACCESSOR

#define DEFINE_SETTER(field_type, field_name, method_name) \
	inline Style& method_name(field_type new_value)        \
	{                                                      \
		field_name = std::move(new_value);                 \
		return *this;                                      \
	}

		DEFINE_SETTER(const pdf::Font*, m_font, set_font);
		DEFINE_SETTER(Scalar, m_font_size, set_font_size);
		DEFINE_SETTER(Scalar, m_line_spacing, set_line_spacing);
		DEFINE_SETTER(Scalar, m_pre_para_spacing, set_pre_paragraph_spacing);
		DEFINE_SETTER(Scalar, m_post_para_spacing, set_post_paragraph_spacing);

#undef DEFINE_SETTER


		const Style* parent() const
		{
			return m_parent;
		}


		// static inline const Style* fallback(const Style* main, const Style* backup)
		// {
		// 	return main ? main : backup;
		// }

		/*
		    basically, make a style that uses the fields from "main" if it exists (or any of its parents)
		    or from the backup style (or any of its parents). if both input styles are null, then the
		    default style is used.
		*/
		static inline const Style* combine(const Style* main, const Style* backup)
		{
			if(main == nullptr && backup == nullptr)
				return &defaultStyle();

			else if(main == nullptr)
				main = backup, backup = nullptr;

			auto style = util::make<Style>();
			style->set_font(main->font(backup))
				.set_font_size(main->font_size(backup))
				.set_line_spacing(main->line_spacing(backup))
				.set_pre_paragraph_spacing(main->pre_paragraph_spacing(backup))
				.set_post_paragraph_spacing(main->post_paragraph_spacing(backup));

			return style;
		}


	private:
		const pdf::Font* m_font = nullptr;
		std::optional<Scalar> m_font_size;
		std::optional<Scalar> m_line_spacing;
		std::optional<Scalar> m_pre_para_spacing;
		std::optional<Scalar> m_post_para_spacing;

		const Style* m_parent = nullptr;
	};


	struct Stylable
	{
		explicit inline Stylable(const Style* style = nullptr) : m_style(style) { }

		const Style* style() const { return m_style; }
		void setStyle(const Style* s) { m_style = s; }

	protected:
		const Style* m_style;
	};
}
