// style.cpp
// Copyright (c) 2022, yuki
// SPDX-License-Identifier: Apache-2.0

#include "sap/style.h"

namespace sap
{
	const Style& Style::empty()
	{
		static Style ret {};
		return ret;
	}
}
