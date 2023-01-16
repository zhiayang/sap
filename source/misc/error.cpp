// error.cpp
// Copyright (c) 2022, zhiayang
// SPDX-License-Identifier: Apache-2.0

#include "error.h"
#include "interp/interp.h"

namespace sap
{
	ErrorMessage::ErrorMessage(Location loc, std::string msg) : m_location(std::move(loc)), m_message(std::move(msg))
	{
	}

	ErrorMessage::ErrorMessage(const interp::Typechecker* ts, std::string msg) : m_location(ts->loc()), m_message(std::move(msg))
	{
	}

	ErrorMessage::ErrorMessage(const interp::Evaluator* ev, std::string msg) : m_location(ev->loc()), m_message(std::move(msg))
	{
	}

	std::string ErrorMessage::string() const
	{
		return zpr::sprint("{}:{}:{}: error: {}", m_location.file, m_location.line + 1, m_location.column + 1, m_message);
	}

	void ErrorMessage::display() const
	{
		zpr::fprintln(stderr, "{}", this->string());
	}
}
