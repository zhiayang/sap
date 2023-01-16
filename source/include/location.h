// location.h
// Copyright (c) 2022, zhiayang
// SPDX-License-Identifier: Apache-2.0

#pragma once

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

		static inline Location builtin()
		{
			return Location {
				.line = 0,
				.column = 0,
				.length = 1,
				.file = "builtin",
			};
		}
	};

	template <typename... Args>
	[[noreturn]] void error(const Location& loc, const char* fmt, Args&&... args)
	{
		zpr::fprintln(stderr, "{}:{}:{}: error: {}", loc.file, loc.line + 1, loc.column + 1,
		    zpr::fwd(fmt, static_cast<Args&&>(args)...));
		abort();
		// exit(1);
	}

	template <typename... Args>
	[[noreturn]] void error(const std::optional<Location>& loc, const char* fmt, Args&&... args)
	{
		if(loc.has_value())
		{
			error(*loc, fmt, static_cast<Args&&>(args)...);
		}
		else
		{
			zpr::fprintln(stderr, "<no location>: error: {}", zpr::fwd(fmt, static_cast<Args&&>(args)...));
			exit(1);
		}
	}
}
