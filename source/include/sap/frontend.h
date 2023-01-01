// frontend.h
// Copyright (c) 2022, zhiayang
// SPDX-License-Identifier: Apache-2.0

#pragma once

#include "location.h" // for Location

namespace sap::tree
{
	struct Document;
}

namespace sap::frontend
{
	enum class TokenType
	{
		Invalid,
		EndOfFile,

		// common tokens
		Backslash,
		Comment,
		LBrace,
		RBrace,

		// text mode tokens
		Text,
		ParagraphBreak,

		// script mode tokens
		Identifier,
		Number,
		String,

		LParen,
		RParen,
		LSquare,
		RSquare,
		LAngle,
		RAngle,
		Colon,
		Comma,
		Period,
		Ampersand,

		Semicolon,
		Plus,
		Minus,
		Asterisk,
		Slash,
		Equal,
		Percent,

		RArrow,
		ColonColon,
		LAngleEqual,
		RAngleEqual,
		EqualEqual,
		ExclamationEqual,

		PlusEqual,
		MinusEqual,
		AsteriskEqual,
		SlashEqual,
		PercentEqual,

		Ellipsis,
	};

	struct Token
	{
		Location loc;
		TokenType type = TokenType::Invalid;
		bool whitespace_before = false;
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
	private:
		struct SaveState
		{
			zst::str_view stream {};
			Location location {};
		};

	public:
		enum class Mode
		{
			Text,
			Script,
		};

		Lexer(zst::str_view filename, zst::str_view contents);

		Token peek() const;
		Token next();
		bool eof() const;

		bool isWhitespace() const;

		bool expect(TokenType type);
		std::optional<Token> match(TokenType type);

		Token peekWithMode(Mode mode) const;

		SaveState save();
		void rewind(SaveState st);

		Mode mode() const;
		void pushMode(Mode mode);
		void popMode(Mode mode);

		Location location() const { return m_location; }

	private:
		std::vector<Mode> m_mode_stack {};
		zst::str_view m_stream {};
		Location m_location {};
	};

	struct LexerModer
	{
		LexerModer(Lexer& lexer, Lexer::Mode mode) : m_lexer(lexer), m_mode(mode) { m_lexer.pushMode(m_mode); }
		~LexerModer() { m_lexer.popMode(m_mode); }

	private:
		Lexer& m_lexer;
		Lexer::Mode m_mode;
	};



	tree::Document parse(zst::str_view filename, zst::str_view contents);
}
