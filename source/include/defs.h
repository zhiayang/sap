// defs.h
// Copyright (c) 2021, zhiayang
// SPDX-License-Identifier: Apache-2.0

#pragma once

#include <cmath>

#include <zst.h>
#include <zpr.h>

#include "error.h"
#include "units.h"

namespace sap
{
}

inline void zst::error_and_exit(const char* str, size_t len)
{
	sap::internal_error("{}", zst::str_view(str, len));
}
