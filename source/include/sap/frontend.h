// frontend.h
// Copyright (c) 2022, yuki / zhiayang
// SPDX-License-Identifier: Apache-2.0

#pragma once

#include "location.h"

namespace sap::tree
{
	struct Document;
}

namespace sap::frontend
{
#define TOKEN_TYPES_LIST       \
	X(Invalid)                 \
	X(EndOfFile)               \
	X(Backslash)               \
	X(Comment)                 \
	X(LBrace)                  \
	X(RBrace)                  \
	X(Text)                    \
	X(ParagraphBreak)          \
	X(Identifier)              \
	X(Number)                  \
	X(String)                  \
	X(CharLiteral)             \
	X(FStringStart)            \
	X(FStringMiddle)           \
	X(FStringEnd)              \
	X(FStringDummy)            \
	X(LParen)                  \
	X(RParen)                  \
	X(LSquare)                 \
	X(RSquare)                 \
	X(LAngle)                  \
	X(RAngle)                  \
	X(Dollar)                  \
	X(Colon)                   \
	X(Comma)                   \
	X(Period)                  \
	X(Ampersand)               \
	X(Question)                \
	X(Semicolon)               \
	X(Plus)                    \
	X(Minus)                   \
	X(Asterisk)                \
	X(Slash)                   \
	X(Equal)                   \
	X(Percent)                 \
	X(Exclamation)             \
	X(At)                      \
	X(RArrow)                  \
	X(ColonColon)              \
	X(LAngleEqual)             \
	X(RAngleEqual)             \
	X(EqualEqual)              \
	X(ExclamationEqual)        \
	X(PlusEqual)               \
	X(MinusEqual)              \
	X(AsteriskEqual)           \
	X(SlashEqual)              \
	X(PercentEqual)            \
	X(QuestionQuestion)        \
	X(QuestionPeriod)          \
	X(SlashSlash)              \
	X(SlashSlashEqual)         \
	X(SlashSlashQuestion)      \
	X(SlashSlashQuestionEqual) \
	X(Ellipsis)                \
	X(RawBlock)                \
	X(KW_If)                   \
	X(KW_Fn)                   \
	X(KW_Or)                   \
	X(KW_In)                   \
	X(KW_For)                  \
	X(KW_Mut)                  \
	X(KW_Let)                  \
	X(KW_Var)                  \
	X(KW_And)                  \
	X(KW_Not)                  \
	X(KW_Else)                 \
	X(KW_Enum)                 \
	X(KW_True)                 \
	X(KW_False)                \
	X(KW_While)                \
	X(KW_Break)                \
	X(KW_Using)                \
	X(KW_Union)                \
	X(KW_Struct)               \
	X(KW_Return)               \
	X(KW_Import)               \
	X(KW_Global)               \
	X(KW_Continue)             \
	X(KW_Namespace)

#define X(x) x,
	enum class TokenType
	{
		TOKEN_TYPES_LIST
	};
#undef X

#define X(x) { #x, TokenType::x },
	static const util::hashmap<std::string, TokenType> STRING_TO_TOKEN_TYPE = { TOKEN_TYPES_LIST };
#undef X

#define X(x) { TokenType::x, #x },
	static const util::hashmap<TokenType, std::string> TOKEN_TYPE_TO_STRING = { TOKEN_TYPES_LIST };
#undef X




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
