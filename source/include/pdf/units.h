// units.h
// Copyright (c) 2022, zhiayang
// SPDX-License-Identifier: Apache-2.0

#pragma once

#include "defs.h"

/*
	72 units is 1 inch, 1 inch is 25.4mm. we don't want to assume
	the base unit (which we can change to be eg. µm or something)
*/
DEFINE_UNIT_IN_NAMESPACE(
	pdf_typographic_unit,
	(25.4 / 72.0) * dim::units::millimetre::scale_factor,
	pdf,
	typographic_unit
)

namespace pdf
{
	using Scalar = dim::Scalar<dim::units::pdf_typographic_unit>;
	using Vector2 = dim::Vector2<dim::units::pdf_typographic_unit>;

	using Offset2d = Vector2;
	using Position2d = Vector2;
}
