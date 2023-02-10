// parser.cpp
// Copyright (c) 2022, zhiayang
// SPDX-License-Identifier: Apache-2.0

#include "util.h"     // for consumeCodepointFromUtf8, is_one_of
#include "location.h" // for error, Location

#include "sap/units.h"
#include "sap/frontend.h" // for Token, Lexer, TokenType, Lexer::Mode

#include "tree/base.h"
#include "tree/document.h"
#include "tree/paragraph.h"
#include "tree/container.h"

#include "interp/ast.h"      // for Expr, FunctionDecl::Param, FunctionCall...
#include "interp/type.h"     // for Type, interp, ArrayType, FunctionType
#include "interp/basedefs.h" // for DocumentObject, InlineObject
#include "interp/parser_type.h"

namespace sap::frontend
{
	constexpr auto KW_NULL = "null";

	constexpr auto KW_PARA_BLOCK = "p";
	constexpr auto KW_SCRIPT_BLOCK = "script";
	constexpr auto KW_START_DOCUMENT = "start_document";

	constexpr auto KW_PHASE_LAYOUT = "layout";
	constexpr auto KW_PHASE_POSITION = "position";
	constexpr auto KW_PHASE_RENDER = "render";
	constexpr auto KW_PHASE_POST = "post";

	using TT = TokenType;
	using namespace sap::tree;

	template <typename T>
	static void must_expect(Lexer& lexer, T&& x)
	{
		if(not lexer.expect(static_cast<T&&>(x)))
			parse_error(lexer.location(), "?????");
	}


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
					parse_error(loc, "unrecognised escape sequence '\\{}'", text[1]);

