// location.h
// Copyright (c) 2022, zhiayang
// SPDX-License-Identifier: Apache-2.0

#pragma once

#include <zst.h>
#include <zpr.h>

#include <stdint.h>
#include <stddef.h>
#include <stdlib.h>

namespace sap
{
	/*
		Internal location members are 0-based; when printing, convert to 1-based yourself.
	*/
	struct Location
	{
		uint32_t line;
		uint32_t column;
		uint32_t length;
		zst::str_view file;
	};

	template <typename... Args>
	[[noreturn]] void error(const Location& loc, const char* fmt, Args&&... args)
	{
		zpr::fprintln(stderr, "{}:{}:{}: error: {}", loc.file, loc.line + 1, loc.column + 1,
			zpr::fwd(fmt, static_cast<Args&&>(args)...));
		exit(1);
	}
}
