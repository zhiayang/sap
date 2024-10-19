// units.h
// Copyright (c) 2022, yuki / zhiayang
// SPDX-License-Identifier: Apache-2.0

#pragma once

namespace pdf
{
	struct PdfFont;
}

namespace sap
{
	struct Style;

	using Length = dim::Scalar<dim::units::mm>;
	using Vector2 = dim::Vector2<dim::units::mm, BASE_COORD_SYSTEM>;

	using Size2d = Vector2;
	using Position = Vector2;
	using Offset2d = Vector2;

	struct DynLength
	{
		enum Unit
		{
			MM,  // millimetre
			CM,  // centimetre
			EM,  // em (m width [actually just the font size -- 1em = 10pt for 10pt fonts, etc.])
			EX,  // ex (x height [actually])
			IN,  // inch
			PT,  // point (1/72 of an inch)
			PC,  // pica (12 pts)
			REM, // root em - always the em-size of the document font size, regardless of nesting/containers/parents
		};

		DynLength() : m_value(0), m_unit(MM) { }
		explicit DynLength(sap::Length len);
		explicit DynLength(double value, Unit unit) : m_value(value), m_unit(unit) { }

		Unit unit() const { return m_unit; }
		double value() const { return m_value; }

		std::string str() const;

		DynLength negate() const { return DynLength(-m_value, m_unit); }

		sap::Length resolve(const Style& style) const;
		sap::Length resolve(const pdf::PdfFont* font, sap::Length font_size, sap::Length root_font_size) const;

		sap::Length resolveWithoutFont(sap::Length font_size, sap::Length root_font_size) const;

		static std::optional<Unit> stringToUnit(zst::str_view sv);
		static const char* unitToString(Unit unit);

	private:
		double m_value;
		Unit m_unit;
	};

	struct DynLength2d
	{
		DynLength x;
		DynLength y;

		sap::Size2d resolve(const Style& style) const;
		sap::Size2d resolve(const pdf::PdfFont* font, sap::Length font_size, sap::Length root_font_size) const;

		sap::Size2d resolveWithoutFont(sap::Length font_size, sap::Length root_font_size) const;
	};

	struct LayoutSize
	{
		Length width;
		Length ascent;
		Length descent;

		Length total_height() const { return this->ascent + this->descent; }
	};



	constexpr sap::Length operator""_mm(long double x)
	{
		return Length(static_cast<double>(x));
	}

	constexpr sap::Length operator""_mm(unsigned long long x)
	{
		return Length(static_cast<double>(x));
	}



	constexpr sap::Length operator""_pt(unsigned long long x)
	{
		return Length(static_cast<double>(x) * 25.4 / 72);
	}

	constexpr sap::Length operator""_pt(long double x)
	{
		return Length(static_cast<double>(x) * 25.4 / 72);
	}
}
