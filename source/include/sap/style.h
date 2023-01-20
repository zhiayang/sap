// style.h
// Copyright (c) 2022, zhiayang
// SPDX-License-Identifier: Apache-2.0

#pragma once

#include "util.h"
#include "units.h" // for Scalar

#include "sap/units.h"       // for Length
#include "sap/font_family.h" // for FontSet, FontStyle

namespace sap
{
	enum class Alignment
	{
		Left,
		Centre,
		Right,
		Justified,
	};



	struct Style
	{
		inline Style() { }

#define DEFINE_ACCESSOR(field_type, field_name, method_name)                   \
	inline field_type method_name(const Style* default_parent = nullptr) const \
	{                                                                          \
		if(default_parent == this)                                             \
			default_parent = nullptr;                                          \
		if(field_name.has_value())                                             \
			return *field_name;                                                \
		else if(default_parent)                                                \
			return default_parent->method_name();                              \
		sap::internal_error("Accessed unset style field");                     \
	}

		DEFINE_ACCESSOR(FontFamily, m_font_family, font_family);
		DEFINE_ACCESSOR(FontStyle, m_font_style, font_style);
		DEFINE_ACCESSOR(Length, m_font_size, font_size);
		DEFINE_ACCESSOR(Length, m_root_font_size, root_font_size);
		DEFINE_ACCESSOR(double, m_line_spacing, line_spacing);
		DEFINE_ACCESSOR(Length, m_paragraph_spacing, paragraph_spacing);
		DEFINE_ACCESSOR(Alignment, m_alignment, alignment);
#undef DEFINE_ACCESSOR

#define DEFINE_SETTER(field_type, field_name, method_name)         \
	inline Style& method_name(std::optional<field_type> new_value) \
	{                                                              \
		if(new_value.has_value())                                  \
			field_name = std::move(new_value);                     \
		return *this;                                              \
	}

		DEFINE_SETTER(FontFamily, m_font_family, set_font_family);
		DEFINE_SETTER(FontStyle, m_font_style, set_font_style);
		DEFINE_SETTER(Length, m_font_size, set_font_size);
		DEFINE_SETTER(Length, m_root_font_size, set_root_font_size);
		DEFINE_SETTER(double, m_line_spacing, set_line_spacing);
		DEFINE_SETTER(Length, m_paragraph_spacing, set_paragraph_spacing);
		DEFINE_SETTER(Alignment, m_alignment, set_alignment);

#undef DEFINE_SETTER

		/*
		    basically, make a style that uses the fields from "main" if it exists (or any of its parents)
		    or from the backup style (or any of its parents). if both input styles are null, then the
		    default style is used.
		*/
		const Style* extendWith(const Style* main) const;

		constexpr bool operator==(const Style& other) const = default;

		const pdf::PdfFont* font() const { return this->font_family().getFontForStyle(this->font_style()); }

		static const Style* empty() { return &s_empty_style; }

	private:
		std::optional<FontFamily> m_font_family;
		std::optional<FontStyle> m_font_style;
		std::optional<Length> m_font_size;
		std::optional<Length> m_root_font_size;
		std::optional<double> m_line_spacing;
		std::optional<Length> m_paragraph_spacing;
		std::optional<Alignment> m_alignment;

		static Style s_empty_style;
	};


	struct Stylable
	{
		explicit inline Stylable(const Style* style = Style::empty()) : m_style(style) { }

		const Style* style() const { return m_style; }
		void setStyle(const Style* s) { m_style = s; }

	protected:
		const Style* m_style;
	};


}
