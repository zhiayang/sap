// style.cpp
// Copyright (c) 2022, zhiayang
// SPDX-License-Identifier: Apache-2.0

#include "sap/style.h"

namespace sap
{
	static Style g_defaultStyle {};

	const Style& defaultStyle()
	{
		return g_defaultStyle;
	}

	void setDefaultStyle(Style s)
	{
		g_defaultStyle = s;
	}
}
