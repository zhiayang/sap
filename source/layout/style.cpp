// style.cpp
// Copyright (c) 2022, zhiayang
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
