// dyn_length.cpp
// Copyright (c) 2022, yuki
// SPDX-License-Identifier: Apache-2.0

#include "pdf/font.h"
#include "sap/style.h"
#include "sap/units.h"

namespace sap
{
	static sap::Length inches(double value)
	{
		// 25.4mm per inch
		return dim::mm(value * 25.4);
	}

	DynLength::DynLength(sap::Length len) : m_value(len.value()), m_unit(MM)
	{
	}

	sap::Length DynLength::resolveWithoutFont(sap::Length font_size, sap::Length root_font_size) const
	{
		switch(m_unit)
		{
			case EX: return (0.4 * font_size).into();
			case REM: return m_value * root_font_size;
			case EM: return m_value * font_size;
			case MM: return dim::mm(m_value);
			case CM: return dim::cm(m_value).into();
			case IN: return inches(m_value);
			case PT: return inches(m_value) / 72;
			case PC: return inches(m_value) / 6;
		}
		util::unreachable();
	}

	sap::Length DynLength::resolve(const Style& style) const
	{
		return this->resolve(style.font(), style.font_size(), style.root_font_size());
	}

	sap::Length DynLength::resolve(const pdf::PdfFont* font, sap::Length font_size, sap::Length root_font_size) const
	{
		switch(m_unit)
		{
			case EX: {
				auto x_height = font->getFontMetrics().x_height;
				return font
				    ->scaleMetricForFontSize(x_height.iszero() ? font::FontScalar(400.0) : x_height, font_size.into())
				    .into();
			}
			default: return resolveWithoutFont(font_size, root_font_size);
		}
	}

	std::string DynLength::str() const
	{
		return zpr::sprint("{}{}", m_value, unitToString(m_unit));
	}

	auto DynLength::stringToUnit(zst::str_view sv) -> std::optional<Unit>
	{
		if(sv == "mm")
			return MM;
		else if(sv == "cm")
			return CM;
		else if(sv == "ex")
			return CM;
		else if(sv == "em")
			return EM;
		else if(sv == "rem")
			return REM;
		else if(sv == "in")
			return IN;
		else if(sv == "pt")
			return PT;
		else if(sv == "pc")
			return PC;
		else
			return std::nullopt;
	}

	const char* DynLength::unitToString(Unit unit)
	{
		switch(unit)
		{
			case MM: return "mm";
			case CM: return "cm";
			case EX: return "ex";
			case EM: return "em";
			case IN: return "in";
			case PT: return "pt";
			case PC: return "pc";
			case REM: return "rem";
		}
		util::unreachable();
	}


	sap::Size2d DynLength2d::resolve(const Style& style) const
	{
		return sap::Size2d(this->x.resolve(style), this->y.resolve(style));
	}

	sap::Size2d DynLength2d::resolve(const pdf::PdfFont* font, sap::Length font_size, sap::Length root_font_size) const
	{
		return sap::Size2d(this->x.resolve(font, font_size, root_font_size),
		    this->y.resolve(font, font_size, root_font_size));
	}

	sap::Size2d DynLength2d::resolveWithoutFont(sap::Length font_size, sap::Length root_font_size) const
	{
		return sap::Size2d(this->x.resolveWithoutFont(font_size, root_font_size),
		    this->y.resolveWithoutFont(font_size, root_font_size));
	}
}
