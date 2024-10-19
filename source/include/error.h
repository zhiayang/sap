// error.h
// Copyright (c) 2021, yuki / zhiayang
// SPDX-License-Identifier: Apache-2.0

#pragma once

#include <cstdio>
#include <cstdlib>

#include <string>
#include <vector>
#include <utility>

#include <zpr.h>
#include <zst/zst.h>

#include "location.h"

namespace util::impl
{
	void log_impl(const char* prefix, int level, const std::string& msg, const char* who = nullptr);
}

namespace sap
{
	namespace interp
	{
		struct Typechecker;
		struct Evaluator;
	}

	struct ErrorMessage
	{
		ErrorMessage() = default;
		explicit ErrorMessage(Location loc, const std::string& msg);
		explicit ErrorMessage(const interp::Typechecker* ts, const std::string& msg);
		explicit ErrorMessage(const interp::Evaluator* ev, const std::string& msg);

		ErrorMessage& addInfo(Location loc, const std::string& msg);
		ErrorMessage& addInfo(const interp::Typechecker* ts, const std::string& msg);
		ErrorMessage& addInfo(const interp::Evaluator* ev, const std::string& msg);

		void display() const;
		const std::string& string() const;
		[[noreturn]] void showAndExit() const;

	private:
		Location m_location;
		std::string m_message;

		std::vector<std::pair<Location, std::string>> m_infos;
	};

	template <typename T>
	using ErrorOr = zst::Result<T, ErrorMessage>;

	template <typename... Args>
	[[nodiscard]] zst::Err<ErrorMessage> ErrMsg(const Location& location, const char* fmt, Args&&... args)
	{
		return zst::Err<ErrorMessage>(location, zpr::sprint(fmt, static_cast<Args&&>(args)...));
	}

	template <typename... Args>
	[[nodiscard]] zst::Err<ErrorMessage> ErrMsg(const interp::Typechecker* ts, const char* fmt, Args&&... args)
	{
		return zst::Err<ErrorMessage>(ts, zpr::sprint(fmt, static_cast<Args&&>(args)...));
	}

	template <typename... Args>
	[[nodiscard]] zst::Err<ErrorMessage> ErrMsg(const interp::Evaluator* ev, const char* fmt, Args&&... args)
	{
		return zst::Err<ErrorMessage>(ev, zpr::sprint(fmt, static_cast<Args&&>(args)...));
	}





	template <typename... Args>
	[[noreturn]] inline void internal_error(const char* fmt, Args&&... args)
	{
		zpr::fprintln(stderr, "error: {}", zpr::fwd(fmt, static_cast<Args&&>(args)...));
		abort();
	}

	template <typename... Args>
	inline void log(const char* who, const char* fmt, Args&&... args)
	{
		util::impl::log_impl("log", 1, zpr::sprint(fmt, static_cast<Args&&>(args)...), who);
	}

	template <typename... Args>
	inline void warn(const char* who, const char* fmt, Args&&... args)
	{
		util::impl::log_impl("wrn", 2, zpr::sprint(fmt, static_cast<Args&&>(args)...), who);
	}

	template <typename... Args>
	[[noreturn]] inline void error(const char* who, const char* fmt, Args&&... args)
	{
		util::impl::log_impl("err", 3, zpr::sprint(fmt, static_cast<Args&&>(args)...), who);
		abort();
	}
}

template <>
struct zpr::print_formatter<sap::ErrorMessage>
{
	template <typename Cb>
	ZPR_ALWAYS_INLINE void print(const sap::ErrorMessage& err, Cb&& cb, format_args args)
	{
		print_one(cb, static_cast<format_args&&>(args), err.string());
	}
};
