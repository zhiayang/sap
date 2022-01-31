// lexer.cpp
// Copyright (c) 2022, zhiayang
// SPDX-License-Identifier: Apache-2.0

#include "frontend/lexer.h"

namespace sap::frontend
{
	static Token consume_text_token(zst::str_view& stream, Location& loc)
	{
		auto advance_and_return = [&](Token tok, size_t n) -> Token {
			loc.column += n;
			tok.loc.length = n;

			stream.remove_prefix(n);

			return tok;
		};

		// drop all leading horizontal whitespaces
		{
			while(stream.size() > 0)
			{
				if(stream[0] == ' ' || stream[0] == '\t')
					stream.remove_prefix(1);

				else if(stream[0] == '\n' && !stream.starts_with("\n\n"))
					stream.remove_prefix(1);

				else if(stream.starts_with("\r\n") && !stream.starts_with("\r\n\r\n"))
					stream.remove_prefix(2);

				else
					break;
			}
		}

		if(stream.empty())
		{
			return Token {
				.loc = loc,
				.type = TokenType::EndOfFile,
				.text = "",
			};
		}
		else if(stream.starts_with("#:"))
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

			return Token {
				.loc = begin_loc,
				.type = TokenType::Comment,
				.text = begin.drop_last(stream.size())
			};
		}
		else if(stream.starts_with(":#"))
		{
			sap::error(loc, "unexpected end of block comment");
		}
		else if(stream.starts_with("#"))
		{
			auto begin = stream;
			auto begin_loc = loc;
			while(!stream.empty() && stream[0] != '\n')
				loc.length += 1, loc.column += 1, stream.remove_prefix(1);

			assert(stream.empty() || stream[0] == '\n');
			stream.remove_prefix(1);
			loc.column = 0;
			loc.line += 1;

			return Token {
				.loc = begin_loc,
				.type = TokenType::Comment,
				.text = begin.drop_last(stream.size() + 1)
			};
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

			auto ret = advance_and_return(Token {
				.loc = loc,
				.type = TokenType::ParagraphBreak,
				.text = stream.take(num)
			}, num);

			loc.column = 0;
			loc.line += lines;
			return ret;
		}
		else if(stream[0] == '\\' && (stream.size() == 1 || (stream[1] != '\\' && stream[1] != '#')))
		{
			return advance_and_return(Token {
				.loc = loc,
				.type = TokenType::Backslash,
				.text = stream.take(1)
			}, 1);
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
					else
					{
						// emit a backslash and bail
						return advance_and_return(Token {
							.loc = loc,
							.type = TokenType::Backslash,
							.text = stream.take(1)
						}, 1);
					}
				}
				else if(stream[n] == ' ' || stream[n] == '\t' || stream[n] == '\n' || stream[n] == '\r')
				{
					return advance_and_return(Token {
						.loc = loc,
						.type = TokenType::Word,
						.text = stream.take(n)
					}, n);
				}
				else
				{
					n++;
				}
			}

			return advance_and_return(Token {
				.loc = loc,
				.type = TokenType::Word,
				.text = stream.take(n)
			}, n);
		}
	}




















	Lexer::Lexer(zst::str_view contents, zst::str_view filename) : m_stream(contents)
	{
		m_location = Location {
			.line = 0,
			.column = 0,
			.file = filename
		};
	}

	Token Lexer::peek() const
	{
		// copy them
		auto foo = m_stream;
		auto bar = m_location;

		if(m_mode == Mode::Text)
			return consume_text_token(foo, bar);
		else
			assert(false);
	}

	Token Lexer::next()
	{
		if(m_mode == Mode::Text)
			return consume_text_token(m_stream, m_location);
		else
			assert(false);
	}
}
