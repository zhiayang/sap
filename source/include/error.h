// error.h
// Copyright (c) 2021, zhiayang
// SPDX-License-Identifier: Apache-2.0

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

	template <typename... Args>
	inline void log(const char* who, const char* fmt, Args&&... args)
	{
		zpr::println("[log] {}: {}", who, zpr::fwd(fmt, static_cast<Args&&>(args)...));
	}

	template <typename... Args>
	inline void warn(const char* who, const char* fmt, Args&&... args)
	{
		zpr::println("[wrn] {}: {}", who, zpr::fwd(fmt, static_cast<Args&&>(args)...));
	}

	template <typename... Args>
	[[noreturn]] inline void error(const char* who, const char* fmt, Args&&... args)
	{
		zpr::println("[err] {}: {}", who, zpr::fwd(fmt, static_cast<Args&&>(args)...));
		exit(1);
	}
}
