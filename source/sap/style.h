// style.h
// Copyright (c) 2022, yuki / zhiayang
// SPDX-License-Identifier: Apache-2.0

#pragma once

#include "util.h"
#include "units.h"

#include "sap/units.h"
#include "sap/colour.h"
#include "sap/font_family.h"

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
#define DEFINE_ACCESSOR(field_type, field_name, method_name, name2)     \
	inline field_type method_name() const                               \
	{                                                                   \
		if(m_present_styles & (STY_##field_name))                       \
			return *field_name;                                         \
		sap::internal_error("accessed unset style field " #field_name); \
	}                                                                   \
	inline bool name2() const                                           \
	{                                                                   \
		return m_present_styles & STY_##field_name;                     \
	}

		DEFINE_ACCESSOR(FontFamily, m_font_family, font_family, have_font_family);
		DEFINE_ACCESSOR(FontStyle, m_font_style, font_style, have_font_style);
		DEFINE_ACCESSOR(Length, m_font_size, font_size, have_font_size);
		DEFINE_ACCESSOR(Length, m_root_font_size, root_font_size, have_root_font_size);
		DEFINE_ACCESSOR(double, m_line_spacing, line_spacing, have_line_spacing);
		DEFINE_ACCESSOR(double, m_sentence_space_stretch, sentence_space_stretch, have_sentence_space_stretch);
		DEFINE_ACCESSOR(Length, m_paragraph_spacing, paragraph_spacing, have_paragraph_spacing);
		DEFINE_ACCESSOR(Alignment, m_horz_alignment, horz_alignment, have_horz_alignment);
		DEFINE_ACCESSOR(Colour, m_colour, colour, have_colour);
		DEFINE_ACCESSOR(bool, m_enable_smart_quotes, smart_quotes_enabled, have_smart_quotes_enablement);
#undef DEFINE_ACCESSOR



#define DEFINE_SETTER(field_type, field_name, method_name, method_name2) \
	inline Style& method_name(std::optional<field_type> new_value)       \
	{                                                                    \
		if(new_value.has_value())                                        \
		{                                                                \
			field_name.set(std::move(*new_value));                       \
			m_present_styles |= STY_##field_name;                        \
		}                                                                \
		return *this;                                                    \
	}                                                                    \
                                                                         \
	inline Style method_name2(field_type new_value) const                \
	{                                                                    \
		auto copy = *this;                                               \
		copy.method_name(std::move(new_value));                          \
		return copy;                                                     \
	}

		DEFINE_SETTER(FontFamily, m_font_family, set_font_family, with_font_family);
		DEFINE_SETTER(FontStyle, m_font_style, set_font_style, with_font_style);
		DEFINE_SETTER(Length, m_font_size, set_font_size, with_font_size);
		DEFINE_SETTER(Length, m_root_font_size, set_root_font_size, with_root_font_size);
		DEFINE_SETTER(double, m_line_spacing, set_line_spacing, with_line_spacing);
		DEFINE_SETTER(double, m_sentence_space_stretch, set_sentence_space_stretch, with_sentence_space_stretch);
		DEFINE_SETTER(Length, m_paragraph_spacing, set_paragraph_spacing, with_paragraph_spacing);
		DEFINE_SETTER(Alignment, m_horz_alignment, set_horz_alignment, with_horz_alignment);
		DEFINE_SETTER(Colour, m_colour, set_colour, with_colour);
		DEFINE_SETTER(bool, m_enable_smart_quotes, enable_smart_quotes, with_smart_quotes_enabled);

#undef DEFINE_SETTER

#define NONE

#define VALUE_OR_ELSE(field_name, left, right)                             \
	[&]() {                                                                \
		std::optional<typename decltype(field_name)::value_type> __ret {}; \
		if((left).m_present_styles & (STY_##field_name))                   \
			__ret = *(left).field_name;                                    \
		else if((right).m_present_styles & (STY_##field_name))             \
			__ret = *(right).field_name;                                   \
		return __ret;                                                      \
	}()

		/*
		    with the current style as the reference, change all of our fields to those that `main` has.
		*/
		Style extendWith(const Style& main) const
		{
			auto style = Style();
			style.set_font_family(VALUE_OR_ELSE(m_font_family, main, *this))
			    .set_font_style(VALUE_OR_ELSE(m_font_style, main, *this))
			    .set_font_size(VALUE_OR_ELSE(m_font_size, main, *this))
			    .set_line_spacing(VALUE_OR_ELSE(m_line_spacing, main, *this))
			    .set_sentence_space_stretch(VALUE_OR_ELSE(m_sentence_space_stretch, main, *this))
			    .set_paragraph_spacing(VALUE_OR_ELSE(m_paragraph_spacing, main, *this))
			    .set_horz_alignment(VALUE_OR_ELSE(m_horz_alignment, main, *this))
			    .set_root_font_size(VALUE_OR_ELSE(m_root_font_size, main, *this))
			    .set_colour(VALUE_OR_ELSE(m_colour, main, *this))
			    .enable_smart_quotes(VALUE_OR_ELSE(m_enable_smart_quotes, main, *this)) //
			    ;

			return style;
		}

		/*
		    use `fallback` to fill in any fields that the current style does not have.
		*/
		Style useDefaultsFrom(const Style& fallback) const
		{
			auto style = Style();
			style.set_font_family(VALUE_OR_ELSE(m_font_family, *this, fallback))
			    .set_font_style(VALUE_OR_ELSE(m_font_style, *this, fallback))
			    .set_font_size(VALUE_OR_ELSE(m_font_size, *this, fallback))
			    .set_line_spacing(VALUE_OR_ELSE(m_line_spacing, *this, fallback))
			    .set_sentence_space_stretch(VALUE_OR_ELSE(m_sentence_space_stretch, *this, fallback))
			    .set_paragraph_spacing(VALUE_OR_ELSE(m_paragraph_spacing, *this, fallback))
			    .set_horz_alignment(VALUE_OR_ELSE(m_horz_alignment, *this, fallback))
			    .set_root_font_size(VALUE_OR_ELSE(m_root_font_size, *this, fallback))
			    .set_colour(VALUE_OR_ELSE(m_colour, *this, fallback))
			    .enable_smart_quotes(VALUE_OR_ELSE(m_enable_smart_quotes, *this, fallback)) //
			    ;

			return style;
		}
#undef NONE
#undef VALUE_OR_ELSE


		bool operator!=(const Style& other) const = default;
		bool operator==(const Style& other) const
		{
			if(m_present_styles != other.m_present_styles)
				return false;

			return (not(m_present_styles & STY_m_font_family) || *m_font_family == *other.m_font_family)
			    && (not(m_present_styles & STY_m_font_style) || *m_font_style == *other.m_font_style)
			    && (not(m_present_styles & STY_m_font_size) || *m_font_size == *other.m_font_size)
			    && (not(m_present_styles & STY_m_root_font_size) || *m_root_font_size == *other.m_root_font_size)
			    && (not(m_present_styles & STY_m_line_spacing) || *m_line_spacing == *other.m_line_spacing)
			    && (not(m_present_styles & STY_m_sentence_space_stretch)
			        || *m_sentence_space_stretch == *other.m_sentence_space_stretch)
			    && (not(m_present_styles & STY_m_paragraph_spacing)
			        || *m_paragraph_spacing == *other.m_paragraph_spacing)
			    && (not(m_present_styles & STY_m_horz_alignment) || *m_horz_alignment == *other.m_horz_alignment)
			    && (not(m_present_styles & STY_m_colour) || *m_colour == *other.m_colour)
			    && (not(m_present_styles & STY_m_enable_smart_quotes)
			        || *m_enable_smart_quotes == *other.m_enable_smart_quotes);
		}

		static const Style& empty();
		const pdf::PdfFont* font() const { return this->font_family().getFontForStyle(this->font_style()); }


		static constexpr uint32_t STY_m_font_family = (1u << 0);
		static constexpr uint32_t STY_m_font_style = (1u << 1);
		static constexpr uint32_t STY_m_font_size = (1u << 2);
		static constexpr uint32_t STY_m_root_font_size = (1u << 3);
		static constexpr uint32_t STY_m_line_spacing = (1u << 4);
		static constexpr uint32_t STY_m_sentence_space_stretch = (1u << 5);
		static constexpr uint32_t STY_m_paragraph_spacing = (1u << 6);
		static constexpr uint32_t STY_m_horz_alignment = (1u << 7);
		static constexpr uint32_t STY_m_colour = (1u << 8);
		static constexpr uint32_t STY_m_enable_smart_quotes = (1u << 9);

	private:
		uint32_t m_present_styles = 0;

		Uninitialised<FontFamily> m_font_family;
		Uninitialised<FontStyle> m_font_style;
		Uninitialised<Length> m_font_size;
		Uninitialised<Length> m_root_font_size;
		Uninitialised<double> m_line_spacing;
		Uninitialised<double> m_sentence_space_stretch;
		Uninitialised<Length> m_paragraph_spacing;
		Uninitialised<Alignment> m_horz_alignment;
		Uninitialised<Colour> m_colour;

		Uninitialised<bool> m_enable_smart_quotes;
	};



	struct Stylable
	{
		explicit inline Stylable(const Style& style = Style::empty()) : m_style(style) { }

		const Style& style() const { return m_style; }
		void setStyle(const Style& s) { m_style = s; }

	protected:
		Style m_style;
	};
}
