// error.cpp
// Copyright (c) 2022, zhiayang
// SPDX-License-Identifier: Apache-2.0

#include "error.h"
#include "interp/interp.h"

namespace sap
{
	inline constexpr const char* COLOUR_RESET = "\033[0m";
	inline constexpr const char* COLOUR_BLACK = "\033[30m";
	inline constexpr const char* COLOUR_RED = "\033[31m";
	inline constexpr const char* COLOUR_GREEN = "\033[32m";
	inline constexpr const char* COLOUR_YELLOW = "\033[33m";
	inline constexpr const char* COLOUR_BLUE = "\033[34m";
	inline constexpr const char* COLOUR_MAGENTA = "\033[35m";
	inline constexpr const char* COLOUR_CYAN = "\033[36m";
	inline constexpr const char* COLOUR_WHITE = "\033[37m";
	inline constexpr const char* COLOUR_BLACK_BOLD = "\033[1m";
	inline constexpr const char* COLOUR_RED_BOLD = "\033[1m\033[31m";
	inline constexpr const char* COLOUR_GREEN_BOLD = "\033[1m\033[32m";
	inline constexpr const char* COLOUR_YELLOW_BOLD = "\033[1m\033[33m";
	inline constexpr const char* COLOUR_BLUE_BOLD = "\033[1m\033[34m";
	inline constexpr const char* COLOUR_MAGENTA_BOLD = "\033[1m\033[35m";
	inline constexpr const char* COLOUR_CYAN_BOLD = "\033[1m\033[36m";
	inline constexpr const char* COLOUR_WHITE_BOLD = "\033[1m\033[37m";
	inline constexpr const char* COLOUR_GREY_BOLD = "\033[30;1m";


	void showErrorMessage(const Location& loc, const std::string& message)
	{
		zpr::println("{}error:{} {}{}{}", COLOUR_RED_BOLD, COLOUR_RESET, COLOUR_BLACK_BOLD, message, COLOUR_RESET);
		zpr::println("{} at:{} {}{}:{}:{}{}", COLOUR_BLUE, COLOUR_RESET, COLOUR_BLACK_BOLD, loc.filename, loc.line + 1,
		    loc.column + 1, COLOUR_RESET);

		// search backwards from the token to find the newline
		size_t tmp = loc.file_contents.take(loc.byte_offset).rfind('\n');
		if(tmp == std::string::npos)
			tmp = 0;
		else
			tmp += 1;

		auto current_line = loc.file_contents.drop(tmp).take_until('\n');
		current_line.remove_prefix((not current_line.empty() && current_line.back() == '\r') ? 1 : 0);

		auto line_num = std::to_string(loc.line + 1);
		auto line_num_padding = std::string(line_num.size(), ' ');

		while(not current_line.empty())
		{
			if(util::is_one_of(current_line[0], ' ', '\t'))
				current_line.remove_prefix(1);
			else
				break;
		}

		zpr::println("{}{} |{}", COLOUR_BLUE, line_num_padding, COLOUR_RESET);
		zpr::println("{}{} |  {} {}", COLOUR_BLUE, line_num, COLOUR_RESET, current_line);
		zpr::println("{}{} |{}", COLOUR_BLUE, line_num_padding, COLOUR_RESET);
		zpr::println("");
	}




	ErrorMessage::ErrorMessage(Location loc, const std::string& msg) : m_location(std::move(loc))
	{
		// m_message = zpr::sprint("{}:{}:{}: error: {}", m_location.file, m_location.line + 1, m_location.column + 1, msg);
		m_message = msg;
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
		showErrorMessage(m_location, m_message);

		// zpr::fprintln(stderr, "{}", m_message);
	}

	[[noreturn]] void ErrorMessage::showAndExit() const
	{
		this->display();
		abort();
	}
}
