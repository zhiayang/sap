// font_scalar.h
// Copyright (c) 2022, yuki
// SPDX-License-Identifier: Apache-2.0

#pragma once

#include "units.h"


struct FONT_UNIT_DESIGN_SPACE;

// this is not compatible with anybody
DEFINE_UNIT_IN_NAMESPACE(font_design_space, 1, FONT_UNIT_DESIGN_SPACE, pdf, design_space);

namespace font
{
	struct FONT_COORD_SPACE;

	using FontScalar = dim::Scalar<dim::units::font_design_space>;
	using FontVector2d = dim::Vector2<dim::units::font_design_space, FONT_COORD_SPACE>;
}
