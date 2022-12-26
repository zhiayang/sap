// parser.cpp
// Copyright (c) 2022, zhiayang
// SPDX-License-Identifier: Apache-2.0

#include "util.h"     // for consumeCodepointFromUtf8
#include "location.h" // for error, Location

#include "sap/frontend.h" // for Token, Lexer, Lexer::Mode, Lexer::Mode:...

#include "interp/tree.h"     // for Paragraph, ScriptBlock, ScriptCall, Text
#include "interp/interp.h"   // for Expr, FunctionCall::Arg, FunctionCall
#include "interp/basedefs.h" // for DocumentObject

namespace sap::frontend
{
	using TT = TokenType;
	using namespace sap::tree;

	static std::u32string escape_word_text(const Location& loc, zst::str_view text_)
	{
		std::u32string ret {};
		auto text = text_.bytes();

		ret.reserve(text.size());


		while(text.size() > 0)
		{
			if(text[0] == '\\')
			{
				// lexer should've caught this
				assert(text.size() > 1);

				if(text[1] == '\\')
					ret.push_back(U'\\');
				else if(text[1] == '{')
					ret.push_back(U'{');
				else if(text[1] == '}')
					ret.push_back(U'}');
				else if(text[1] == '#')
					ret.push_back(U'#');
				else
					error(loc, "unrecognised escape sequence '\\{}'", text[1]);

				text.remove_prefix(2);
			}
			else
			{
				ret += unicode::consumeCodepointFromUtf8(text);
			}
		}

		return ret;
	}

