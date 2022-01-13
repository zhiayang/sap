// page.cpp
// Copyright (c) 2021, zhiayang
// SPDX-License-Identifier: Apache-2.0

#include "sap.h"
#include "pdf/page.h"

namespace sap
{
	pdf::Page* Page::render()
	{
		return util::make<pdf::Page>();
	}
}
