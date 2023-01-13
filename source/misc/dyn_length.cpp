// dyn_length.cpp
// Copyright (c) 2022, zhiayang
// SPDX-License-Identifier: Apache-2.0

#include "pdf/font.h"
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

	sap::Length DynLength::resolve(const pdf::PdfFont* font, sap::Length font_size) const
	{
		switch(m_unit)
		{
			case EX: {
				auto x_height = font->getFontMetrics().x_height;
				return font->scaleMetricForFontSize(font::FontScalar(x_height == 0 ? 400 : x_height), font_size.into()).into();
			}

			case EM:
			case REM:
				// ez. note that for `rem` we just require the caller to sort out their shit
				// before calling us. I don't want to store the documetnnt font size here or anything.
				return font_size;

			case MM: return dim::mm(m_value);
			case CM: return dim::cm(m_value).into();
			case IN: return inches(m_value);
			case PT: return inches(m_value) / 72;
			case PC: return inches(m_value) / 6;
		}
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
	}
}
