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
		CharLiteral,

		// stolen ƒrom PEP701
		FStringStart,
		FStringMiddle,
		FStringEnd,
		FStringDummy,

		LParen,
		RParen,
		LSquare,
		RSquare,
		LAngle,
		RAngle,
		Dollar,
		Colon,
		Comma,
		Period,
		Ampersand,
		Question,

		Semicolon,
		Plus,
		Minus,
		Asterisk,
		Slash,
		Equal,
		Percent,
		Exclamation,
		At,

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
		QuestionQuestion,
		QuestionPeriod,

		SlashSlash,
		SlashSlashEqual,
		SlashSlashQuestion,
		SlashSlashQuestionEqual,

		Ellipsis,

		RawBlock,

		// keywords
		KW_If,
		KW_Fn,
		KW_Or,
		KW_In,
		KW_For,
		KW_Mut,
		KW_Let,
		KW_Var,
		KW_And,
		KW_Not,
		KW_Else,
		KW_Enum,
		KW_True,
		KW_False,
		KW_While,
		KW_Break,
		KW_Using,
		KW_Union,
		KW_Struct,
		KW_Return,
		KW_Import,
		KW_Global,
		KW_Continue,
		KW_Namespace,
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
	public:
		enum class Mode
		{
			Text,
			Script,
			FString,
		};

	private:
		struct SaveState
		{
			std::vector<Mode> mode_stack {};
			zst::str_view stream {};
			Location location {};
		};

	public:
		Lexer(zst::str_view filename, zst::str_view contents);

		bool eof() const;
		Token peek() const;
		ErrorOr<Token> next();

		void skipWhitespaceAndComments();

		bool expect(TokenType type);
		std::optional<Token> match(TokenType type);

		bool expect(zst::str_view sv);

		Token peekWithMode(Mode mode) const;
		ErrorOr<Token> nextWithMode(Mode mode);

		SaveState save();
		void rewind(SaveState st);

		Mode mode() const;
		void pushMode(Mode mode);
		void popMode(Mode mode);

		zst::str_view stream() const { return m_stream; }

		Location location() const;
		void setLocation(Location loc) { m_location = std::move(loc); }

	private:
		void update_location() const;

	private:
		std::vector<Mode> m_mode_stack {};
		zst::str_view m_file_contents {};
		zst::str_view m_stream {};

		SaveState m_previous_token {};

		mutable Location m_location {};
	};

	struct LexerModer
	{
		LexerModer(Lexer& lexer, Lexer::Mode mode) : m_lexer(lexer), m_mode(mode) { m_lexer.pushMode(m_mode); }
		~LexerModer() { m_lexer.popMode(m_mode); }

	private:
		Lexer& m_lexer;
		Lexer::Mode m_mode;
	};


	constexpr inline size_t TAB_WIDTH = 4;
	ErrorOr<tree::Document> parse(zst::str_view filename, zst::str_view contents);
}
