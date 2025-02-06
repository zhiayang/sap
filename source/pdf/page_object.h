// page_object.h
// Copyright (c) 2021, yuki / zhiayang
// SPDX-License-Identifier: Apache-2.0

#pragma once

namespace pdf
{
	struct Page;
	struct Stream;

	struct PageObject
	{
		virtual ~PageObject();

		virtual void writePdfCommands(Stream* stream) const = 0;
		virtual void addResources(const Page* page) const = 0;
	};
}
