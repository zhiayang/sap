// pageobject.h
// Copyright (c) 2021, zhiayang
// Licensed under the Apache License Version 2.0.

#pragma once

#include <string>

namespace pdf
{
	struct Page;

	struct PageObject
	{
		virtual ~PageObject();
		virtual std::string serialise(const Page* page) const = 0;
	};
}
