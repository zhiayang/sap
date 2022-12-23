// parser.cpp
// Copyright (c) 2022, zhiayang
// SPDX-License-Identifier: Apache-2.0

#include "interp/tree.h"
#include "interp/interp.h"

#include "sap/frontend.h"
#include "util.h"
#include "zpr.h"
#include "zst.h"
#include <string>
#include <string_view>

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
				if(text[i] == '\\')
					ret.push_back('\\');
				else if(text[i] == '{')
					ret.push_back('{');
				else if(text[i] == '}')
					ret.push_back('}');
				else if(text[i] == '#')
					ret.push_back('#');
				else
					error(loc, "unrecognised escape sequence '\\{}'", text[i]);
			}
			else
			{
				ret.push_back(text[i]);
			}
		}

		return ret;
	}

	static int get_front_token_precedence(Lexer& lexer)
	{
		switch(lexer.peek())
		{
			case TT::LParen:
				return 1000;

			case TT::Asterisk:
			case TT::Slash:
				return 400;

			case TT::Plus:
			case TT::Minus:
				return 300;

			default:
				return -1;
		}
	}

	static bool is_right_associative(Lexer& lexer)
	{
		return false;
	}

	static bool is_postfix_unary(Lexer& lexer)
	{
		return lexer.peek() == TT::LParen || lexer.peek() == TT::LSquare;
	}

	static interp::Op parseOperatorTokens(Lexer& lexer)
	{
		switch(lexer.peek())
		{
			case TT::Plus:
				lexer.next();
				return interp::Op::Add;
			case TT::Minus:
				lexer.next();
				return interp::Op::Subtract;
			case TT::Asterisk:
				lexer.next();
				return interp::Op::Multiply;
			case TT::Slash:
				lexer.next();
				return interp::Op::Divide;
			default:
				error(lexer.peek().loc, "unknown operator token '{}'", lexer.peek().text);
		}
	}


	constexpr auto KW_LET = "let";
	constexpr auto KW_SCRIPT_BLOCK = "script";

	static std::unique_ptr<interp::Expr> parseExpr(Lexer& lexer);
	static std::unique_ptr<interp::Expr> parseUnary(Lexer& lexer);



	static interp::QualifiedId parseQualifiedId(Lexer& lexer)
	{
		interp::QualifiedId qid {};
		while(not lexer.eof())
		{
			auto tok = lexer.next();
			if(tok != TT::Identifier)
				error(tok.loc, "expected identifier");

			if(lexer.expect(TT::ColonColon))
			{
				qid.parents.push_back(tok.text.str());
			}
			else
			{
				qid.name = tok.text.str();
				break;
			}
		}

		return qid;
	}

	static std::unique_ptr<interp::FunctionCall> parseFunctionCall(Lexer& lexer, std::unique_ptr<interp::Expr> callee)
	{
		if(not lexer.expect(TT::LParen))
			error(lexer.peek().loc, "expected '(' to begin function call");

		auto call = std::make_unique<interp::FunctionCall>();
		call->callee = std::move(callee);

		// parse arguments.
		bool have_named_param = false;
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

					have_named_param = true;
					flag = true;
				}
				else
				{
					lexer.rewind(save);
				}
			}

			if(not flag)
			{
				if(have_named_param)
					error(lexer.peek().loc, "positional arguments are not allowed after named arguments");

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

		return call;
	}

	static std::unique_ptr<interp::Expr> parsePostfixUnary(Lexer& lexer, std::unique_ptr<interp::Expr> lhs)
	{
		if(lexer.peek() == TT::LParen)
			return parseFunctionCall(lexer, std::move(lhs));

		error(lexer.peek().loc, "expected '(' or '[' after expression, found '{}'", lexer.peek().text);
	}


	static std::unique_ptr<interp::Expr> parseRhs(Lexer& lexer, std::unique_ptr<interp::Expr> lhs, int prio)
	{
		if(lexer.eof())
			return lhs;

		while(true)
		{
			int prec = get_front_token_precedence(lexer);
			if(prec < prio && !is_right_associative(lexer))
				return lhs;

			// we don't really need to check, because if it's botched we'll have returned due to -1 < everything
			if(is_postfix_unary(lexer))
			{
				lhs = parsePostfixUnary(lexer, std::move(lhs));
				continue;
			}

			// auto op_loc = lexer.peek().loc;
			auto op = parseOperatorTokens(lexer);

			auto rhs = parseUnary(lexer);
			int next = get_front_token_precedence(lexer);

			if(next > prec || is_right_associative(lexer))
				rhs = parseRhs(lexer, std::move(rhs), prec + 1);

			if(interp::isAssignmentOp(op))
			{
				error(lexer.peek().loc, "asdf");
				// auto newlhs = util::pool<AssignOp>(loc);

				// newlhs->left = dcast(Expr, lhs);
				// newlhs->right = rhs;
				// newlhs->op = op;

				// lhs = newlhs;
			}
			else if(interp::isComparisonOp(op))
			{
				error(lexer.peek().loc, "aoeu");
				// if(auto cmp = dcast(ComparisonOp, lhs))
				// {
				// 	cmp->ops.push_back({ op, loc });
				// 	cmp->exprs.push_back(rhs);
				// 	lhs = cmp;
				// }
				// else
				// {
				// 	iceAssert(lhs);
				// 	auto newlhs = util::pool<ComparisonOp>(loc);
				// 	newlhs->exprs.push_back(lhs);
				// 	newlhs->exprs.push_back(rhs);
				// 	newlhs->ops.push_back({ op, loc });

				// 	lhs = newlhs;
				// }
			}
			else
			{
				auto tmp = std::make_unique<interp::BinaryOp>();
				tmp->lhs = std::move(lhs);
				tmp->rhs = std::move(rhs);
				tmp->op = op;

				lhs = std::move(tmp);
			}
		}
	}


	static std::u32string unescape_string(const Location& loc, zst::str_view sv)
	{
		std::u32string ret {};
		ret.reserve(sv.size());

		auto bs = sv.bytes();
		while(bs.size() > 0)
		{
			auto cp = unicode::consumeCodepointFromUtf8(bs);
			if(cp == U'\\')
			{
				auto is_hex = [](uint8_t c) -> bool {
					return ('0' <= c && c <= '9') || ('a' <= c && c <= 'f') || ('A' <= c && c <= 'F');
				};

				auto convert_hex = [](uint8_t c) -> char32_t {
					if('0' <= c && c <= '9')
						return static_cast<char32_t>(c - '0');
					else if('a' <= c && c <= 'f')
						return static_cast<char32_t>(10 + c - 'a');
					else if('A' <= c && c <= 'F')
						return static_cast<char32_t>(10 + c - 'A');
					else
						return 0;
				};

				cp = unicode::consumeCodepointFromUtf8(bs);
				switch(cp)
				{
					case U'\\':
						ret += U'\\';
						break;
					case U'n':
						ret += U'\n';
						break;
					case U't':
						ret += U'\t';
						break;
					case U'b':
						ret += U'\b';
						break;
					case U'x':
						assert(bs.size() >= 2);
						assert(is_hex(bs[0]) && is_hex(bs[1]));
						ret += 16u * convert_hex(bs[0]) + convert_hex(bs[1]);
						break;
					case U'u':
						assert(bs.size() >= 4);
						assert(is_hex(bs[0]) && is_hex(bs[1]) && is_hex(bs[2]) && is_hex(bs[3]));
						ret += 16u * 16u * 16u * convert_hex(bs[0])  //
						     + 16u * 16u * convert_hex(bs[1])        //
						     + 16u * convert_hex(bs[2])              //
						     + convert_hex(bs[3]);
						break;
					case U'U':
						assert(bs.size() >= 8);
						assert(is_hex(bs[0]) && is_hex(bs[1]) && is_hex(bs[2]) && is_hex(bs[3]));
						assert(is_hex(bs[4]) && is_hex(bs[5]) && is_hex(bs[6]) && is_hex(bs[7]));

						ret += 16u * 16u * 16u * 16u * 16u * 16u * 16u * convert_hex(bs[0])  //
						     + 16u * 16u * 16u * 16u * 16u * 16u * convert_hex(bs[1])        //
						     + 16u * 16u * 16u * 16u * 16u * convert_hex(bs[2])              //
						     + 16u * 16u * 16u * 16u * convert_hex(bs[3])                    //
						     + 16u * 16u * 16u * convert_hex(bs[4])                          //
						     + 16u * 16u * convert_hex(bs[5])                                //
						     + 16u * convert_hex(bs[6])                                      //
						     + convert_hex(bs[7]);
						break;
					default:
						sap::error(loc, "invalid escape sequence starting with '{}'", cp);
						break;
				}
			}
			else
			{
				ret += cp;
			}
		}

		return ret;
	}

	static std::unique_ptr<interp::Expr> parsePrimary(Lexer& lexer)
	{
		if(lexer.peek() == TT::Identifier)
		{
			auto qid = parseQualifiedId(lexer);
			auto ident = std::make_unique<interp::Ident>();
			ident->name = std::move(qid);
			return ident;
		}
		else if(auto num = lexer.match(TT::Number); num)
		{
			auto ret = std::make_unique<interp::NumberLit>();
			ret->is_floating = false;
			ret->int_value = std::stoll(num->text.str());
			ret->float_value = 0;

			return ret;
		}
		else if(auto str = lexer.match(TT::String); str)
		{
			auto ret = std::make_unique<interp::StringLit>();
			ret->string = unescape_string(str->loc, str->text);

			return ret;
		}
		else
		{
			error(lexer.peek().loc, "invalid start of expression '{}'", lexer.peek().text);
		}
	}

	static std::unique_ptr<interp::Expr> parseUnary(Lexer& lexer)
	{
		return parsePrimary(lexer);
	}

	static std::unique_ptr<interp::Expr> parseExpr(Lexer& lexer)
	{
		auto lm = LexerModer(lexer, Lexer::Mode::Script);
		auto lhs = parseUnary(lexer);

		return parseRhs(lexer, std::move(lhs), 0);
	}



	static std::unique_ptr<interp::Stmt> parseStmt(Lexer& lexer)
	{
		auto lm = LexerModer(lexer, Lexer::Mode::Script);

		auto tok = lexer.peek();
		std::unique_ptr<interp::Stmt> stmt {};

		switch(tok.type)
		{
			case TT::Identifier:
				if(tok.text == KW_LET)
				{
					error(tok.loc, "TODO: 'let' not implemented");
				}
				else
				{
					stmt = parseExpr(lexer);
				}
				break;

			default:
				stmt = parseExpr(lexer);
				break;
		}

		if(not stmt)
			error(tok.loc, "invalid start of statement");

		else if(not lexer.expect(TT::Semicolon))
			error(lexer.peek().loc, "expected ';' after statement, found '{}'", lexer.peek().text);

		return stmt;
	}


	static std::unique_ptr<ScriptBlock> parseScriptBlock(Lexer& lexer)
	{
		auto lm = LexerModer(lexer, Lexer::Mode::Script);
		if(auto x = lexer.match(TT::Identifier); not x || x->text != KW_SCRIPT_BLOCK)
			error(Location {}, "?????");

		auto openbrace = lexer.next();
		if(openbrace != TT::LBrace)
			error(openbrace.loc, "expected '{' after {}", KW_SCRIPT_BLOCK);

		auto block = std::make_unique<ScriptBlock>();
		while(lexer.peek() != TT::RBrace)
			block->statements.push_back(parseStmt(lexer));

		if(lexer.eof())
			error(openbrace.loc, "unexpected end of file; unclosed '{' here");

		else if(not lexer.match(TT::RBrace))
			error(openbrace.loc, "expected '}' to match '{' here");

		return block;
	}

	static std::unique_ptr<ScriptCall> parseInlineScript(Lexer& lexer)
	{
		auto lm = LexerModer(lexer, Lexer::Mode::Script);

		auto qid = parseQualifiedId(lexer);
		auto lhs = std::make_unique<interp::Ident>();
		lhs->name = std::move(qid);

		auto open_paren = lexer.peek();
		if(open_paren != TT::LParen)
			error(open_paren.loc, "expected '(' after qualified-id in inline script call");

		auto sc = std::make_unique<ScriptCall>();
		sc->call = parseFunctionCall(lexer, std::move(lhs));

		return sc;
	}

	static std::unique_ptr<ScriptObject> parseScriptObject(Lexer& lexer)
	{
		auto next = lexer.peekWithMode(Lexer::Mode::Script);
		if(next == TT::Identifier)
		{
			if(next.text == KW_SCRIPT_BLOCK)
				return parseScriptBlock(lexer);
			else
				return parseInlineScript(lexer);
		}
		else
		{
			error(next.loc, "unexpected token '{}' after '\\'", next.text);
		}
	}


	static std::unique_ptr<Paragraph> parseParagraph(Lexer& lexer)
	{
		auto para = std::make_unique<Paragraph>();

		bool was_sticking_right = false;
		while(true)
		{
			bool was_whitespace = lexer.isWhitespace();

			auto tok = lexer.next();

			if(tok == TT::Word)
			{
				auto word = std::make_shared<Word>(escape_word_text(tok.loc, tok.text));

				// if the next word sticks to the left, that means this word should stick to the right.
				if(not lexer.isWhitespace())
					word->stick_to_right = true;

				// and vice-versa for left
				if(was_sticking_right)
					word->stick_to_left = true;

				para->addObject(std::move(word));
				was_sticking_right = false;
			}
			else if(tok == TT::ParagraphBreak || tok == TT::EndOfFile)
			{
				break;
			}
			else if(tok == TT::Backslash)
			{
				was_sticking_right = false;

				auto obj = parseScriptObject(lexer);

				if(auto inl = dynamic_cast<InlineObject*>(obj.get()); inl != nullptr)
				{
					inl->stick_to_left = not was_whitespace;
					inl->stick_to_right = not lexer.isWhitespace();

					was_sticking_right = inl->stick_to_right;
				}

				para->addObject(std::move(obj));
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
			else if(tok == TT::Backslash)
			{
				lexer.next();
				return parseScriptObject(lexer);
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
