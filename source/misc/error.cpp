// error.cpp
// Copyright (c) 2022, zhiayang
// SPDX-License-Identifier: Apache-2.0

#include "error.h"
#include "interp/interp.h"

namespace sap
{
	ErrorMessage::ErrorMessage(Location loc, const std::string& msg) : m_location(std::move(loc))
	{
		m_message = zpr::sprint("{}:{}:{}: error: {}", m_location.file, m_location.line + 1, m_location.column + 1, msg);
	}

	ErrorMessage::ErrorMessage(const interp::Typechecker* ts, const std::string& msg) : ErrorMessage(ts->loc(), msg)
	{
	}

	ErrorMessage::ErrorMessage(const interp::Evaluator* ev, const std::string& msg) : ErrorMessage(ev->loc(), msg)
	{
	}

	const std::string& ErrorMessage::string() const
	{
		return m_message;
	}

	void ErrorMessage::display() const
	{
		zpr::fprintln(stderr, "{}", m_message);
	}

	[[noreturn]] void ErrorMessage::showAndExit() const
	{
		this->display();
		abort();
	}
}