				text.remove_prefix(2);
			}
			else
			{
				ret += unicode::consumeCodepointFromUtf8(text);
			}
		}

		return ret;
	}

	static std::pair<std::u32string, size_t> unescape_string_part(const Location& loc, zst::byte_span orig)
	{
		std::u32string ret {};

		auto bs = orig;
		while(bs.size() > 0)
		{
			auto copy = bs;
			auto cp = unicode::consumeCodepointFromUtf8(copy);

			if(cp == U'\\')
			{
				bs = copy;

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
					case U'{': ret += U'{'; break;
					case U'}': ret += U'}'; break;
					case U'"': ret += U'"'; break;
					case U'\'': ret += U'\''; break;
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
					default: //
						parse_error(loc, "invalid escape sequence starting with '{}'", cp);
						break;
				}
			}
			else
			{
				break;
			}
		}

		return { ret, orig.size() - bs.size() };
	}



	static std::u32string unescape_string(const Location& loc, zst::str_view sv)
	{
		std::u32string ret {};
		ret.reserve(sv.size());

		auto bs = sv.bytes();
		while(not bs.empty())
		{
			auto [e, n] = unescape_string_part(loc, bs);
			if(n > 0)
			{
				bs.remove_prefix(n);
				ret += e;
			}
			else
			{
				ret += unicode::consumeCodepointFromUtf8(bs);
			}
		}

		return ret;
	}


	static ProcessingPhase parse_processing_phase(Lexer& lexer)
	{
		auto ident = lexer.match(TT::Identifier);
		if(not ident.has_value())
			parse_error(lexer.location(), "expected identifier after '@'");

		if(ident->text == KW_PHASE_LAYOUT)
			return ProcessingPhase::Layout;
		else if(ident->text == KW_PHASE_POSITION)
			return ProcessingPhase::Position;
		else if(ident->text == KW_PHASE_POST)
			return ProcessingPhase::PostLayout;
		else if(ident->text == KW_PHASE_RENDER)
			return ProcessingPhase::Render;

		parse_error(lexer.location(), "invalid phase '{}'", ident->text);
	}




	static int get_front_token_precedence(Lexer& lexer)
	{
		switch(lexer.peek())
		{
			case TT::LParen:
			case TT::LSquare:
			case TT::Question:
			case TT::Ellipsis:
			case TT::Exclamation: return 1000000;

			case TT::Period: return 900;
			case TT::QuestionPeriod: return 900;

			case TT::Asterisk:
			case TT::Percent:
			case TT::Slash: return 400;

			case TT::Plus:
			case TT::Minus: return 300;

			case TT::LAngle:
			case TT::RAngle:
			case TT::EqualEqual:
			case TT::LAngleEqual:
			case TT::RAngleEqual:
			case TT::ExclamationEqual: return 200;

			case TT::QuestionQuestion: return 100;

			default: return -1;
		}
	}

	static bool is_right_associative(Lexer& lexer)
	{
		return false;
	}

	static bool is_assignment_op(TokenType tok)
	{
		return util::is_one_of(tok, TT::Equal, TT::PlusEqual, TT::MinusEqual, TT::AsteriskEqual, TT::SlashEqual,
			TT::PercentEqual);
	}

	static bool is_regular_binary_op(TokenType tok)
	{
		return util::is_one_of(tok, TT::Plus, TT::Minus, TT::Asterisk, TT::Slash, TT::Percent);
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
			case TT::Percent: return interp::BinaryOp::Modulo;
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

	static interp::AssignOp::Op convert_assignment_op(TokenType tok)
	{
		switch(tok)
		{
			case TT::Equal: return interp::AssignOp::None;
			case TT::PlusEqual: return interp::AssignOp::Add;
			case TT::MinusEqual: return interp::AssignOp::Subtract;
			case TT::AsteriskEqual: return interp::AssignOp::Multiply;
			case TT::SlashEqual: return interp::AssignOp::Divide;
			case TT::PercentEqual: return interp::AssignOp::Modulo;
			default: assert(false && "unreachable!");
		}
	}


	static std::unique_ptr<interp::Stmt> parse_stmt(Lexer& lexer, bool is_top_level);
	static std::unique_ptr<interp::Expr> parse_expr(Lexer& lexer);
	static std::unique_ptr<interp::Expr> parse_unary(Lexer& lexer);
	static std::unique_ptr<interp::Expr> parse_primary(Lexer& lexer);


	static std::pair<interp::QualifiedId, uint32_t> parse_qualified_id(Lexer& lexer)
	{
		uint32_t len = 0;

		interp::QualifiedId qid {};
		while(not lexer.eof())
		{
			if(lexer.expect(TT::ColonColon))
			{
				len += 2;
				qid.top_level = true;
			}

			auto tok = lexer.next();
			if(tok != TT::Identifier)
				parse_error(tok.loc, "expected identifier, got '{}'", tok.text);

			len += checked_cast<uint32_t>(tok.loc.length);
			if(lexer.expect(TT::ColonColon))
			{
				len += 2;
				qid.parents.push_back(tok.text.str());
			}
			else
			{
				qid.name = tok.text.str();
				break;
			}
		}

		return { std::move(qid), len };
	}

	static std::unique_ptr<interp::FunctionCall> parse_function_call(Lexer& lexer, std::unique_ptr<interp::Expr> callee)
	{
		if(not lexer.expect(TT::LParen))
			parse_error(lexer.peek().loc, "expected '(' to begin function call");

		auto call = std::make_unique<interp::FunctionCall>(callee->loc());
		call->callee = std::move(callee);

		// parse arguments.
		for(bool first = true; not lexer.expect(TT::RParen); first = false)
		{
			if(not first && not lexer.expect(TT::Comma))
				parse_error(lexer.peek().loc, "expected ',' or ')' in call argument list, got '{}'", lexer.peek().text);

			bool is_arg_named = false;

			interp::FunctionCall::Arg arg {};
			if(lexer.peek() == TT::Identifier)
			{
				auto save = lexer.save();
				auto name = lexer.next();

				if(lexer.expect(TT::Colon))
				{
					arg.value = parse_expr(lexer);
					arg.name = name.text.str();

					is_arg_named = true;
				}
				else
				{
					lexer.rewind(save);
				}
			}

			if(not is_arg_named)
			{
				arg.name = {};
				arg.value = parse_expr(lexer);
			}

			call->arguments.push_back(std::move(arg));
		}

		// TODO: (potentially multiple) trailing blocks
		return call;
	}

	static std::unique_ptr<interp::Expr> parse_subscript(Lexer& lexer, std::unique_ptr<interp::Expr> lhs)
	{
		auto loc = lexer.peek().loc;

		must_expect(lexer, TT::LSquare);
		auto subscript = parse_expr(lexer);

		if(not lexer.expect(TT::RSquare))
			parse_error(lexer.location(), "expected ']'");

		auto ret = std::make_unique<interp::SubscriptOp>(loc);
		ret->array = std::move(lhs);
		ret->index = std::move(subscript);
		return ret;
	}

	static std::unique_ptr<interp::Expr> parse_postfix_unary(Lexer& lexer)
	{
		auto lhs = parse_primary(lexer);

		if(lexer.peek() == TT::LParen)
		{
			return parse_function_call(lexer, std::move(lhs));
		}
		else if(lexer.peek() == TT::LSquare)
		{
			return parse_subscript(lexer, std::move(lhs));
		}
		else if(auto t = lexer.match(TT::Question); t.has_value())
		{
			auto ret = std::make_unique<interp::OptionalCheckOp>(t->loc);
			ret->expr = std::move(lhs);
			return ret;
		}
		else if(auto t = lexer.match(TT::Exclamation); t.has_value())
		{
			auto ret = std::make_unique<interp::DereferenceOp>(t->loc);
			ret->expr = std::move(lhs);
			return ret;
		}
		else if(auto t = lexer.match(TT::Ellipsis); t.has_value())
		{
			return std::make_unique<interp::ArraySpreadOp>(t->loc, std::move(lhs));
		}
		else
		{
			return lhs;
		}
	}

	static std::unique_ptr<interp::FunctionCall> parse_ufcs(Lexer& lexer,
		std::unique_ptr<interp::Expr> first_arg,
		const std::string& method_name,
		bool is_optional)
	{
		auto method = std::make_unique<interp::Ident>(lexer.location());
		method->name.top_level = false;
		method->name.name = method_name;

		auto call = parse_function_call(lexer, std::move(method));
		call->rewritten_ufcs = true;
		call->is_optional_ufcs = is_optional;

		call->arguments.insert(call->arguments.begin(),
			interp::FunctionCall::Arg {
				.name = std::nullopt,
				.value = std::move(first_arg),
			});

		return call;
	}


	static std::unique_ptr<interp::Expr> parse_rhs(Lexer& lexer, std::unique_ptr<interp::Expr> lhs, int prio)
	{
		if(lexer.eof())
			return lhs;

		while(true)
		{
			int prec = get_front_token_precedence(lexer);
			if(prec < prio && not is_right_associative(lexer))
				return lhs;

			// if we see an assignment, bail without consuming the operator token.
			if(is_assignment_op(lexer.peek()))
				return lhs;

			auto op_tok = lexer.next();

			// special handling for dot op -- check here.
			if(op_tok == TT::Period || op_tok == TT::QuestionPeriod)
			{
				// TODO: maybe support .0, .1 syntax for tuples
				auto field_name = lexer.match(TT::Identifier);
				if(not field_name.has_value())
					parse_error(lexer.location(), "expected field name after '.'");

				auto newlhs = std::make_unique<interp::DotOp>(op_tok.loc);
				newlhs->is_optional = (op_tok == TT::QuestionPeriod);
				newlhs->rhs = field_name->text.str();
				newlhs->lhs = std::move(lhs);

				// special case method calls: UFCS, rewrite into a free function call.
				if(lexer.peek() == TT::LParen)
				{
					lhs = parse_ufcs(lexer, std::move(newlhs->lhs), field_name->text.str(),
						/* is_optional */ op_tok == TT::QuestionPeriod);
				}
				else
				{
					lhs = std::move(newlhs);
				}
				continue;
			}

			auto rhs = parse_unary(lexer);
			int next = get_front_token_precedence(lexer);

			if(next > prec || is_right_associative(lexer))
				rhs = parse_rhs(lexer, std::move(rhs), prec + 1);

			if(is_comparison_op(op_tok))
			{
				if(auto cmp = dynamic_cast<interp::ComparisonOp*>(lhs.get()))
				{
					cmp->rest.push_back({ convert_comparison_op(op_tok), std::move(rhs) });
				}
				else
				{
					auto newlhs = std::make_unique<interp::ComparisonOp>(op_tok.loc);
					newlhs->first = std::move(lhs);
					newlhs->rest.push_back({ convert_comparison_op(op_tok), std::move(rhs) });

					lhs = std::move(newlhs);
				}
			}
			else if(is_regular_binary_op(op_tok))
			{
				auto tmp = std::make_unique<interp::BinaryOp>(op_tok.loc);
				tmp->lhs = std::move(lhs);
				tmp->rhs = std::move(rhs);
				tmp->op = convert_binop(op_tok);

				lhs = std::move(tmp);
			}
			else if(op_tok == TT::QuestionQuestion)
			{
				auto tmp = std::make_unique<interp::NullCoalesceOp>(op_tok.loc);
				tmp->lhs = std::move(lhs);
				tmp->rhs = std::move(rhs);

				lhs = std::move(tmp);
			}
			else
			{
				parse_error(lexer.peek().loc, "unknown operator token '{}'", lexer.peek().text);
			}
		}
	}

	static std::unique_ptr<interp::StructLit> parse_struct_literal(Lexer& lexer, interp::QualifiedId id)
	{
		if(not lexer.expect(TT::LBrace))
			parse_error(lexer.location(), "expected '{' for struct literal");

		auto ret = std::make_unique<interp::StructLit>(lexer.location());
		ret->struct_name = std::move(id);

		while(not lexer.expect(TT::RBrace))
		{
			if(not lexer.expect(TT::Period))
				parse_error(lexer.location(), "expected '.' to begin designated struct initialiser, found '{}'");

			auto field_name = lexer.match(TT::Identifier);
			if(not field_name.has_value())
				parse_error(lexer.location(), "expected identifier after '.' for field name");

			if(not lexer.expect(TT::Equal))
				parse_error(lexer.location(), "expected '=' after field name");

			auto value = parse_expr(lexer);
			ret->field_inits.push_back(interp::StructLit::Arg {
				.name = field_name->text.str(),
				.value = std::move(value),
			});

			if(not lexer.expect(TT::Comma) && lexer.peek() != TT::RBrace)
				parse_error(lexer.location(), "expected ',' in struct initialiser list");
		}

		return ret;
	}

	static std::unique_ptr<interp::FStringExpr> parse_fstring(Lexer& main_lexer, const Token& tok)
	{
		auto fstr = tok.text.bytes();

		std::u32string string {};
		string.reserve(fstr.size());

		std::vector<std::variant<std::u32string, std::unique_ptr<interp::Expr>>> fstring_parts;

		while(fstr.size() > 0)
		{
			if(fstr.starts_with("\\{"_bs))
			{
				fstr.remove_prefix(2);
				string += U"{";
			}
			else if(fstr.starts_with("{"_bs))
			{
				fstr.remove_prefix(1);

				fstring_parts.push_back(std::move(string));
				string.clear();

				auto tmp_lexer = Lexer(tok.loc.filename, fstr.chars());
				tmp_lexer.setLocation(tok.loc);
				tmp_lexer.pushMode(Lexer::Mode::Script);

				fstring_parts.push_back(parse_expr(tmp_lexer));

				fstr.remove_prefix((size_t) (tmp_lexer.stream().bytes().data() - fstr.data()));
				if(not fstr.starts_with('}'))
					parse_error(tmp_lexer.location(), "expected '}' to end f-string expression");

				fstr.remove_prefix(1);
			}
			else
			{
				auto [e, n] = unescape_string_part(tok.loc, fstr);
				if(n > 0)
				{
					fstr.remove_prefix(n);
					string += e;
				}
				else
				{
					string += unicode::consumeCodepointFromUtf8(fstr);
				}
			}
		}

		if(not string.empty())
			fstring_parts.push_back(std::move(string));

		auto ret = std::make_unique<interp::FStringExpr>(tok.loc);
		ret->parts = std::move(fstring_parts);

		return ret;
	}

	static PType parse_type(Lexer& lexer);

	static std::unique_ptr<interp::Expr> parse_primary(Lexer& lexer)
	{
		if(lexer.peek() == TT::Identifier || lexer.peek() == TT::ColonColon)
		{
			if(lexer.peek().text == KW_NULL)
				return std::make_unique<interp::NullLit>(lexer.next().loc);

			auto loc = lexer.peek().loc;
			auto [qid, len] = parse_qualified_id(lexer);

			loc.length = len;
			auto ident = std::make_unique<interp::Ident>(loc);
			ident->name = std::move(qid);

			// check for struct literal
			if(lexer.peek() == TT::LBrace)
			{
				return parse_struct_literal(lexer, std::move(ident->name));
			}
			else
			{
				return ident;
			}
		}
		else if(lexer.peek() == TT::LBrace)
		{
			// '{' starts a struct literal with no name
			return parse_struct_literal(lexer, {});
		}
		else if(auto num = lexer.match(TT::Number); num)
		{
			// check if we have a unit later.
			if(auto unit = lexer.match(TT::Identifier); unit.has_value())
			{
				auto maybe_unit = DynLength::stringToUnit(unit->text);
				if(not maybe_unit.has_value())
					parse_error(lexer.location(), "unknown unit '{}' following number literal", unit->text);

				auto ret = std::make_unique<interp::LengthExpr>(num->loc);
				ret->length = DynLength(std::stod(num->text.str()), *maybe_unit);

				return ret;
			}
			else
			{
				auto ret = std::make_unique<interp::NumberLit>(num->loc);
				if(num->text.find('.') != (size_t) -1)
				{
					ret->is_floating = true;
					ret->float_value = std::stod(num->text.str());
				}
				else
				{
					ret->is_floating = false;
					ret->int_value = std::stoll(num->text.str());
				}
				return ret;
			}
		}
		else if(auto str = lexer.match(TT::String); str)
		{
			auto ret = std::make_unique<interp::StringLit>(str->loc);
			ret->string = unescape_string(str->loc, str->text);

			return ret;
		}
		else if(auto fstr = lexer.match(TT::FString); fstr)
		{
			return parse_fstring(lexer, *fstr);
		}
		else if(auto bool_true = lexer.match(TT::KW_True); bool_true)
		{
			return std::make_unique<interp::BooleanLit>(bool_true->loc, true);
		}
		else if(auto bool_false = lexer.match(TT::KW_False); bool_false)
		{
			return std::make_unique<interp::BooleanLit>(bool_true->loc, false);
		}
		else if(lexer.expect(TT::LParen))
		{
			auto inside = parse_expr(lexer);
			if(not lexer.expect(TT::RParen))
				parse_error(lexer.location(), "expected closing ')'");

			return inside;
		}
		else if(lexer.expect(TT::LSquare))
		{
			auto array = std::make_unique<interp::ArrayLit>(lexer.location());
			if(lexer.expect(TT::Colon))
				array->elem_type = parse_type(lexer);

			while(lexer.peek() != TT::RSquare)
			{
				array->elements.push_back(parse_expr(lexer));
				if(lexer.expect(TT::Comma))
					continue;
				else if(lexer.peek() == TT::RSquare)
					break;
				else
					parse_error(lexer.location(), "expected ']' or ',' in array literal, found '{}'",
						lexer.peek().text);
			}

			must_expect(lexer, TT::RSquare);
			return array;
		}
		else
		{
			parse_error(lexer.location(), "invalid start of expression '{}'", lexer.peek().text);
		}
	}

	static std::unique_ptr<interp::Expr> parse_unary(Lexer& lexer)
	{
		if(auto t = lexer.match(TT::Ampersand); t.has_value())
		{
			bool is_mutable = lexer.expect(TT::KW_Mut);

			auto ret = std::make_unique<interp::AddressOfOp>(t->loc);
			ret->expr = parse_unary(lexer);
			ret->is_mutable = is_mutable;
			return ret;
		}
		else if(auto t = lexer.match(TT::Asterisk); t.has_value())
		{
			auto ret = std::make_unique<interp::MoveExpr>(t->loc);
			ret->expr = parse_unary(lexer);
			return ret;
		}

		return parse_postfix_unary(lexer);
	}

	static std::unique_ptr<interp::Expr> parse_expr(Lexer& lexer)
	{
		auto lm = LexerModer(lexer, Lexer::Mode::Script);
		auto lhs = parse_unary(lexer);

		return parse_rhs(lexer, std::move(lhs), 0);
	}

	static std::unique_ptr<interp::AssignOp> parse_assignment(std::unique_ptr<interp::Expr> lhs, Lexer& lexer)
	{
		auto op = lexer.next();
		assert(is_assignment_op(op));

		auto ret = std::make_unique<interp::AssignOp>(op.loc);
		ret->op = convert_assignment_op(op);
		ret->lhs = std::move(lhs);
		ret->rhs = parse_expr(lexer);

		return ret;
	}


	static PType parse_type(Lexer& lexer)
	{
		if(lexer.eof())
			parse_error(lexer.location(), "unexpected end of file");

		using namespace interp;

		auto fst = lexer.peek();
		if(fst == TT::Identifier)
		{
			return PType::named(parse_qualified_id(lexer).first);
		}
		else if(fst == TT::ColonColon)
		{
			return PType::named(parse_qualified_id(lexer).first);
		}
		else if(fst == TT::Ampersand)
		{
			lexer.next();

			bool is_mutable = false;
			if(lexer.expect(TT::KW_Mut))
				is_mutable = true;

			return PType::pointer(parse_type(lexer), is_mutable);
		}
		else if(fst == TT::Question)
		{
			lexer.next();
			return PType::optional(parse_type(lexer));
		}
		else if(fst == TT::QuestionQuestion)
		{
			lexer.next();
			return PType::optional(PType::optional(parse_type(lexer)));
		}
		else if(fst == TT::LSquare)
		{
			lexer.next();

			auto elm = parse_type(lexer);
			bool is_variadic = lexer.expect(TT::Ellipsis);

			if(not lexer.expect(TT::RSquare))
				parse_error(lexer.location(), "expected ']' in array type");

			return PType::array(std::move(elm), is_variadic);
		}
		else if(fst == TT::LParen)
		{
			lexer.next();

			std::vector<PType> types {};
			for(bool first = true; not lexer.expect(TT::RParen); first = false)
			{
				if(not first && not lexer.expect(TT::Comma))
					parse_error(lexer.location(), "expected ',' or ')' in type specifier");

				types.push_back(parse_type(lexer));
			}

			// TODO: support tuples
			if(not lexer.expect(TT::RArrow))
				parse_error(lexer.location(), "TODO: tuples not implemented");

			auto return_type = parse_type(lexer);
			return PType::function(std::move(types), return_type);
		}

		parse_error(lexer.location(), "invalid start of type specifier '{}'", lexer.peek().text);
	}

	static std::unique_ptr<interp::VariableDefn> parse_var_defn(Lexer& lexer, bool is_top_level)
	{
		auto kw = lexer.next();
		assert(kw == TT::KW_Let || kw == TT::KW_Var);

		// expect a name
		auto maybe_name = lexer.match(TT::Identifier);
		if(not maybe_name.has_value())
			parse_error(lexer.location(), "expected identifier after '{}'", kw.text);

		std::unique_ptr<interp::Expr> initialiser {};
		std::optional<PType> explicit_type {};

		if(lexer.match(TT::Colon))
			explicit_type = parse_type(lexer);

		if(lexer.match(TT::Equal))
			initialiser = parse_expr(lexer);

		return std::make_unique<interp::VariableDefn>(kw.loc, maybe_name->str(), /* is_mutable: */ kw == TT::KW_Var,
			/* is_global: */ is_top_level, std::move(initialiser), std::move(explicit_type));
	}

	static interp::FunctionDecl::Param parse_param(Lexer& lexer)
	{
		auto name = lexer.match(TT::Identifier);
		if(not name.has_value())
			parse_error(lexer.location(), "expected parameter name");

		if(not lexer.expect(TT::Colon))
			parse_error(lexer.location(), "expected ':' after parameter name");

		auto type = parse_type(lexer);
		std::unique_ptr<interp::Expr> default_value = nullptr;

		if(lexer.expect(TT::Equal))
			default_value = parse_expr(lexer);

		return interp::FunctionDecl::Param {
			.name = name->text.str(),
			.type = type,
			.default_value = std::move(default_value),
			.loc = name->loc,
		};
	}

	static std::unique_ptr<interp::Block> parse_block_or_stmt(Lexer& lexer, bool is_top_level, bool mandatory_braces)
	{
		auto block = std::make_unique<interp::Block>(lexer.location());
		if(lexer.expect(TT::LBrace))
		{
			while(not lexer.expect(TT::RBrace))
			{
				if(lexer.expect(TT::Semicolon))
					continue;

				block->body.push_back(parse_stmt(lexer, is_top_level));
			}
		}
		else
		{
			if(mandatory_braces)
				parse_error(lexer.location(), "expected '{' to begin a block");

			block->body.push_back(parse_stmt(lexer, is_top_level));
		}

		return block;
	}

	static std::unique_ptr<interp::FunctionDefn> parse_function_defn(Lexer& lexer)
	{
		using namespace interp;
		auto loc = lexer.location();
		must_expect(lexer, TT::KW_Fn);

		std::string name;
		if(auto name_tok = lexer.match(TT::Identifier); not name_tok.has_value())
			parse_error(lexer.location(), "expected identifier after 'fn'");
		else
			name = name_tok->text.str();

		if(not lexer.expect(TT::LParen))
			parse_error(lexer.location(), "expected '(' in function definition");

		std::vector<FunctionDecl::Param> params {};
		for(bool first = true; not lexer.expect(TT::RParen); first = false)
		{
			if(not first && not lexer.expect(TT::Comma))
				parse_error(lexer.location(), "expected ',' or ')' in function parameter list");

			params.push_back(parse_param(lexer));
		}

		auto return_type = PType::named(TYPE_VOID);
		if(lexer.expect(TT::RArrow))
			return_type = parse_type(lexer);

		auto defn = std::make_unique<interp::FunctionDefn>(loc, name, std::move(params), return_type);
		if(lexer.peek() != TT::LBrace)
			parse_error(lexer.location(), "expected '{' to begin function body");

		defn->body = parse_block_or_stmt(lexer, /* is_top_level: */ false, /* braces: */ true);
		return defn;
	}

	static std::unique_ptr<interp::StructDefn> parse_struct_defn(Lexer& lexer)
	{
		auto loc = lexer.location();
		must_expect(lexer, TT::KW_Struct);

		std::string name;
		if(auto name_tok = lexer.match(TT::Identifier); not name_tok.has_value())
			parse_error(lexer.location(), "expected identifier after 'struct'");
		else
			name = name_tok->text.str();


		if(not lexer.expect(TT::LBrace))
			parse_error(lexer.location(), "expected '{' in struct body");

		std::vector<interp::StructDefn::Field> fields {};
		while(not lexer.expect(TT::RBrace))
		{
			auto field_name = lexer.match(TT::Identifier);
			if(not field_name.has_value())
				parse_error(lexer.location(), "expected field name in struct");

			if(not lexer.expect(TT::Colon))
				parse_error(lexer.location(), "expected ':' after field name");

			auto field_type = parse_type(lexer);
			std::unique_ptr<interp::Expr> field_initialiser {};

			if(lexer.expect(TT::Equal))
				field_initialiser = parse_expr(lexer);

			fields.push_back(interp::StructDefn::Field {
				.name = field_name->text.str(),
				.type = std::move(field_type),
				.initialiser = std::move(field_initialiser),
			});

			if(not lexer.match(TT::Semicolon) && lexer.peek() != TT::RBrace)
				parse_error(lexer.location(), "expected ';' or '}' in struct body");
		}

		return std::make_unique<interp::StructDefn>(loc, name, std::move(fields));
	}

	static std::unique_ptr<interp::EnumDefn> parse_enum_defn(Lexer& lexer)
	{
		auto loc = lexer.location();
		must_expect(lexer, TT::KW_Enum);

		std::string name;
		if(auto name_tok = lexer.match(TT::Identifier); not name_tok.has_value())
			parse_error(lexer.location(), "expected identifier after 'enum'");
		else
			name = name_tok->text.str();

		const auto pt_int = PType::named(TYPE_INT);
		auto enum_type = pt_int;

		if(lexer.expect(TT::Colon))
			enum_type = parse_type(lexer);

		if(not lexer.expect(TT::LBrace))
			parse_error(lexer.location(), "expected '{' for enum body");

		std::vector<interp::EnumDefn::EnumeratorDefn> enumerators {};
		while(not lexer.expect(TT::RBrace))
		{
			auto enum_name = lexer.match(TT::Identifier);
			if(not enum_name.has_value())
				parse_error(lexer.location(), "expected enumerator name");

			std::unique_ptr<interp::Expr> enum_value {};

			if(lexer.expect(TT::Equal))
				enum_value = parse_expr(lexer);

			enumerators.push_back(interp::EnumDefn::EnumeratorDefn(enum_name->loc, enum_name->text.str(),
				std::move(enum_value)));

			if(not lexer.match(TT::Semicolon))
				parse_error(lexer.location(), "expected ';' after enumerator");
		}

		return std::make_unique<interp::EnumDefn>(std::move(loc), std::move(name), std::move(enum_type),
			std::move(enumerators));
	}





	static std::unique_ptr<interp::IfStmt> parse_if_stmt(Lexer& lexer)
	{
		auto loc = lexer.location();
		must_expect(lexer, TT::KW_If);

		if(not lexer.expect(TT::LParen))
			parse_error(lexer.location(), "expected '(' after 'if'");

		auto if_stmt = std::make_unique<interp::IfStmt>(loc);

		if_stmt->if_cond = parse_expr(lexer);
		if(not lexer.expect(TT::RParen))
			parse_error(lexer.location(), "expected ')' after condition");

		if_stmt->if_body = parse_block_or_stmt(lexer, /* is_top_level: */ false, /* braces: */ false);

		if(lexer.expect(TT::KW_Else))
			if_stmt->else_body = parse_block_or_stmt(lexer, /* is_top_level: */ false, /* braces: */ false);

		return if_stmt;
	}

	static std::unique_ptr<interp::WhileLoop> parse_while_loop(Lexer& lexer)
	{
		auto loc = lexer.location();
		must_expect(lexer, TT::KW_While);

		if(not lexer.expect(TT::LParen))
			parse_error(lexer.location(), "expected '(' after 'while'");

		auto loop = std::make_unique<interp::WhileLoop>(loc);

		loop->condition = parse_expr(lexer);
		if(not lexer.expect(TT::RParen))
			parse_error(lexer.location(), "expected ')' after condition");

		loop->body = parse_block_or_stmt(lexer, /* is_top_level: */ false, /* braces: */ false);
		return loop;
	}


	static std::unique_ptr<interp::ReturnStmt> parse_return_stmt(Lexer& lexer)
	{
		auto loc = lexer.location();
		must_expect(lexer, TT::KW_Return);

		auto ret = std::make_unique<interp::ReturnStmt>(loc);
		if(lexer.peek() == TT::Semicolon)
			return ret;

		ret->expr = parse_expr(lexer);
		return ret;
	}

	static std::unique_ptr<interp::ImportStmt> parse_import_stmt(Lexer& lexer)
	{
		auto loc = lexer.location();
		must_expect(lexer, TT::KW_Import);

		auto maybe_str = lexer.match(TT::String);

		if(not maybe_str.has_value())
			parse_error(lexer.location(), "expected string literal after 'import'");

		auto path = unicode::stringFromU32String(unescape_string(maybe_str->loc, maybe_str->text));

		auto ret = std::make_unique<interp::ImportStmt>(loc);
		ret->file_path = std::move(path);

		return ret;
	}

	static std::unique_ptr<interp::Block> parse_namespace(Lexer& lexer)
	{
		must_expect(lexer, TT::KW_Namespace);

		if(lexer.peek() != TT::Identifier)
			parse_error(lexer.location(), "expected identifier after 'namespace'");

		auto [qid, len] = parse_qualified_id(lexer);

		qid.parents.push_back(std::move(qid.name));
		qid.name.clear();

		auto block = parse_block_or_stmt(lexer, /* is_top_level: */ true, /* braces: */ false);
		block->target_scope = std::move(qid);

		return block;
	}

	static std::unique_ptr<interp::HookBlock> parse_hook_block(Lexer& lexer)
	{
		must_expect(lexer, TT::At);
		auto block = std::make_unique<interp::HookBlock>(lexer.location());
		block->phase = parse_processing_phase(lexer);
		block->body = parse_block_or_stmt(lexer, /* is_top_level: */ false, /* braces: */ false);
		return block;
	}





	static std::unique_ptr<interp::Stmt> parse_stmt(Lexer& lexer, bool is_top_level)
	{
		auto lm = LexerModer(lexer, Lexer::Mode::Script);

		if(lexer.eof())
			parse_error(lexer.location(), "unexpected end of file");

		// just consume empty statements
		while(lexer.expect(TT::Semicolon))
			;

		auto tok = lexer.peek();
		std::unique_ptr<interp::Stmt> stmt {};

		bool optional_semicolon = false;

		if(tok == TT::KW_Let || tok == TT::KW_Var)
		{
			stmt = parse_var_defn(lexer, is_top_level);
		}
		else if(tok == TT::KW_Fn)
		{
			stmt = parse_function_defn(lexer);
			optional_semicolon = true;
		}
		else if(tok == TT::KW_Struct)
		{
			stmt = parse_struct_defn(lexer);
			optional_semicolon = true;
		}
		else if(tok == TT::KW_Enum)
		{
			stmt = parse_enum_defn(lexer);
			optional_semicolon = true;
		}
		else if(tok == TT::KW_If)
		{
			stmt = parse_if_stmt(lexer);
			optional_semicolon = true;
		}
		else if(tok == TT::KW_Namespace)
		{
			stmt = parse_namespace(lexer);
			optional_semicolon = true;
		}
		else if(tok == TT::KW_While)
		{
			stmt = parse_while_loop(lexer);
			optional_semicolon = true;
		}
		else if(tok == TT::KW_Return)
		{
			stmt = parse_return_stmt(lexer);
		}
		else if(tok == TT::KW_Import)
		{
			stmt = parse_import_stmt(lexer);
		}
		else if(tok == TT::At)
		{
			stmt = parse_hook_block(lexer);
			optional_semicolon = true;
		}
		else if(tok == TT::KW_Continue)
		{
			lexer.next();
			stmt = std::make_unique<interp::ContinueStmt>(tok.loc);
		}
		else if(tok == TT::KW_Break)
		{
			lexer.next();
			stmt = std::make_unique<interp::BreakStmt>(tok.loc);
		}
		else
		{
			stmt = parse_expr(lexer);
		}

		if(not stmt)
			parse_error(tok.loc, "invalid start of statement");

		// check for assignment
		if(is_assignment_op(lexer.peek()))
		{
			if(auto expr = dynamic_cast<const interp::Expr*>(stmt.get()); expr != nullptr)
				stmt = parse_assignment(util::static_pointer_cast<interp::Expr>(std::move(stmt)), lexer);
		}

		if(not lexer.expect(TT::Semicolon) && not optional_semicolon)
			parse_error(lexer.previous().loc, "expected ';' after statement, found '{}'", lexer.peek().text);

		return stmt;
	}

	static std::unique_ptr<ScriptBlock> parse_script_block(Lexer& lexer)
	{
		auto lm = LexerModer(lexer, Lexer::Mode::Script);
		must_expect(lexer, KW_SCRIPT_BLOCK);

		auto phase = ProcessingPhase::Layout;
		if(lexer.expect(TT::At))
			phase = parse_processing_phase(lexer);

		auto block = std::make_unique<ScriptBlock>(phase);

		std::optional<interp::QualifiedId> target_scope;
		if(lexer.peek() == TT::ColonColon || lexer.peek() == TT::Identifier)
		{
			// parse the thing manually, because it's slightly different from normal.
			interp::QualifiedId qid {};
			if(lexer.expect(TT::ColonColon))
				qid.top_level = true;

			while(lexer.peek() != TT::LBrace)
			{
				auto id = lexer.match(TT::Identifier);
				if(not id.has_value())
					parse_error(lexer.location(), "expected identifier in scope before '{'");

				qid.parents.push_back(id->text.str());
				if(not lexer.expect(TT::ColonColon))
					parse_error(lexer.location(), "expected '::' after identifier");
			}

			target_scope = std::move(qid);
		}

		block->body = parse_block_or_stmt(lexer, /* mandatory_braces: */ true, /* braces: */ true);
		block->body->target_scope = std::move(target_scope);

		return block;
	}


	static std::unique_ptr<tree::InlineObject> parse_inline_obj(Lexer& lexer);
	static std::optional<std::unique_ptr<Paragraph>> parse_paragraph(Lexer& lexer);
	static std::vector<std::unique_ptr<BlockObject>> parse_top_level(Lexer& lexer);

	static std::unique_ptr<ScriptCall> parse_inline_script_call(Lexer& lexer)
	{
		auto lm = LexerModer(lexer, Lexer::Mode::Script);

		auto loc = lexer.peek().loc;
		auto [qid, len] = parse_qualified_id(lexer);
		loc.length = len;

		auto lhs = std::make_unique<interp::Ident>(loc);
		lhs->name = std::move(qid);

		auto phase = ProcessingPhase::Layout;

		if(lexer.expect(TT::At))
			phase = parse_processing_phase(lexer);

		auto open_paren = lexer.peek();
		if(open_paren != TT::LParen)
			parse_error(open_paren.loc, "expected '(' after qualified-id in inline script call");

		auto sc = std::make_unique<ScriptCall>(phase);
		sc->call = parse_function_call(lexer, std::move(lhs));

		// check if we have trailing things
		// TODO: support manually specifying inline \t
		bool is_block = false;

		// if we see a semicolon or a new paragraph, stop.
		if(lexer.expect(TT::Semicolon) || lexer.peekWithMode(Lexer::Mode::Text) == TT::ParagraphBreak)
			return sc;

		if(lexer.expect(TT::Backslash))
		{
			if(auto n = lexer.match(TT::Identifier); not n.has_value() || n->text != KW_PARA_BLOCK)
				parse_error(lexer.location(), "expected 'p' after '\\' in script call, got '{}'",
					n ? n->text : lexer.peek().text);

			is_block = true;
			if(lexer.peek() != TT::LBrace)
				parse_error(lexer.location(), "expected '{' after '\\p'");
		}

		if(lexer.expect(TT::LBrace))
		{
			auto loc = lexer.location();
			if(is_block)
			{
				auto obj = std::make_unique<interp::TreeBlockExpr>(loc);
				auto container = tree::Container::makeVertBox();
				auto inner = parse_top_level(lexer);

				container->contents().insert(container->contents().end(), std::move_iterator(inner.begin()),
					std::move_iterator(inner.end()));

				obj->object = std::move(container);

				if(not lexer.expect(TT::RBrace))
					parse_error(lexer.location(), "expected closing '}', got '{}'", lexer.peek().text);

				sc->call->arguments.push_back(interp::FunctionCall::Arg {
					.name = std::nullopt,
					.value = std::move(obj),
				});
			}
			else
			{
				auto obj = std::make_unique<interp::TreeInlineExpr>(loc);

				while(not lexer.expect(TT::RBrace))
					obj->objects.push_back(parse_inline_obj(lexer));

				sc->call->arguments.push_back(interp::FunctionCall::Arg {
					.name = std::nullopt,
					.value = std::move(obj),
				});
			}
		}
		else if(is_block)
		{
			parse_error(lexer.location(), "expected '{' after \\p in script call");
		}

		return sc;
	}

	static std::unique_ptr<ScriptObject> parse_script_object(Lexer& lexer)
	{
		auto next = lexer.peekWithMode(Lexer::Mode::Script);
		if(next == TT::Identifier && next.text == KW_SCRIPT_BLOCK)
			return parse_script_block(lexer);

		else if(next == TT::Identifier || next == TT::ColonColon)
			return parse_inline_script_call(lexer);

		else
			parse_error(next.loc, "unexpected token '{}' after '\\'", next.text);
	}


	static std::unique_ptr<tree::InlineObject> parse_inline_obj(Lexer& lexer)
	{
		auto _ = LexerModer(lexer, Lexer::Mode::Text);

		// for now, these must be text or calls
		auto tok = lexer.next();
		if(tok == TT::Text)
		{
			return std::make_unique<Text>(escape_word_text(tok.loc, tok.text));
		}
		else if(tok == TT::Backslash)
		{
			auto next = lexer.peekWithMode(Lexer::Mode::Script);
			if(next.text == KW_SCRIPT_BLOCK)
				parse_error(next.loc, "script blocks are not allowed in an inline context");

			return parse_inline_script_call(lexer);
		}
		else
		{
			parse_error(lexer.location(), "unexpected token '{}' in inline object", tok.text);
		}
	}


	static std::optional<std::unique_ptr<Paragraph>> parse_paragraph(Lexer& lexer)
	{
		auto para = std::make_unique<Paragraph>();
		while(true)
		{
			auto peek = lexer.peek();
			if(peek == TT::ParagraphBreak || peek == TT::EndOfFile)
			{
				break;
			}
			else if(peek != TT::Backslash && peek != TT::Text)
			{
				if(para->contents().size() == 0)
					parse_error(lexer.location(), "unexpected token '{}' in inline object", peek.text);
				else
					break;
			}

			para->addObject(parse_inline_obj(lexer));
		}

		return para;
	}

	static std::vector<std::unique_ptr<BlockObject>> parse_top_level(Lexer& lexer)
	{
		auto lm = LexerModer(lexer, Lexer::Mode::Text);

		// this only needs to happen at the beginning
		lexer.skipWhitespaceAndComments();

		std::vector<std::unique_ptr<BlockObject>> objs {};

		while(true)
		{
			if(lexer.eof())
				break;

			if(auto tok = lexer.peek(); tok == TT::Text)
			{
				if(auto ret = parse_paragraph(lexer); ret.has_value())
					objs.push_back(std::move(*ret));
			}
			else if(tok == TT::ParagraphBreak)
			{
				lexer.next();
				lexer.skipWhitespaceAndComments();

				continue;
			}
			else if(tok == TT::Backslash)
			{
				lexer.next();
				objs.push_back(parse_script_object(lexer));
			}
			else if(tok == TT::LBrace)
			{
				lexer.next();

				auto container = tree::Container::makeVertBox();
				auto inner = parse_top_level(lexer);

				container->contents().insert(container->contents().end(), std::move_iterator(inner.begin()),
					std::move_iterator(inner.end()));

				if(not lexer.expect(TT::RBrace))
					parse_error(lexer.location(), "expected closing '}', got '{}'", lexer.peek().text);

				objs.push_back(std::move(container));
			}
			else
			{
				break;
			}
		}

		return objs;
	}


	static std::pair<std::unique_ptr<interp::Block>, std::unique_ptr<interp::FunctionCall>> parse_preamble(Lexer& lexer)
	{
		auto _ = LexerModer(lexer, Lexer::Mode::Script);

		auto block = std::make_unique<interp::Block>(lexer.location());
		std::unique_ptr<interp::FunctionCall> doc_start {};

		while(not lexer.eof())
		{
			if(lexer.expect(TT::Backslash))
			{
				if(not lexer.expect(KW_START_DOCUMENT))
					parse_error(lexer.location(), "expected '\\{}' in preamble (saw '\\')", KW_START_DOCUMENT);

				auto callee = interp::QualifiedId {
					.top_level = true,
					.parents = { "builtin" },
					.name = KW_START_DOCUMENT,
				};

				auto ident = std::make_unique<interp::Ident>(lexer.previous().loc);
				ident->name = std::move(callee);

				doc_start = parse_function_call(lexer, std::move(ident));

				if(lexer.expect(TT::Semicolon))
					;

				break;
			}
			else
			{
				block->body.push_back(parse_stmt(lexer, /* is_top_level: */ true));
			}
		}

		block->target_scope = interp::QualifiedId { .top_level = true };
		return { std::move(block), std::move(doc_start) };
	}


	Document parse(zst::str_view filename, zst::str_view contents)
	{
		auto lexer = Lexer(filename, contents);

		auto [preamble, doc_start] = parse_preamble(lexer);
		auto document = Document(std::move(preamble), std::move(doc_start));

		while(not lexer.eof())
		{
			auto objs = parse_top_level(lexer);
			for(auto& obj : objs)
				document.addObject(std::move(obj));

			if(not lexer.eof())
				parse_error(lexer.location(), "unexpected token '{}'", lexer.peek().text);
			else
				break;
		}

		return document;
	}
}
