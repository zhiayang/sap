// units.h
// Copyright (c) 2022, zhiayang
// SPDX-License-Identifier: Apache-2.0

#pragma once

#include "util.h"
#include "../units.h"


/*
    72 units is 1 inch, 1 inch is 25.4mm. we don't want to assume
    the base unit (which we can change to be eg. µm or something).

    We define two separate units/coordinate systems -- the typographic_unit is the normal PDF unit and
    coordinate system, running at 72dpi with Y+ going UPWARDS. We also define typographic_unit_y_down,
    which is also 72dpi, but Y+ goes DOWNWARDS.

    Later on (below), we delete the conversion functions between these systems, because we need the page
    size to convert between them, so it's not as simple as just a `into()`. At the PDF interface,
    all systems require the normal `typographic_unit`.

    However, we can't delete every combination (due to C++ limitations), so care must still be taken to
    convert between the y-up and y-down coordinate systems at the sap-pdf interface.
*/

struct PDF_COORD_Y_UP;
struct PDF_COORD_Y_DOWN;
struct PDF_COORD_ANY;
struct PDF_UNIT_TYPOGRAPHIC;
struct PDF_UNIT_GLYPH_SPACE;
struct PDF_UNIT_TEXT_SPACE;

DEFINE_UNIT_IN_NAMESPACE(                                 //
    pdf_typographic_unit,                                 //
    (25.4 / 72.0) * dim::units::millimetre::scale_factor, //
    PDF_UNIT_TYPOGRAPHIC,                                 //
    pdf,                                                  //
    typographic_unit);

// PDF 1.7: 9.2.4 Glyph Positioning and Metrics
// ... the units of glyph space are one-thousandth of a unit of text space ...
static constexpr double PDF_GLYPH_SPACE_UNITS = 1000.0;

DEFINE_UNIT_IN_NAMESPACE(pdf_glyph_space, PDF_GLYPH_SPACE_UNITS, PDF_UNIT_GLYPH_SPACE, pdf, glyph_space);
DEFINE_UNIT_IN_NAMESPACE(pdf_text_space, 1, PDF_UNIT_TEXT_SPACE, pdf, text_space);


MAKE_UNITS_COMPATIBLE(dim::units::base_unit::Tag, PDF_UNIT_TYPOGRAPHIC);
MAKE_UNITS_COMPATIBLE(PDF_UNIT_TYPOGRAPHIC, dim::units::base_unit::Tag);

MAKE_UNITS_COMPATIBLE(PDF_UNIT_GLYPH_SPACE, PDF_UNIT_TEXT_SPACE);
MAKE_UNITS_COMPATIBLE(PDF_UNIT_TEXT_SPACE, PDF_UNIT_GLYPH_SPACE);

// pdf down is convertible to base down
MAKE_COORD_SYSTEMS_COMPATIBLE(PDF_COORD_Y_DOWN, BASE_COORD_SYSTEM);
MAKE_COORD_SYSTEMS_COMPATIBLE(BASE_COORD_SYSTEM, PDF_COORD_Y_DOWN);

// base is convertible to any
MAKE_COORD_SYSTEMS_COMPATIBLE(PDF_COORD_ANY, BASE_COORD_SYSTEM);
MAKE_COORD_SYSTEMS_COMPATIBLE(BASE_COORD_SYSTEM, PDF_COORD_ANY);

namespace pdf
{
	using PdfScalar = dim::Scalar<dim::units::pdf_typographic_unit>;

	using Vector2_YUp = dim::Vector2<dim::units::pdf_typographic_unit, PDF_COORD_Y_UP>;
	using Vector2_YDown = dim::Vector2<dim::units::pdf_typographic_unit, PDF_COORD_Y_DOWN>;

	using Size2d = dim::Vector2<dim::units::pdf_typographic_unit, PDF_COORD_ANY>;

	using Offset2d = Vector2_YUp;
	using Position2d = Vector2_YUp;

	using Size2d_YDown = Vector2_YDown;
	using Offset2d_YDown = Vector2_YDown;
	using Position2d_YDown = Vector2_YDown;

	using GlyphSpace1d = dim::Scalar<dim::units::pdf_glyph_space>;
	using GlyphSpace2d = dim::Vector2<dim::units::pdf_glyph_space, PDF_COORD_Y_UP>;

	using TextSpace1d = dim::Scalar<dim::units::pdf_text_space>;
	using TextSpace2d = dim::Vector2<dim::units::pdf_text_space, PDF_COORD_Y_UP>;
}
