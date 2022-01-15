// defs.h
// Copyright (c) 2021, zhiayang
// SPDX-License-Identifier: Apache-2.0

#pragma once

#include <cmath>

#include "zst.h"
#include "zpr.h"
#include "error.h"

#include "units.h"

namespace sap
{
}


template <typename... Args>
void zst::error_and_exit(const char* fmt, Args&&... args)
{
	sap::internal_error(fmt, static_cast<Args&&>(args)...);
}
