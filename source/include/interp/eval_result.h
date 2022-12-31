// eval_result.h
// Copyright (c) 2022, zhiayang
// SPDX-License-Identifier: Apache-2.0

#pragma once

#include <optional> // for optional, nullopt

#include "interp/value.h" // for Value

namespace sap::interp
{
	struct TCResult
	{
		const Type* type() const { return m_type; }
		bool isMutable() const { return m_is_mutable; }
		bool isLValue() const { return m_is_lvalue; }

		TCResult replacingType(const Type* type) const { return TCResult(type, m_is_mutable, m_is_lvalue); }

		static ErrorOr<TCResult> ofVoid() { return Ok(TCResult(Type::makeVoid(), false, false)); }
		static ErrorOr<TCResult> ofRValue(const Type* type) { return Ok(TCResult(type, false, false)); }
		static ErrorOr<TCResult> ofLValue(const Type* type, bool is_mutable) { return Ok(TCResult(type, is_mutable, true)); }

	private:
		explicit TCResult(const Type* type, bool is_mutable, bool is_lvalue)
		    : m_type(type)
		    , m_is_mutable(is_mutable)
		    , m_is_lvalue(is_lvalue)
		{
		}

		const Type* m_type;
		bool m_is_mutable;
		bool m_is_lvalue;
	};


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

			return std::move(*m_result);
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

			return Ok(std::move(ret));
		}

	private:
		EvalResult(ResultKind kind, std::optional<Value> value) : m_kind(kind), m_result(std::move(value)) { }

		ResultKind m_kind = Normal;
		std::optional<Value> m_result = std::nullopt;

		Value* m_lvalue = nullptr;
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
		if(not r->hasValue())                                                               \
			return ErrFmt("unexpected void value");                                         \
		std::move(r)->take();                                                               \
	})
}
