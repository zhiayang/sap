// writer.h
// Copyright (c) 2021, zhiayang
// SPDX-License-Identifier: Apache-2.0

#pragma once

#include <zst.h>
#include <zpr.h>
#include <cstddef>

namespace pdf
{
	struct Object;

	struct Writer
	{
		Writer(zst::str_view path);
		~Writer();

		int fd;
		int nesting;
		zst::str_view path;
		size_t bytes_written;

		void close();

		size_t position() const;

		size_t write(zst::str_view sv);
		size_t writeln(zst::str_view sv);
		size_t writeln();

		void write(const Object* obj);
		void writeFull(const Object* obj);

		size_t writeBytes(const uint8_t* bytes, size_t len);

		template <typename... Args>
		size_t write(zst::str_view fmt, Args&&... args)
		{
			return zpr::cprint(
				[this](const char* s, size_t l) {
					this->write(zst::str_view(s, l));
				},
				fmt, static_cast<Args&&>(args)...);
		}

		template <typename... Args>
		size_t writeln(zst::str_view fmt, Args&&... args)
		{
			return zpr::cprintln(
				[this](const char* s, size_t l) {
					this->write(zst::str_view(s, l));
				},
				fmt, static_cast<Args&&>(args)...);
		}
	};
}
