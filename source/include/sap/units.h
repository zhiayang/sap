// units.h
// Copyright (c) 2022, zhiayang
// SPDX-License-Identifier: Apache-2.0

#pragma once

#include "units.h" // for mm, BASE_COORD_SYSTEM, Scalar, Vector2

#include "sap/units.h"

namespace sap
{
	using Length = dim::Scalar<dim::units::mm>;
	using Vector2 = dim::Vector2<dim::units::mm, BASE_COORD_SYSTEM>;
}
