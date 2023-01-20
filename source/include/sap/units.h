// units.h
// Copyright (c) 2022, zhiayang
// SPDX-License-Identifier: Apache-2.0

#pragma once

namespace pdf
{
	struct PdfFont;
}

namespace sap
{
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

		sap::Length resolve(const pdf::PdfFont* font, sap::Length font_size, sap::Length root_font_size) const;

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
	};
}
