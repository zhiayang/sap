// path.h
// Copyright (c) 2023, zhiayang
// SPDX-License-Identifier: Apache-2.0

#pragma once

#include "sap/colour.h"

#include "pdf/units.h"
#include "pdf/page_object.h"

namespace pdf
{
	struct Path : PageObject
	{
		virtual void addResources(const Page* page) const override;
		virtual void writePdfCommands(Stream* stream) const override;
	};
}
