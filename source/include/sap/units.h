// units.h
// Copyright (c) 2022, zhiayang
// SPDX-License-Identifier: Apache-2.0

#pragma once

#include "../units.h"
#include "pdf/units.h"

namespace sap
{
	using Scalar = dim::Scalar<dim::units::mm>;
	using Vector2 = dim::Vector2<dim::units::mm>;
}

// also, do our part to prevent converting to the pdf y-up coordinate system implicitly! we want
// to always convert to the y-down system, then use explicit conversions to get to the pdf y-up system.
DELETE_CONVERSION_VECTOR2(pdf_typographic_unit, mm)
DELETE_CONVERSION_VECTOR2(mm, pdf_typographic_unit)
