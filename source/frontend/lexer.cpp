// lexer.cpp
// Copyright (c) 2022, zhiayang
// SPDX-License-Identifier: Apache-2.0

#include <ctype.h>

#include "error.h"
#include "sap/frontend.h"

namespace sap::frontend
{
	using TT = TokenType;

	static Token advance_and_return(zst::str_view& stream, Location& loc, Token tok, size_t n)
	{
		loc.column += n;
		tok.loc.length = n;

		stream.remove_prefix(n);
		return tok;
	}

	static Token parse_comment(zst::str_view& stream, Location& loc)
	{
		if(stream.starts_with("#:"))
		{
			auto begin = stream;
			auto begin_loc = loc;

			std::vector<Location> start_locs {};
			while(true)
			{
				if(stream.empty())
					sap::error(start_locs.back(), "unterminated block comment (reached end of file)");

				size_t n = 1;
				if(stream.starts_with("\r\n"))
				{
					loc.column = 0;
					loc.line += 1;
					n = 2;
				}
				else if(stream.starts_with("#:"))
				{
					start_locs.push_back(loc);
					n = 2;
				}
				else if(stream.starts_with(":#"))
				{
					assert(start_locs.size() > 0);
					start_locs.pop_back();
					n = 2;

					if(start_locs.empty())
						break;
				}
				else if(stream[0] == '\n')
				{
					loc.column = 0;
					loc.line += 1;
				}
				else
				{
					loc.column += 1;
				}

				stream.remove_prefix(n);
			}

			assert(stream.starts_with(":#"));
			stream.remove_prefix(2);

			return Token { .loc = begin_loc,
				.type = TT::Comment,
				.whitespace_before = false,
				.text = begin.drop_last(stream.size()) };
		}
		else if(stream.starts_with("#"))
		{
			auto begin = stream;
			auto begin_loc = loc;
			while(not stream.empty() && stream[0] != '\n')
				loc.length += 1, loc.column += 1, stream.remove_prefix(1);

			assert(stream.empty() || stream[0] == '\n');
			stream.remove_prefix(1);
			loc.column = 0;
			loc.line += 1;

			return Token { .loc = begin_loc,
				.type = TT::Comment,
				.whitespace_before = false,
				.text = begin.drop_last(stream.size() + 1) };
		}
		else
		{
			assert(not "not a comment, hello?!");
		}
	}

	static Token consume_text_token(zst::str_view& stream, Location& loc)
	{
		bool ws_before = false;
		// drop all leading horizontal whitespaces
		{
			while(stream.size() > 0)
			{
				if(stream[0] == ' ' || stream[0] == '\t')
					ws_before = true, loc.column++, stream.remove_prefix(1);

				else if(stream[0] == '\n' && not stream.starts_with("\n\n"))
					ws_before = true, loc.column = 0, loc.line++, stream.remove_prefix(1);

				else if(stream.starts_with("\r\n") && not stream.starts_with("\r\n\r\n"))
					ws_before = true, loc.column = 0, loc.line++, stream.remove_prefix(2);

				else
					break;
			}
		}

		if(stream.empty())
		{
			return Token {
				.loc = loc,
				.type = TT::EndOfFile,
				.whitespace_before = ws_before,
				.text = "",
			};
		}
		else if(stream.starts_with("#"))
		{
			auto tok = parse_comment(stream, loc);
			tok.whitespace_before = ws_before;
			return tok;
		}
		else if(stream.starts_with(":#"))
		{
			sap::error(loc, "unexpected end of block comment");
		}
		else if(stream.starts_with("\n\n") || stream.starts_with("\r\n\r\n"))
		{
			size_t lines = 0;
			size_t num = 0;
			while(num < stream.size())
			{
				if(stream.drop(num).starts_with("\n"))
					lines++, num += 1;
				else if(stream.drop(num).starts_with("\r\n"))
					lines++, num += 2;
				else
					break;
			}

			auto ret = advance_and_return(stream, loc,
				Token { .loc = loc, .type = TT::ParagraphBreak, .whitespace_before = ws_before, .text = stream.take(num) }, num);

			loc.column = 0;
			loc.line += lines;
			return ret;
		}
		else if(stream[0] == '{' || stream[0] == '}')
		{
			return advance_and_return(stream, loc,
				Token { .loc = loc,
					.type = (stream[0] == '{' ? TT::LBrace : TT::RBrace),
					.whitespace_before = ws_before,
					.text = stream.take(1) },
				1);
		}
		else if(stream[0] == '\\' &&
				(stream.size() == 1 || (stream[1] != '\\' && stream[1] != '#' && stream[1] != '{' && stream[1] != '}')))
		{
			return advance_and_return(stream, loc,
				Token { .loc = loc, .type = TT::Backslash, .whitespace_before = ws_before, .text = stream.take(1) }, 1);
		}
		else
		{
			// normal tokens, and parse escape sequences.
			size_t n = 0;
			while(n < stream.size())
			{
				if(stream[n] == '\\')
				{
					if(n + 1 >= stream.size())
						sap::error(loc, "unterminated '\\' escape sequence");
					else if(stream[n + 1] == '\\')
						n += 2;
					else if(stream[n + 1] == '#')
						n += 2;
					else if(stream[n + 1] == '{')
						n += 2;
					else if(stream[n + 1] == '}')
						n += 2;
					else
					{
						// this really shouldn't be the first char, because we check for non-{}# escapes
						// above, before this clause
						assert(n > 0);

						// so, we emit whatever we have in the current word, then bail.
						break;
					}
				}
				else if(stream[n] == ' ' || stream[n] == '\t' || stream[n] == '\n' || stream[n] == '\r')
				{
					return advance_and_return(stream, loc,
						Token { .loc = loc, .type = TT::Word, .whitespace_before = ws_before, .text = stream.take(n) }, n);
				}
				else
				{
					n++;
				}
			}

			// note: this does imply that the parser needs to "re-parse" any escape sequences, but that's fine
			// because we don't want to deal with lifetime issues regarding a constructed string in the Token.
			return advance_and_return(stream, loc,
				Token { .loc = loc, .type = TT::Word, .whitespace_before = ws_before, .text = stream.take(n) }, n);
		}
	}

