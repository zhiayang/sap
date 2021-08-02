// misc.h
// Copyright (c) 2021, zhiayang
// Licensed under the Apache License Version 2.0.

#pragma once

#include <cstdio>
#include <cstdlib>

#include <zst.h>
#include <zpr.h>

namespace pdf
{
	template <typename... Args>
	[[noreturn]] inline void error(zst::str_view fmt, Args&&... args)
	{
		zpr::fprintln(stderr, "internal error (pdf): {}", zpr::fwd(fmt, static_cast<Args&&>(args)...));
		exit(1);
	}
}
