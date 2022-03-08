// parser.cpp
// Copyright (c) 2022, zhiayang
// SPDX-License-Identifier: Apache-2.0

#include "interp/tree.h"
#include "sap/frontend.h"

namespace sap::frontend
{
	using TT = TokenType;
	using namespace sap::tree;

	static std::string escape_word_text(const Location& loc, zst::str_view text)
	{
		std::string ret {};
		ret.reserve(text.size());

		for(size_t i = 0; i < text.size(); i++)
		{
			if(text[i] == '\\')
			{
				// lexer should've caught this
				assert(i + 1 < text.size());

				i += 1;
				if(text[i] == '\\')     ret.push_back('\\');
				else if(text[i] == '{') ret.push_back('{');
				else if(text[i] == '}') ret.push_back('}');
				else if(text[i] == '#') ret.push_back('#');
				else                    error(loc, "unrecognised escape sequence '\\{}'", text[i]);
			}
			else
			{
				ret.push_back(text[i]);
			}
		}

		return ret;
	}

	static std::shared_ptr<Paragraph> parseParagraph(Lexer& lexer)
	{
		auto para = std::make_shared<Paragraph>();
		while(true)
		{
			auto tok = lexer.next();
			if(tok == TT::Word)
			{
				para->addObject(std::make_shared<Word>(escape_word_text(tok.loc, tok.text)));
			}
			else if(tok == TT::ParagraphBreak || tok == TT::EndOfFile)
			{
				break;
			}
			else
			{
				error(tok.loc, "unexpected token '{}' in paragraph", tok.text);
			}
		}

		return para;
	}


	static std::shared_ptr<DocumentObject> parseTopLevel(Lexer& lexer)
	{
		while(true)
		{
			if(auto tok = lexer.peek(); tok == TT::Word)
			{
				return parseParagraph(lexer);
			}
			else if(tok == TT::ParagraphBreak)
			{
				lexer.next();
				continue;
			}
			else if(tok == TT::EndOfFile)
			{
				error(tok.loc, "unexpected end of file");
			}
			else
			{
				error(tok.loc, "unexpected token '{}' ({}) at top level", tok.text, tok.type);
			}
		}
	}

	Document parse(zst::str_view filename, zst::str_view contents)
	{
		auto lexer = Lexer(filename, contents);
		Document document {};

		while(true)
		{
			lexer.skipComments();
			auto tok = lexer.peek();

			if(tok == TokenType::EndOfFile)
				break;

			document.addObject(parseTopLevel(lexer));
		}

		return document;
	}
}
