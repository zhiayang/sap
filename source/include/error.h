// error.h
// Copyright (c) 2021, zhiayang
// SPDX-License-Identifier: Apache-2.0

#pragma once

#include <cstdio>
#include <cstdlib>

#include <zpr.h>
#include <zst.h>

#include "location.h"

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

		void display() const;
		const std::string& string() const;
		[[noreturn]] void showAndExit() const;

	private:
		Location m_location;
		std::string m_message;
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
		// exit(1);
		abort();
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
		// exit(1);
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
