// error.cpp
// Copyright (c) 2022, zhiayang
// SPDX-License-Identifier: Apache-2.0

#include <unistd.h>

#include "sap/frontend.h"

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

	static void show_message(const char* error_text,
		const char* _error_colour,
		const Location& loc,
		const std::string& message)
	{
		bool coloured = isatty(STDERR_FILENO);

		const char* colour_error = coloured ? _error_colour : "";
		const char* colour_black_bold = coloured ? COLOUR_BLACK_BOLD : "";
		const char* colour_blue = coloured ? COLOUR_BLUE : "";
		const char* colour_reset = coloured ? COLOUR_RESET : "";

		zpr::fprintln(stderr, "{}{}:{} {}{}{}", colour_error, error_text, colour_reset, colour_black_bold, message,
			colour_reset);
		zpr::fprintln(stderr, "{} at:{} {}{}:{}:{}{}", colour_blue, colour_reset, colour_black_bold, loc.filename,
			loc.line + 1, loc.column + 1, colour_reset);

		if(loc.is_builtin)
			return;

		size_t tmp = loc.file_contents.take(loc.byte_offset).rfind('\n');
		zpr::println("byte offset = {}, tmp = {}", loc.byte_offset, tmp);

		if(tmp == std::string::npos)
			tmp = 0;
		else
			tmp += 1;

		auto current_line = loc.file_contents.drop(tmp).take_until('\n');
		current_line.remove_prefix((not current_line.empty() && current_line.back() == '\r') ? 1 : 0);

		auto line_num = std::to_string(loc.line + 1);
		auto line_num_padding = std::string(line_num.size(), ' ');

		size_t col = loc.column;
		size_t stripped_spaces = 0;
		while(not current_line.empty())
		{
			if(current_line[0] == ' ')
				col -= 1, stripped_spaces += 1, current_line.remove_prefix(1);
			else if(current_line[0] == '\t')
				col -= frontend::TAB_WIDTH, stripped_spaces += 1, current_line.remove_prefix(1);
			else
				break;
		}

		auto caret_spaces = std::string(col + 1, ' ');
		auto carets = std::string(loc.length, '^');

		zpr::fprintln(stderr, "{}{} |{}", colour_blue, line_num_padding, colour_reset);
		zpr::fprintln(stderr, "{}{} |  {} {}", colour_blue, line_num, colour_reset, current_line);
		zpr::fprintln(stderr, "{}{} |  {}{}{}{}{}", colour_blue, line_num_padding, colour_reset, colour_error,
			caret_spaces, carets, colour_reset);
		zpr::fprintln(stderr, "");
	}


	ErrorMessage::ErrorMessage(Location loc, const std::string& msg) : m_location(std::move(loc))
	{
		m_message = msg;
	}

	ErrorMessage::ErrorMessage(const interp::Typechecker* ts, const std::string& msg) : ErrorMessage(ts->loc(), msg)
	{
	}

	ErrorMessage::ErrorMessage(const interp::Evaluator* ev, const std::string& msg) : ErrorMessage(ev->loc(), msg)
	{
	}

	ErrorMessage& ErrorMessage::addInfo(Location loc, const std::string& msg)
	{
		m_infos.emplace_back(loc, msg);
		return *this;
	}

	ErrorMessage& ErrorMessage::addInfo(const interp::Typechecker* ts, const std::string& msg)
	{
		return this->addInfo(ts->loc(), msg);
	}

	ErrorMessage& ErrorMessage::addInfo(const interp::Evaluator* ev, const std::string& msg)
	{
		return this->addInfo(ev->loc(), msg);
	}

	const std::string& ErrorMessage::string() const
	{
		return m_message;
	}

	void ErrorMessage::display() const
	{
		show_message("error", COLOUR_RED_BOLD, m_location, m_message);
		for(auto& [loc, info] : m_infos)
			show_message("note", COLOUR_GREY_BOLD, loc, info);
	}

	[[noreturn]] void ErrorMessage::showAndExit() const
	{
		this->display();
		abort();
	}
}

namespace util::impl
{
	void log_impl(const std::string& msg)
	{
		using namespace sap;
		bool coloured = isatty(STDOUT_FILENO);

		const char* colour_grey_bold = coloured ? COLOUR_GREY_BOLD : "";
		const char* colour_reset = coloured ? COLOUR_RESET : "";

		zpr::fprintln(stderr, "{}[log]:{} {}", colour_grey_bold, colour_reset, msg);
	}
}
