// eval_result.h
// Copyright (c) 2022, yuki / zhiayang
// SPDX-License-Identifier: Apache-2.0

#pragma once

#include <optional> // for optional, nullopt

#include "interp/value.h" // for Value

namespace sap::interp
{
	struct EvalResult
	{
		bool hasValue() const { return m_result.has_value() || m_lvalue != nullptr; }

		bool isLValue() const { return m_kind == Normal && m_lvalue != nullptr; }
		bool isNormal() const { return m_kind == Normal; }
		bool isReturn() const { return m_kind == Return; }
		bool isLoopBreak() const { return m_kind == LoopBreak; }
		bool isLoopContinue() const { return m_kind == LoopContinue; }

		enum ResultKind
		{
			Normal,
			Return,
			LoopBreak,
			LoopContinue,
		};

		const Value& get() const
		{
			assert(this->hasValue());
			if(m_lvalue != nullptr)
				return *m_lvalue;

			return *m_result;
		}

		Value& get()
		{
			assert(this->hasValue());
			if(m_lvalue != nullptr)
				return *m_lvalue;

			return *m_result;
		}

		Value take()
		{
			assert(this->hasValue());

			// if we are an lvalue, we can't let you "take" -- that doesn't make any sense.
			// instead we clone the lvalue and give it to you.
			if(m_lvalue != nullptr)
				return m_lvalue->clone();

			if(m_result->hasGenerator())
				return m_result->clone();

			return std::move(*m_result);
		}

		Value move()
		{
			assert(this->isLValue());
			assert(m_lvalue != nullptr);

			return std::move(*m_lvalue);
		}

		Value* lValuePointer() const
		{
			assert(this->isLValue());
			return m_lvalue;
		}

		static ErrorOr<EvalResult> ofVoid() { return Ok(EvalResult(Normal, {})); }
		static ErrorOr<EvalResult> ofValue(Value value) { return Ok(EvalResult(Normal, std::move(value))); }

		static ErrorOr<EvalResult> ofLoopBreak() { return Ok(EvalResult(LoopBreak, {})); }
		static ErrorOr<EvalResult> ofLoopContinue() { return Ok(EvalResult(LoopContinue, {})); }
		static ErrorOr<EvalResult> ofReturnVoid() { return Ok(EvalResult(Return, {})); }
		static ErrorOr<EvalResult> ofReturnValue(Value value) { return Ok(EvalResult(Return, std::move(value))); }

		static ErrorOr<EvalResult> ofLValue(Value& lvalue)
		{
			auto ret = EvalResult(Normal, {});
			ret.m_lvalue = &lvalue;

			return OkMove(ret);
		}

	private:
		EvalResult(ResultKind kind, std::optional<Value> value) : m_kind(kind), m_result(std::move(value)) { }

		ResultKind m_kind = Normal;
		std::optional<Value> m_result = std::nullopt;

		Value* m_lvalue = nullptr;
	};


#define __TRY_VALUE(x, L)                                                                   \
	__extension__({                                                                         \
		auto&& __r##L = x;                                                                  \
		using R = std::decay_t<decltype(__r##L)>;                                           \
		using V = typename R::value_type;                                                   \
		using E = typename R::error_type;                                                   \
		static_assert(not std::is_same_v<V, void>, "cannot use TRY_VALUE on Result<void>"); \
		if((__r##L).is_err())                                                               \
			return Err<E>((__r##L).take_error());                                           \
		if(not(__r##L)->hasValue())                                                         \
			sap::internal_error("unexpected void value");                                   \
		auto __ret = std::move(__r##L)->take();                                             \
		__ret.hasGenerator() ? __ret.clone() : std::move(__ret);                            \
	})
}

#define _TRY_VALUE(x, L) __TRY_VALUE(x, L)
#define TRY_VALUE(x) _TRY_VALUE(x, __COUNTER__)