	static int get_front_token_precedence(Lexer& lexer)
	{
		switch(lexer.peek())
		{
			case TT::LParen: return 1000;

			case TT::Asterisk:
			case TT::Slash: return 400;

			case TT::Plus:
			case TT::Minus: return 300;

			case TT::LAngle:
			case TT::RAngle:
			case TT::EqualEqual:
			case TT::LAngleEqual:
			case TT::RAngleEqual:
			case TT::ExclamationEqual: return 200;

			default: return -1;
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

	static bool is_assignment_op(TokenType tok)
	{
		return util::is_one_of(tok, TT::Equal);
	}

	static bool is_regular_binary_op(TokenType tok)
	{
		return util::is_one_of(tok, TT::Plus, TT::Minus, TT::Asterisk, TT::Slash);
	}

	static bool is_comparison_op(TokenType tok)
	{
		return util::is_one_of(tok, TT::LAngle, TT::RAngle, TT::LAngleEqual, TT::RAngleEqual, TT::EqualEqual,
		    TT::ExclamationEqual);
	}

	static interp::BinaryOp::Op convert_binop(TokenType tok)
	{
		switch(tok)
		{
			case TT::Plus: return interp::BinaryOp::Add;
			case TT::Minus: return interp::BinaryOp::Subtract;
			case TT::Asterisk: return interp::BinaryOp::Multiply;
			case TT::Slash: return interp::BinaryOp::Divide;
			default: assert(false && "unreachable!");
		}
	}

	static interp::ComparisonOp::Op convert_comparison_op(TokenType tok)
	{
		switch(tok)
		{
			case TT::LAngle: return interp::ComparisonOp::LT;
			case TT::RAngle: return interp::ComparisonOp::GT;
			case TT::LAngleEqual: return interp::ComparisonOp::LE;
			case TT::RAngleEqual: return interp::ComparisonOp::GE;
			case TT::EqualEqual: return interp::ComparisonOp::EQ;
			case TT::ExclamationEqual: return interp::ComparisonOp::NE;
			default: assert(false && "unreachable!");
		}
	}



	constexpr auto KW_LET = "let";
	constexpr auto KW_VAR = "var";
	constexpr auto KW_SCRIPT_BLOCK = "script";

	static std::unique_ptr<interp::Expr> parse_expr(Lexer& lexer);
	static std::unique_ptr<interp::Expr> parse_unary(Lexer& lexer);



	static interp::QualifiedId parse_qualified_id(Lexer& lexer)
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

	static std::unique_ptr<interp::FunctionCall> parse_function_call(Lexer& lexer, std::unique_ptr<interp::Expr> callee)
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
					arg.value = parse_expr(lexer);
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
				arg.value = parse_expr(lexer);
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

	static std::unique_ptr<interp::Expr> parse_postfix_unary(Lexer& lexer, std::unique_ptr<interp::Expr> lhs)
	{
		if(lexer.peek() == TT::LParen)
			return parse_function_call(lexer, std::move(lhs));

		error(lexer.peek().loc, "expected '(' or '[' after expression, found '{}'", lexer.peek().text);
	}


	static std::unique_ptr<interp::Expr> parse_rhs(Lexer& lexer, std::unique_ptr<interp::Expr> lhs, int prio)
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
				lhs = parse_postfix_unary(lexer, std::move(lhs));
				continue;
			}

			auto op_tok = lexer.next();
			auto rhs = parse_unary(lexer);
			int next = get_front_token_precedence(lexer);

			if(next > prec || is_right_associative(lexer))
				rhs = parse_rhs(lexer, std::move(rhs), prec + 1);

			if(is_assignment_op(op_tok))
			{
				error(lexer.peek().loc, "asdf");
				// auto newlhs = util::pool<AssignOp>(loc);

				// newlhs->left = dcast(Expr, lhs);
				// newlhs->right = rhs;
				// newlhs->op = op;

				// lhs = newlhs;
			}
			else if(is_comparison_op(op_tok))
			{
				if(auto cmp = dynamic_cast<interp::ComparisonOp*>(lhs.get()))
				{
					cmp->rest.push_back({ convert_comparison_op(op_tok), std::move(rhs) });
				}
				else
				{
					auto newlhs = std::make_unique<interp::ComparisonOp>();
					newlhs->first = std::move(lhs);
					newlhs->rest.push_back({ convert_comparison_op(op_tok), std::move(rhs) });

					lhs = std::move(newlhs);
				}
			}
			else if(is_regular_binary_op(op_tok))
			{
				auto tmp = std::make_unique<interp::BinaryOp>();
				tmp->lhs = std::move(lhs);
				tmp->rhs = std::move(rhs);
				tmp->op = convert_binop(op_tok);

				lhs = std::move(tmp);
			}
			else
			{
				error(lexer.peek().loc, "unknown operator token '{}'", lexer.peek().text);
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
					case U'\\': ret += U'\\'; break;
					case U'n': ret += U'\n'; break;
					case U't': ret += U'\t'; break;
					case U'b': ret += U'\b'; break;
					case U'x':
						assert(bs.size() >= 2);
						assert(is_hex(bs[0]) && is_hex(bs[1]));
						ret += 16u * convert_hex(bs[0]) + convert_hex(bs[1]);
						break;
					case U'u':
						assert(bs.size() >= 4);
						assert(is_hex(bs[0]) && is_hex(bs[1]) && is_hex(bs[2]) && is_hex(bs[3]));
						ret += 16u * 16u * 16u * convert_hex(bs[0]) //
						     + 16u * 16u * convert_hex(bs[1])       //
						     + 16u * convert_hex(bs[2])             //
						     + convert_hex(bs[3]);
						break;
					case U'U':
						assert(bs.size() >= 8);
						assert(is_hex(bs[0]) && is_hex(bs[1]) && is_hex(bs[2]) && is_hex(bs[3]));
						assert(is_hex(bs[4]) && is_hex(bs[5]) && is_hex(bs[6]) && is_hex(bs[7]));

						ret += 16u * 16u * 16u * 16u * 16u * 16u * 16u * convert_hex(bs[0]) //
						     + 16u * 16u * 16u * 16u * 16u * 16u * convert_hex(bs[1])       //
						     + 16u * 16u * 16u * 16u * 16u * convert_hex(bs[2])             //
						     + 16u * 16u * 16u * 16u * convert_hex(bs[3])                   //
						     + 16u * 16u * 16u * convert_hex(bs[4])                         //
						     + 16u * 16u * convert_hex(bs[5])                               //
						     + 16u * convert_hex(bs[6])                                     //
						     + convert_hex(bs[7]);
						break;
					default: sap::error(loc, "invalid escape sequence starting with '{}'", cp); break;
				}
			}
			else
			{
				ret += cp;
			}
		}

		return ret;
	}

	static std::unique_ptr<interp::Expr> parse_primary(Lexer& lexer)
	{
		if(lexer.peek() == TT::Identifier)
		{
			auto qid = parse_qualified_id(lexer);
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

	static std::unique_ptr<interp::Expr> parse_unary(Lexer& lexer)
	{
		return parse_primary(lexer);
	}

	static std::unique_ptr<interp::Expr> parse_expr(Lexer& lexer)
	{
		auto lm = LexerModer(lexer, Lexer::Mode::Script);
		auto lhs = parse_unary(lexer);

		return parse_rhs(lexer, std::move(lhs), 0);
	}

	static const interp::Type* parse_type(Lexer& lexer)
	{
		return nullptr;
	}

	static std::unique_ptr<interp::VariableDefn> parse_var_defn(Lexer& lexer)
	{
		auto kw = lexer.next();
		assert(kw == TT::Identifier);
		assert(kw.text == KW_LET || kw.text == KW_VAR);

		// expect a name
		auto maybe_name = lexer.match(TT::Identifier);
		if(not maybe_name.has_value())
			error(lexer.location(), "expected identifier after '{}'", kw.text);

		std::unique_ptr<interp::Expr> initialiser {};
		std::optional<const interp::Type*> explicit_type {};

		if(lexer.match(TT::Colon))
			explicit_type = parse_type(lexer);

		if(lexer.match(TT::Equal))
			initialiser = parse_expr(lexer);

		return std::make_unique<interp::VariableDefn>(maybe_name->str(), /* is_mutable: */ kw.text == KW_VAR,
		    std::move(initialiser), std::move(explicit_type));
	}


	static std::unique_ptr<interp::Stmt> parse_stmt(Lexer& lexer)
	{
		auto lm = LexerModer(lexer, Lexer::Mode::Script);

		auto tok = lexer.peek();
		std::unique_ptr<interp::Stmt> stmt {};

		switch(tok.type)
		{
			case TT::Identifier:
				if(tok.text == KW_LET || tok.text == KW_VAR)
					stmt = parse_var_defn(lexer);
				else
					stmt = parse_expr(lexer);
				break;

			default: stmt = parse_expr(lexer); break;
		}

		if(not stmt)
			error(tok.loc, "invalid start of statement");

		else if(not lexer.expect(TT::Semicolon))
			error(lexer.peek().loc, "expected ';' after statement, found '{}'", lexer.peek().text);

		return stmt;
	}


	static std::unique_ptr<ScriptBlock> parse_script_block(Lexer& lexer)
	{
		auto lm = LexerModer(lexer, Lexer::Mode::Script);
		if(auto x = lexer.match(TT::Identifier); not x || x->text != KW_SCRIPT_BLOCK)
			error(Location {}, "?????");

		auto openbrace = lexer.next();
		if(openbrace != TT::LBrace)
			error(openbrace.loc, "expected '{' after {}", KW_SCRIPT_BLOCK);

		auto block = std::make_unique<ScriptBlock>();
		while(lexer.peek() != TT::RBrace)
			block->statements.push_back(parse_stmt(lexer));

		if(lexer.eof())
			error(openbrace.loc, "unexpected end of file; unclosed '{' here");

		else if(not lexer.match(TT::RBrace))
			error(openbrace.loc, "expected '}' to match '{' here");

		return block;
	}

	static std::unique_ptr<ScriptCall> parse_inline_script(Lexer& lexer)
	{
		auto lm = LexerModer(lexer, Lexer::Mode::Script);

		auto qid = parse_qualified_id(lexer);
		auto lhs = std::make_unique<interp::Ident>();
		lhs->name = std::move(qid);

		auto open_paren = lexer.peek();
		if(open_paren != TT::LParen)
			error(open_paren.loc, "expected '(' after qualified-id in inline script call");

		auto sc = std::make_unique<ScriptCall>();
		sc->call = parse_function_call(lexer, std::move(lhs));

		return sc;
	}

	static std::unique_ptr<ScriptObject> parse_script_object(Lexer& lexer)
	{
		auto next = lexer.peekWithMode(Lexer::Mode::Script);
		if(next == TT::Identifier)
		{
			if(next.text == KW_SCRIPT_BLOCK)
				return parse_script_block(lexer);
			else
				return parse_inline_script(lexer);
		}
		else
		{
			error(next.loc, "unexpected token '{}' after '\\'", next.text);
		}
	}


	static std::unique_ptr<Paragraph> parse_paragraph(Lexer& lexer)
	{
		auto para = std::make_unique<Paragraph>();

		while(true)
		{
			auto tok = lexer.next();

			if(tok == TT::Text)
			{
				auto word = std::make_shared<Text>(escape_word_text(tok.loc, tok.text));
				para->addObject(std::move(word));
			}
			else if(tok == TT::ParagraphBreak || tok == TT::EndOfFile)
			{
				break;
			}
			else if(tok == TT::Backslash)
			{
				auto obj = parse_script_object(lexer);
				para->addObject(std::move(obj));
			}
			else
			{
				error(tok.loc, "unexpected token '{}' in paragraph", tok.text);
			}
		}

		return para;
	}


	static std::unique_ptr<DocumentObject> parse_top_level(Lexer& lexer)
	{
		while(true)
		{
			if(auto tok = lexer.peek(); tok == TT::Text)
			{
				return parse_paragraph(lexer);
			}
			else if(tok == TT::ParagraphBreak)
			{
				lexer.next();
				continue;
			}
			else if(tok == TT::Backslash)
			{
				lexer.next();
				return parse_script_object(lexer);
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

			document.addObject(parse_top_level(lexer));
		}

		return document;
	}
}
