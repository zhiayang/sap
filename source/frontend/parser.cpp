// parser.cpp
// Copyright (c) 2022, zhiayang
// SPDX-License-Identifier: Apache-2.0

#include "interp/tree.h"
#include "interp/interp.h"

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

	constexpr auto KW_SCRIPT_BLOCK  = "script";

	static interp::QualifiedId parseQualifiedId(Lexer& lexer)
	{
		// TODO: parse qualified ids. for now we just parse normal ids.
		auto tok = lexer.next();
		if(tok != TT::Identifier)
			error(tok.loc, "expected identifier");

		return interp::QualifiedId {
			.name = tok.text.str()
		};
	}

	static std::unique_ptr<interp::Expr> parseExpr(Lexer& lexer)
	{
		return {};
	}



	static std::unique_ptr<ScriptBlock> parseScriptBlock(Lexer& lexer)
	{
		auto lm = LexerModer(lexer, Lexer::Mode::Script);
		return {};
	}

	static std::unique_ptr<ScriptCall> parseInlineScript(Lexer& lexer)
	{
		auto lm = LexerModer(lexer, Lexer::Mode::Script);

		auto qid = parseQualifiedId(lexer);
		auto open_paren = lexer.next();
		if(open_paren != TT::LParen)
			error(open_paren.loc, "expected '(' after qualified-id in inline script call");

		auto call = std::make_unique<interp::FunctionCall>();

		// parse arguments.
		while(lexer.peek() != TT::RParen)
		{
			bool flag = false;

			interp::FunctionCall::Arg arg {};
			if(lexer.peek() == TT::Identifier)
			{
				auto save = lexer.save();
				auto name = lexer.next();

				if(lexer.expect(TT::Colon))
				{
					arg.value = parseExpr(lexer);
					arg.name = name.text.str();
					flag = true;
				}
				else
				{
					lexer.rewind(save);
				}
			}

			if(not flag)
			{
				arg.name = {};
				arg.value = parseExpr(lexer);
			}

			call->arguments.push_back(std::move(arg));

			if(not lexer.expect(TT::Comma) && lexer.peek() != TT::RParen)
				error(lexer.peek().loc, "expected ',' or ')' in call argument list");
		}

		if(not lexer.expect(TT::RParen))
			error(lexer.peek().loc, "expected ')'");

		// TODO: (potentially multiple) trailing blocks

		auto sc = std::make_unique<ScriptCall>();
		sc->call = std::move(call);

		return sc;
	}


	static std::unique_ptr<Paragraph> parseParagraph(Lexer& lexer)
	{
		auto para = std::make_unique<Paragraph>();
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
			else if(tok == TT::Backslash)
			{
				auto next = lexer.peekWithMode(Lexer::Mode::Script);
				if(next == TT::Identifier)
				{
					if(next.text == KW_SCRIPT_BLOCK)
						para->addObject(parseScriptBlock(lexer));
					else
						para->addObject(parseInlineScript(lexer));
				}
				else
				{
					error(next.loc, "unexpected token '{}' after '\\'", next.text);
				}
			}
			else
			{
				error(tok.loc, "unexpected token '{}' in paragraph", tok.text);
			}
		}

		return para;
	}


	static std::unique_ptr<DocumentObject> parseTopLevel(Lexer& lexer)
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

			if(tok == TT::EndOfFile)
				break;

			document.addObject(parseTopLevel(lexer));
		}

		return document;
	}
}
