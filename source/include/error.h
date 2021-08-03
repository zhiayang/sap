// error.h
// Copyright (c) 2021, zhiayang
// Licensed under the Apache License Version 2.0.

#pragma once

#include <cstdio>
#include <cstdlib>

#include <zpr.h>

namespace sap
{
	template <typename... Args>
	[[noreturn]] inline void internal_error(const char* fmt, Args&&... args)
	{
		zpr::fprintln(stderr, "error: {}", zpr::fwd(fmt, static_cast<Args&&>(args)...));
		exit(1);
	}
}
