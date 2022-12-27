// scalar.h
// Copyright (c) 2022, zhiayang
// SPDX-License-Identifier: Apache-2.0

#pragma once

#include "units.h"


struct UNIT_TAG_FONT_DESIGN_SPACE;

// this is not compatible with anybody
DEFINE_UNIT_IN_NAMESPACE(font_design_space, 1, UNIT_TAG_FONT_DESIGN_SPACE, pdf, design_space);

namespace font
{
	using FontScalar = dim::Scalar<dim::units::font_design_space>;
	using FontVector2d = dim::Vector2<dim::units::font_design_space>;
}
