// defs.h
// Copyright (c) 2021, zhiayang
// SPDX-License-Identifier: Apache-2.0

#pragma once

#include <cmath>
#include <unordered_map>
#include <unordered_set>

#include <zpr.h>
#include <zst/zst.h>

#include "error.h"
#include "units.h"

namespace sap
{
	template <typename T>
	using StrErrorOr = zst::Result<T, std::string>;

	using zst::Ok;
	using zst::Err;
	using zst::ErrFmt;

	using zst::Left;
	using zst::Right;
	using zst::Either;

	using zst::Result;
	using zst::Failable;

	bool isDraftMode();
	bool compile(zst::str_view input_file, zst::str_view output_file);

	namespace watch
	{
		bool isSupportedPlatform();

		void start(zst::str_view main_file, zst::str_view output_file);
		StrErrorOr<void> addFileToWatchList(zst::str_view path);
	}
}

#define __TRY(x, L) __extension__({                                       \
		auto&& __r##L = x;                                                \
		using R = std::decay_t<decltype(__r##L)>;                         \
		using V = typename R::value_type;                                 \
		using E = typename R::error_type;                                 \
		if((__r##L).is_err())                                             \
			return Err(std::move((__r##L).error()));                      \
		util::impl::extract_value_or_return_void<V, E>().extract(__r##L); \
	})

#define _TRY(x, L) __TRY(x, L)
#define TRY(x) _TRY(x, __COUNTER__)

inline void zst::error_and_exit(const char* str, size_t len)
{
	sap::internal_error("{}", zst::str_view(str, len));
}
