// units.h
// Copyright (c) 2022, zhiayang
// SPDX-License-Identifier: Apache-2.0

#pragma once

#include "defs.h"

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
DEFINE_UNIT_IN_NAMESPACE(pdf_typographic_unit, (25.4 / 72.0) * dim::units::millimetre::scale_factor, pdf, typographic_unit)

DEFINE_UNIT_IN_NAMESPACE(pdf_typographic_unit_y_down, (25.4 / 72.0) * dim::units::millimetre::scale_factor, pdf,
	typographic_unit_y_down)

namespace pdf
{
	using Scalar = dim::Scalar<dim::units::pdf_typographic_unit>;

	using Vector2_YUp = dim::Vector2<dim::units::pdf_typographic_unit>;
	using Vector2_YDown = dim::Vector2<dim::units::pdf_typographic_unit_y_down>;

	using Size2d = Vector2_YUp;
	using Offset2d = Vector2_YUp;
	using Position2d = Vector2_YUp;

	using Size2d_YDown = Vector2_YDown;
	using Offset2d_YDown = Vector2_YDown;
	using Position2d_YDown = Vector2_YDown;
}

/*
    this mess simply prevents converting to/from the y-up and y-down coordinate systems,
    because we need the page size to correctly perform such conversions.
*/
DELETE_CONVERSION_VECTOR2(pdf_typographic_unit, pdf_typographic_unit_y_down)
DELETE_CONVERSION_VECTOR2(pdf_typographic_unit_y_down, pdf_typographic_unit)
