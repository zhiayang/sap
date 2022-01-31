// lexer.h
// Copyright (c) 2022, zhiayang
// SPDX-License-Identifier: Apache-2.0

#pragma once

#include <stdint.h>
#include <stddef.h>

#include "location.h"

namespace sap::frontend
{
	enum class TokenType
	{
		Invalid,
		EndOfFile,

		// common tokens
		Backslash,
		Comment,

		// text mode tokens
		Word,
		ParagraphBreak,

		// script mode tokens
	};

	struct Token
	{
		Location loc;
		TokenType type = TokenType::Invalid;
		zst::str_view text;

		inline operator TokenType() const { return this->type; }
		std::string str() const { return this->text.str(); }
	};






	/*
		This lexer has some internal state, so let's make it a struct. Also, due to
		the nature of this language, we cannot lex all tokens at once without some level
		of parsing, so we just fetch one token at a time.

		Lookahead is limited to one (for now) via `peek()`.
	*/
	struct Lexer
	{
		Lexer(zst::str_view contents, zst::str_view filename);

		Token peek() const;
		Token next();

	private:
		enum class Mode
		{
			Text
		};

		Mode m_mode = Mode::Text;
		zst::str_view m_stream {};
		Location m_location {};
	};
}