	static Token consume_script_token(zst::str_view& stream, Location& loc)
	{
		// skip whitespace
		while(stream.size() > 0)
		{
			if(stream[0] == ' ' || stream[0] == '\t' || stream[0] == '\r' || stream[0] == '\n')
				loc.column++, stream.remove_prefix(1);
			else
				break;
		}

		if(stream.empty())
		{
			return Token { .loc = loc, .type = TT::EndOfFile, .text = "" };
		}

		else if(stream.starts_with("#"))
		{
			return parse_comment(stream, loc);
		}
		else if(stream.starts_with(":#"))
		{
			sap::error(loc, "unexpected end of block comment");
		}

		// TODO: unicode identifiers
		else if(isascii(stream[0]) && (isalpha(stream[0]) || stream[0] == '_'))
		{
			size_t n = 0;
			while(isascii(stream[n]) && (isdigit(stream[n]) || isalpha(stream[n]) || stream[n] == '_'))
				n++;

			return advance_and_return(stream, loc, Token { .loc = loc, .type = TT::Identifier, .text = stream.take(n) }, n);
		}
		else if(isascii(stream[0]) && isdigit(stream[0]))
		{
			size_t n = 0;
			while(isascii(stream[n]) && isdigit(stream[n]))
				n++;

			return advance_and_return(stream, loc, Token { .loc = loc, .type = TT::Number, .text = stream.take(n) }, n);
		}
		else if(stream.starts_with("::"))
		{
			return advance_and_return(stream, loc, Token { .loc = loc, .type = TT::ColonColon, .text = stream.take(2) }, 2);
		}
		else
		{
			auto tt = TT::Invalid;
			switch(stream[0])
			{
				case '(':
					tt = TT::LParen;
					break;
				case ')':
					tt = TT::RParen;
					break;
				case ',':
					tt = TT::Comma;
					break;
				case ':':
					tt = TT::Colon;
					break;
				case '{':
					tt = TT::LBrace;
					break;
				case '}':
					tt = TT::RBrace;
					break;
				case '+':
					tt = TT::Plus;
					break;
				case '-':
					tt = TT::Minus;
					break;
				case '*':
					tt = TT::Asterisk;
					break;
				case '/':
					tt = TT::Slash;
					break;
				case '=':
					tt = TT::Equal;
					break;
				case ';':
					tt = TT::Semicolon;
					break;

				default:
					sap::error(loc, "unknown token '{}'", stream[0]);
					break;
			}

			return advance_and_return(stream, loc, Token { .loc = loc, .type = tt, .text = stream.take(1) }, 1);
		}


		return {};
	}


















	Lexer::Lexer(zst::str_view filename, zst::str_view contents) : m_stream(contents)
	{
		m_mode_stack.push_back(Mode::Text);

		m_location = Location { .line = 0, .column = 0, .file = filename };
	}

	Token Lexer::peekWithMode(Lexer::Mode mode) const
	{
		// copy them
		auto foo = m_stream;
		auto bar = m_location;

		// TODO: figure out how we wanna handle comments
		if(mode == Mode::Text)
		{
			Token ret {};
			do
			{
				ret = consume_text_token(foo, bar);
			} while(ret == TT::Comment);
			return ret;
		}
		else if(mode == Mode::Script)
		{
			Token ret {};
			do
			{
				ret = consume_script_token(foo, bar);
			} while(ret == TT::Comment);
			return ret;
		}
		else
		{
			assert(false);
		}
	}

	Token Lexer::peek() const
	{
		return this->peekWithMode(this->mode());
	}

	bool Lexer::eof() const
	{
		return this->peek() == TT::EndOfFile;
	}

	Token Lexer::next()
	{
		if(this->mode() == Mode::Text)
		{
			Token ret {};
			do
			{
				ret = consume_text_token(m_stream, m_location);
			} while(ret == TT::Comment);
			return ret;
		}
		else if(this->mode() == Mode::Script)
		{
			Token ret {};
			do
			{
				ret = consume_script_token(m_stream, m_location);
			} while(ret == TT::Comment);
			return ret;
		}
		else
		{
			assert(false);
		}
	}

	bool Lexer::expect(TokenType type)
	{
		if(this->peek() == type)
		{
			this->next();
			return true;
		}

		return false;
	}

	std::optional<Token> Lexer::match(TokenType type)
	{
		if(this->peek() == type)
			return this->next();

		return {};
	}

	Lexer::Mode Lexer::mode() const
	{
		if(m_mode_stack.empty())
			sap::internal_error("lexer entered invalid mode");
		return m_mode_stack.back();
	}

	void Lexer::pushMode(Lexer::Mode mode)
	{
		m_mode_stack.push_back(mode);
	}

	void Lexer::popMode(Lexer::Mode mode)
	{
		if(m_mode_stack.empty() || m_mode_stack.back() != mode)
		{
			sap::internal_error("unbalanced mode stack: {} / {}", m_mode_stack.back(), mode);
		}

		m_mode_stack.pop_back();
	}

	Lexer::SaveState Lexer::save()
	{
		return SaveState { .stream = m_stream, .location = m_location };
	}

	void Lexer::rewind(SaveState st)
	{
		m_stream = st.stream;
		m_location = st.location;
	}

	void Lexer::skipComments()
	{
		while(this->peek() == TT::Comment)
			this->next();
	}
}
