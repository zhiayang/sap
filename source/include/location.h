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
		size_t byte_offset;

		zst::str_view filename;
		zst::str_view file_contents;

		bool is_builtin = false;

		static inline Location builtin()
		{
			return Location {
				.line = 0,
				.column = 0,
				.length = 1,
				.filename = "builtin",
				.is_builtin = true,
			};
		}
	};
}
