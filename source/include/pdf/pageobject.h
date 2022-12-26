// pageobject.h
// Copyright (c) 2021, zhiayang
// SPDX-License-Identifier: Apache-2.0

#pragma once

namespace pdf
{
	struct Page;

	struct PageObject
	{
		virtual ~PageObject();
		virtual std::string serialise(const Page* page) const = 0;
	};
}
