// eval_result.h
// Copyright (c) 2022, zhiayang
// SPDX-License-Identifier: Apache-2.0

#pragma once

#include <optional>

#include "interp/value.h"

namespace sap::interp
{
	// TODO: make this something that we don't have to keep throwing Ok() around
	struct EvalResult
	{
		bool has_value() const { return result.has_value(); }
		bool is_normal() const { return kind == Normal; }
		bool is_return() const { return kind == Return; }
		bool is_loop_break() const { return kind == LoopBreak; }
		bool is_loop_continue() const { return kind == LoopContinue; }

		const Value& get_value() const
		{
			assert(this->has_value());
			return *result;
		}

		Value& get_value()
		{
			assert(this->has_value());
			return *result;
		}

		Value take_value()
		{
			assert(this->has_value());
			return std::move(*result);
		}

		enum ResultKind
		{
			Normal,
			Return,
			LoopBreak,
			LoopContinue,
		};

		ResultKind kind = Normal;
		std::optional<Value> result = std::nullopt;

		static EvalResult of_void()
		{
			return EvalResult {
				.kind = Normal,
				.result = std::nullopt,
			};
		}

		static EvalResult of_value(Value value)
		{
			return EvalResult {
				.kind = Normal,
				.result = std::move(value),
			};
		}

		static EvalResult of_loop_break()
		{
			return EvalResult {
				.kind = LoopBreak,
				.result = std::nullopt,
			};
		}

		static EvalResult of_loop_continue()
		{
			return EvalResult {
				.kind = LoopContinue,
				.result = std::nullopt,
			};
		}

		static EvalResult of_return_void()
		{
			return EvalResult {
				.kind = Return,
				.result = std::nullopt,
			};
		}

		static EvalResult of_return_value(Value value)
		{
			return EvalResult {
				.kind = Return,
				.result = std::move(value),
			};
		}
	};

#define TRY_VALUE(x)                                                                        \
	__extension__({                                                                         \
		auto&& r = x;                                                                       \
		using R = std::decay_t<decltype(r)>;                                                \
		using V = typename R::value_type;                                                   \
		using E = typename R::error_type;                                                   \
		static_assert(not std::is_same_v<V, void>, "cannot use TRY_VALUE on Result<void>"); \
		if(r.is_err())                                                                      \
			return Err<E>(r.take_error());                                                  \
		if(not r->has_value())                                                              \
			return ErrFmt("unexpected void value");                                         \
		std::move(r)->take_value();                                                         \
	})
}
