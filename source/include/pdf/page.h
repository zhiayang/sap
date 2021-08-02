// page.h
// Copyright (c) 2021, zhiayang
// Licensed under the Apache License Version 2.0.

#pragma once

#include "pdf/object.h"

namespace pdf
{
	struct Document;

	struct Page
	{
		Dictionary* serialise(Document* doc) const;
	};
}
