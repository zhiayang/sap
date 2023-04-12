// parser.cpp
// Copyright (c) 2022, zhiayang
// SPDX-License-Identifier: Apache-2.0

#include "util.h"     // for consumeCodepointFromUtf8, is_one_of
#include "location.h" // for error, Location

#include "sap/units.h"
#include "sap/frontend.h" // for Token, Lexer, TokenType, Lexer::Mode

#include "tree/raw.h"
#include "tree/base.h"
#include "tree/wrappers.h"
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

	constexpr auto KW_BOX_BLOCK = "box";
	constexpr auto KW_PARA_BLOCK = "para";
	constexpr auto KW_LINE_BLOCK = "line";
	constexpr auto KW_VBOX_BLOCK = "vbox";
	constexpr auto KW_HBOX_BLOCK = "hbox";
	constexpr auto KW_ZBOX_BLOCK = "zbox";

	constexpr auto KW_SCRIPT_BLOCK = "script";
	constexpr auto KW_START_DOCUMENT = "start_document";

	constexpr auto KW_PHASE_LAYOUT = "layout";
	constexpr auto KW_PHASE_POSITION = "position";
	constexpr auto KW_PHASE_POST = "post";
	constexpr auto KW_PHASE_FINALISE = "finalise";
	constexpr auto KW_PHASE_RENDER = "render";

	using TT = TokenType;
	using namespace sap::tree;

	template <typename T>
	auto OkMove(T& x)
	{
		return Ok(std::move(x));
	}

	template <typename T>
	static void must_expect(Lexer& lexer, T&& x)
	{
		if(not lexer.expect(static_cast<T&&>(x)))
			sap::internal_error("????");
	}

	static ErrorOr<std::u32string> escape_word_text(const Location& loc, zst::str_view text_)
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
				else if(text[1] == ';')
					ret.push_back(U';');
				else
					return ErrMsg(loc, "unrecognised escape sequence '\\{}'", text[1]);

				text.remove_prefix(2);
			}
			else
			{
				ret += unicode::consumeCodepointFromUtf8(text);
			}
		}

		return OkMove(ret);
	}

	static ErrorOr<std::pair<std::u32string, size_t>> unescape_string_part(const Location& loc, zst::byte_span orig)
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
						return ErrMsg(loc, "invalid escape sequence starting with '{}'", cp);
						break;
				}
			}
			else
			{
				break;
			}
		}

		return Ok<std::pair<std::u32string, size_t>>(std::move(ret), orig.size() - bs.size());
	}



	static ErrorOr<std::u32string> unescape_string(const Location& loc, zst::str_view sv)
	{
		std::u32string ret {};
		ret.reserve(sv.size());

		auto bs = sv.bytes();
		while(not bs.empty())
		{
			auto [e, n] = TRY(unescape_string_part(loc, bs));
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

		return OkMove(ret);
	}


	static ErrorOr<ProcessingPhase> parse_processing_phase(Lexer& lexer)
	{
		auto ident = lexer.match(TT::Identifier);
		if(not ident.has_value())
			return ErrMsg(lexer.location(), "expected identifier after '@'");

		if(ident->text == KW_PHASE_LAYOUT)
			return Ok(ProcessingPhase::Layout);
		else if(ident->text == KW_PHASE_POSITION)
			return Ok(ProcessingPhase::Position);
		else if(ident->text == KW_PHASE_POST)
			return Ok(ProcessingPhase::PostLayout);
		else if(ident->text == KW_PHASE_FINALISE)
			return Ok(ProcessingPhase::Finalise);
		else if(ident->text == KW_PHASE_RENDER)
			return Ok(ProcessingPhase::Render);

		return ErrMsg(lexer.location(), "invalid phase '{}'", ident->text);
	}




	static int get_front_token_precedence(Lexer& lexer)
	{
		switch(lexer.peek())
		{
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

			case TT::KW_And: return 50;
			case TT::KW_Or: return 40;

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

	static bool is_logical_binary_op(TokenType tok)
	{
		return util::is_one_of(tok, TT::KW_And, TT::KW_Or);
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

	static interp::LogicalBinOp::Op convert_logical_binop(TokenType tok)
	{
		switch(tok)
		{
			case TT::KW_And: return interp::LogicalBinOp::Op::And;
			case TT::KW_Or: return interp::LogicalBinOp::Op::Or;
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

	template <typename T>
	using ErrorOrUniquePtr = ErrorOr<std::unique_ptr<T>>;

	static ErrorOrUniquePtr<interp::Stmt> parse_stmt(Lexer& lexer, bool is_top_level, bool expect_semi = true);
	static ErrorOrUniquePtr<interp::Expr> parse_expr(Lexer& lexer);
	static ErrorOrUniquePtr<interp::Expr> parse_unary(Lexer& lexer);
	static ErrorOrUniquePtr<interp::Expr> parse_primary(Lexer& lexer);

	static ErrorOr<std::pair<zst::SharedPtr<InlineObject>, bool>> parse_inline_obj(Lexer& lexer);
	static ErrorOr<std::vector<zst::SharedPtr<BlockObject>>> parse_top_level(Lexer& lexer);
	static ErrorOr<std::optional<zst::SharedPtr<Paragraph>>> parse_paragraph(Lexer& lexer);


	static ErrorOr<std::pair<interp::QualifiedId, uint32_t>> parse_qualified_id(Lexer& lexer)
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

			auto tok = TRY(lexer.next());
			if(tok != TT::Identifier)
				return ErrMsg(tok.loc, "expected identifier, got '{}'", tok.text);

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

		return Ok<std::pair<interp::QualifiedId, uint32_t>>(std::move(qid), len);
	}

	static ErrorOrUniquePtr<interp::FunctionCall> parse_function_call(Lexer& lexer,
	    std::unique_ptr<interp::Expr> callee)
	{
		if(not lexer.expect(TT::LParen))
			return ErrMsg(lexer.location(), "expected '(' to begin function call");

		auto call = std::make_unique<interp::FunctionCall>(callee->loc());
		call->callee = std::move(callee);

		// parse arguments.
		for(bool first = true; not lexer.expect(TT::RParen); first = false)
		{
			if(not first && not lexer.expect(TT::Comma))
			{
				return ErrMsg(lexer.location(), "expected ',' or ')' in call argument list, got '{}'",
				    lexer.peek().text);
			}

			else if(first && lexer.peek() == TT::Comma)
				return ErrMsg(lexer.location(), "unexpected ','");

			// if we are not first, and we see a rparen (at this point), break
			// we check again because it allows us to handle trailing commas (for >0 args)
			if(lexer.expect(TT::RParen))
				break;

			bool is_arg_named = false;

			interp::FunctionCall::Arg arg {};
			if(lexer.peek() == TT::Identifier)
			{
				auto save = lexer.save();
				auto name = TRY(lexer.next());

				if(lexer.expect(TT::Colon))
				{
					arg.value = TRY(parse_expr(lexer));
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
				arg.value = TRY(parse_expr(lexer));
			}

			call->arguments.push_back(std::move(arg));
		}

		// TODO: (potentially multiple) trailing blocks
		return OkMove(call);
	}

	static ErrorOrUniquePtr<interp::Expr> parse_subscript(Lexer& lexer, std::unique_ptr<interp::Expr> lhs)
	{
		auto loc = lexer.peek().loc;

		must_expect(lexer, TT::LSquare);
		auto subscript = TRY(parse_expr(lexer));

		if(not lexer.expect(TT::RSquare))
			return ErrMsg(lexer.location(), "expected ']'");

		auto ret = std::make_unique<interp::SubscriptOp>(loc);
		ret->array = std::move(lhs);
		ret->index = std::move(subscript);
		return OkMove(ret);
	}

	static ErrorOrUniquePtr<interp::FunctionCall> parse_ufcs(Lexer& lexer,
	    std::unique_ptr<interp::Expr> first_arg,
	    const std::string& method_name,
	    bool is_optional)
	{
		auto method = std::make_unique<interp::Ident>(lexer.location());
		method->name.top_level = false;
		method->name.name = method_name;

		auto call = TRY(parse_function_call(lexer, std::move(method)));

		call->rewritten_ufcs = true;
		call->is_optional_ufcs = is_optional;

		call->arguments.insert(call->arguments.begin(),
		    interp::FunctionCall::Arg {
		        .name = std::nullopt,
		        .value = std::move(first_arg),
		    });

		return OkMove(call);
	}

	static ErrorOrUniquePtr<interp::Expr> parse_postfix_unary(Lexer& lexer,
	    std::unique_ptr<interp::Expr> lhs,
	    bool* success)
	{
		*success = true;

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
			return OkMove(ret);
		}
		else if(auto t = lexer.match(TT::Exclamation); t.has_value())
		{
			auto ret = std::make_unique<interp::DereferenceOp>(t->loc);
			ret->expr = std::move(lhs);
			return OkMove(ret);
		}
		else if(auto t = lexer.peek(); t == TT::Period || t == TT::QuestionPeriod)
		{
			auto op_tok = TRY(lexer.next());

			// TODO: maybe support .0, .1 syntax for tuples
			auto field_name = lexer.match(TT::Identifier);
			if(not field_name.has_value())
				return ErrMsg(lexer.location(), "expected field name after '.'");

			auto newlhs = std::make_unique<interp::DotOp>(op_tok.loc);
			newlhs->is_optional = (op_tok == TT::QuestionPeriod);
			newlhs->rhs = field_name->text.str();
			newlhs->lhs = std::move(lhs);

			// special case method calls: UFCS, rewrite into a free function call.
			if(lexer.peek() == TT::LParen)
			{
				return parse_ufcs(lexer, std::move(newlhs->lhs), field_name->text.str(),
				    /* is_optional */ op_tok == TT::QuestionPeriod);
			}
			else
			{
				return OkMove(newlhs);
			}
		}
		else
		{
			*success = false;
			return OkMove(lhs);
		}
	}

	static ErrorOrUniquePtr<interp::Expr> parse_postfix_unary(Lexer& lexer)
	{
		bool keep_going = true;

		auto lhs = TRY(parse_primary(lexer));
		while(keep_going)
			lhs = TRY(parse_postfix_unary(lexer, std::move(lhs), &keep_going));

		return OkMove(lhs);
	}

	static ErrorOrUniquePtr<interp::Expr> parse_rhs(Lexer& lexer, std::unique_ptr<interp::Expr> lhs, int prio)
	{
		if(lexer.eof())
			return OkMove(lhs);

		while(true)
		{
			int prec = get_front_token_precedence(lexer);
			if(prec < prio && not is_right_associative(lexer))
				return OkMove(lhs);

			// if we see an assignment, bail without consuming the operator token.
			if(is_assignment_op(lexer.peek()))
				return OkMove(lhs);

			auto op_tok = TRY(lexer.next());

			auto rhs = TRY(parse_unary(lexer));
			int next = get_front_token_precedence(lexer);

			if(next > prec || is_right_associative(lexer))
				rhs = TRY(parse_rhs(lexer, std::move(rhs), prec + 1));

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
			else if(is_logical_binary_op(op_tok))
			{
				auto tmp = std::make_unique<interp::LogicalBinOp>(op_tok.loc);
				tmp->lhs = std::move(lhs);
				tmp->rhs = std::move(rhs);
				tmp->op = convert_logical_binop(op_tok);

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
				return ErrMsg(lexer.location(), "unknown operator token '{}'", lexer.peek().text);
			}
		}
	}

	static ErrorOrUniquePtr<interp::StructLit> parse_struct_literal(Lexer& lexer, interp::QualifiedId id)
	{
		if(not lexer.expect(TT::LBrace))
			return ErrMsg(lexer.location(), "expected '{' for struct literal");

		auto ret = std::make_unique<interp::StructLit>(lexer.location());
		ret->struct_name = std::move(id);

		while(not lexer.expect(TT::RBrace))
		{
			// if(not lexer.expect(TT::Period))
			// 	return ErrMsg(lexer.location(), "expected '.' to begin designated struct initialiser, found '{}'");

			auto field_name = lexer.match(TT::Identifier);
			if(not field_name.has_value())
				return ErrMsg(lexer.location(), "expected identifier for field name");

			/*
			    note: handle the thing like in crablang, where
			    { name: name } can be shortened to just { name }
			*/

			std::unique_ptr<interp::Expr> value {};

			if(lexer.expect(TT::Colon))
			{
				value = TRY(parse_expr(lexer));
			}
			else
			{
				value = std::make_unique<interp::Ident>(field_name->loc,
				    interp::QualifiedId { .name = field_name->str() });
			}

			ret->field_inits.push_back(interp::StructLit::Arg {
			    .name = field_name->text.str(),
			    .value = std::move(value),
			});

			if(not lexer.expect(TT::Comma) && lexer.peek() != TT::RBrace)
				return ErrMsg(lexer.location(), "expected ',' in struct initialiser list");
		}

		return OkMove(ret);
	}

	static ErrorOrUniquePtr<interp::FStringExpr> parse_fstring(Lexer& main_lexer, const Token& tok)
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

				fstring_parts.push_back(TRY(parse_expr(tmp_lexer)));

				fstr.remove_prefix((size_t) (tmp_lexer.stream().bytes().data() - fstr.data()));
				if(not fstr.starts_with('}'))
				{
					return ErrMsg(tmp_lexer.location(), "expected '}' to end f-string expression, found '{}'",
					    fstr.front());
				}

				fstr.remove_prefix(1);
			}
			else
			{
				auto [e, n] = TRY(unescape_string_part(tok.loc, fstr));
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

		return OkMove(ret);
	}

	static ErrorOr<PType> parse_type(Lexer& lexer);

	static ErrorOrUniquePtr<interp::Expr> parse_primary(Lexer& lexer)
	{
		auto lm = LexerModer(lexer, Lexer::Mode::Script);
		if(auto t = lexer.peek(); t == TT::Identifier || t == TT::ColonColon)
		{
			if(t.text == KW_NULL)
				return Ok(std::make_unique<interp::NullLit>(TRY(lexer.next()).loc));

			auto loc = t.loc;
			auto [qid, len] = TRY(parse_qualified_id(lexer));

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
				return OkMove(ident);
			}
		}
		else if(lexer.peek() == TT::LBrace)
		{
			// '{' starts a struct literal with no name
			auto ret = TRY(parse_struct_literal(lexer, {}));
			ret->is_anonymous = true;

			return OkMove(ret);
		}
		else if(auto num = lexer.match(TT::Number); num)
		{
			// check if we have a unit later.
			if(auto unit = lexer.match(TT::Identifier); unit.has_value())
			{
				auto maybe_unit = DynLength::stringToUnit(unit->text);
				if(not maybe_unit.has_value())
					return ErrMsg(lexer.location(), "unknown unit '{}' following number literal", unit->text);

				auto ret = std::make_unique<interp::LengthExpr>(num->loc);
				ret->length = DynLength(std::stod(num->text.str()), *maybe_unit);

				return OkMove(ret);
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
				return OkMove(ret);
			}
		}
		else if(auto str = lexer.match(TT::String); str)
		{
			auto ret = std::make_unique<interp::StringLit>(str->loc);
			ret->string = TRY(unescape_string(str->loc, str->text));

			return OkMove(ret);
		}
		else if(auto fstr = lexer.match(TT::FString); fstr)
		{
			return parse_fstring(lexer, *fstr);
		}
		else if(auto bool_true = lexer.match(TT::KW_True); bool_true)
		{
			return Ok(std::make_unique<interp::BooleanLit>(bool_true->loc, true));
		}
		else if(auto bool_false = lexer.match(TT::KW_False); bool_false)
		{
			return Ok(std::make_unique<interp::BooleanLit>(bool_true->loc, false));
		}
		else if(lexer.expect(TT::Period))
		{
			auto ident = lexer.match(TT::Identifier);
			if(not ident.has_value())
				return ErrMsg(lexer.location(), "expected enumerator name (an identifier) after '.'");

			auto ret = std::make_unique<interp::EnumLit>(ident->loc);
			ret->name = ident->text.str();

			return OkMove(ret);
		}
		else if(lexer.expect(TT::LParen))
		{
			auto inside = parse_expr(lexer);
			if(not lexer.expect(TT::RParen))
				return ErrMsg(lexer.location(), "expected closing ')'");

			return inside;
		}
		else if(lexer.expect(TT::LSquare))
		{
			auto array = std::make_unique<interp::ArrayLit>(lexer.location());
			if(lexer.expect(TT::Colon))
				array->elem_type = TRY(parse_type(lexer));

			while(lexer.peek() != TT::RSquare)
			{
				array->elements.push_back(TRY(parse_expr(lexer)));
				if(lexer.expect(TT::Comma))
				{
					continue;
				}
				else if(lexer.peek() == TT::RSquare)
				{
					break;
				}
				else
				{
					return ErrMsg(lexer.location(), "expected ']' or ',' in array literal, found '{}'",
					    lexer.peek().text);
				}
			}

			must_expect(lexer, TT::RSquare);
			return OkMove(array);
		}
		else if(lexer.expect(TT::Backslash))
		{
			/*
			    check what's the next guy. we expect either:
			    1. 'box' -- a single block object
			    2. 'vbox' / 'hbox' / 'zbox' -- ≥0 block objects
			    3. 'para' -- paragraph
			    4. '{' -- inline object(s)
			*/
			if(lexer.expect(TT::LBrace))
			{
				auto inlines = std::make_unique<interp::TreeInlineExpr>(lexer.location());
				while(lexer.peek() != TT::RBrace)
				{
					auto [obj, end_para] = TRY(parse_inline_obj(lexer));
					inlines->objects.push_back(std::move(obj));

					if(end_para)
						break;
				}

				if(not lexer.expect(TT::RBrace))
					return ErrMsg(lexer.location(), "unterminated inline text expression");

				return OkMove(inlines);
			}
			else if(lexer.expect(KW_LINE_BLOCK))
			{
				auto blk = std::make_unique<interp::TreeBlockExpr>(lexer.location());

				if(not lexer.expect(TT::LBrace))
					return ErrMsg(lexer.location(), "expected '{' after '\\{}'", KW_LINE_BLOCK);

				std::vector<zst::SharedPtr<tree::InlineObject>> inlines {};
				while(lexer.peek() != TT::RBrace)
				{
					auto [obj, end_para] = TRY(parse_inline_obj(lexer));
					inlines.push_back(std::move(obj));

					if(end_para)
						break;
				}

				if(not lexer.expect(TT::RBrace))
					return ErrMsg(lexer.location(), "unterminated inline text expression");

				auto line = zst::make_shared<tree::WrappedLine>(std::move(inlines));

				blk->object = std::move(line);
				return OkMove(blk);
			}
			else if(lexer.expect(KW_BOX_BLOCK))
			{
				auto blk = std::make_unique<interp::TreeBlockExpr>(lexer.location());

				if(not lexer.expect(TT::LBrace))
					return ErrMsg(lexer.location(), "expected '{' after '\\{}'", KW_BOX_BLOCK);

				auto objs = TRY(parse_top_level(lexer));
				if(not lexer.expect(TT::RBrace))
					return ErrMsg(lexer.location(), "unterminated block expression");

				if(objs.size() != 1)
				{
					return ErrMsg(lexer.location(), "expected exactly one block object inside '\\{}' (found {})",
					    KW_BOX_BLOCK, objs.size());
				}

				blk->object = std::move(objs[0]);
				return OkMove(blk);
			}
			else if(lexer.peek() == TT::Identifier
			        && util::is_one_of(lexer.peek().text, KW_VBOX_BLOCK, KW_HBOX_BLOCK, KW_ZBOX_BLOCK))
			{
				auto blk = std::make_unique<interp::TreeBlockExpr>(lexer.location());
				auto x = TRY(lexer.next());

				if(not lexer.expect(TT::LBrace))
					return ErrMsg(lexer.location(), "expected '{' after '\\{}'", KW_BOX_BLOCK);

				auto objs = TRY(parse_top_level(lexer));
				if(not lexer.expect(TT::RBrace))
					return ErrMsg(lexer.location(), "unterminated block expression");

				using enum tree::Container::Direction;
				auto container = zst::make_shared<tree::Container>(
				    x.text == KW_HBOX_BLOCK ? Horizontal
				    : x.text == KW_VBOX_BLOCK
				        ? Vertical
				        : None);

				container->contents() = std::move(objs);
				blk->object = std::move(container);
				return OkMove(blk);
			}
			else
			{
				return ErrMsg(lexer.location(), "invalid text expression '\\{}'", lexer.peek().text);
			}
		}
		else
		{
			return ErrMsg(lexer.location(), "invalid start of expression '{}'", lexer.peek().text);
		}
	}

	static ErrorOrUniquePtr<interp::Expr> parse_unary(Lexer& lexer)
	{
		if(auto t = lexer.match(TT::Ampersand); t.has_value())
		{
			bool is_mutable = lexer.expect(TT::KW_Mut);

			auto ret = std::make_unique<interp::AddressOfOp>(t->loc);
			ret->expr = TRY(parse_unary(lexer));
			ret->is_mutable = is_mutable;
			return OkMove(ret);
		}
		else if(auto t = lexer.match(TT::Asterisk); t.has_value())
		{
			auto ret = std::make_unique<interp::MoveExpr>(t->loc);
			ret->expr = TRY(parse_unary(lexer));
			return OkMove(ret);
		}
		else if(auto t = lexer.match(TT::Ellipsis); t.has_value())
		{
			return Ok(std::make_unique<interp::ArraySpreadOp>(t->loc, TRY(parse_unary(lexer))));
		}
		else if(auto t = lexer.match(TT::Minus); t.has_value())
		{
			auto ret = std::make_unique<interp::UnaryOp>(t->loc);
			ret->expr = TRY(parse_unary(lexer));
			ret->op = interp::UnaryOp::Op::Minus;
			return OkMove(ret);
		}
		else if(auto t = lexer.match(TT::Plus); t.has_value())
		{
			auto ret = std::make_unique<interp::UnaryOp>(t->loc);
			ret->expr = TRY(parse_unary(lexer));
			ret->op = interp::UnaryOp::Op::Plus;
			return OkMove(ret);
		}
		else if(auto t = lexer.match(TT::KW_Not); t.has_value())
		{
			auto ret = std::make_unique<interp::UnaryOp>(t->loc);
			ret->expr = TRY(parse_unary(lexer));
			ret->op = interp::UnaryOp::Op::LogicalNot;
			return OkMove(ret);
		}

		return parse_postfix_unary(lexer);
	}

	static ErrorOrUniquePtr<interp::Expr> parse_expr(Lexer& lexer)
	{
		auto lm = LexerModer(lexer, Lexer::Mode::Script);
		auto lhs = TRY(parse_unary(lexer));

		return parse_rhs(lexer, std::move(lhs), 0);
	}

	static ErrorOrUniquePtr<interp::AssignOp> parse_assignment(std::unique_ptr<interp::Expr> lhs, Lexer& lexer)
	{
		auto op = TRY(lexer.next());
		assert(is_assignment_op(op));

		auto ret = std::make_unique<interp::AssignOp>(op.loc);
		ret->op = convert_assignment_op(op);
		ret->lhs = std::move(lhs);
		ret->rhs = TRY(parse_expr(lexer));

		return OkMove(ret);
	}


	static ErrorOr<PType> parse_type(Lexer& lexer)
	{
		if(lexer.eof())
			return ErrMsg(lexer.location(), "unexpected end of file");

		using namespace interp;

		auto fst = lexer.peek();
		if(fst == TT::Identifier)
		{
			return Ok(PType::named(TRY(parse_qualified_id(lexer)).first));
		}
		else if(fst == TT::ColonColon)
		{
			return Ok(PType::named(TRY(parse_qualified_id(lexer)).first));
		}
		else if(fst == TT::Ampersand)
		{
			lexer.next();

			bool is_mutable = false;
			if(lexer.expect(TT::KW_Mut))
				is_mutable = true;

			return Ok(PType::pointer(TRY(parse_type(lexer)), is_mutable));
		}
		else if(fst == TT::Question)
		{
			lexer.next();
			return Ok(PType::optional(TRY(parse_type(lexer))));
		}
		else if(fst == TT::QuestionQuestion)
		{
			lexer.next();
			return Ok(PType::optional(PType::optional(TRY(parse_type(lexer)))));
		}
		else if(fst == TT::LSquare)
		{
			lexer.next();

			auto elm = TRY(parse_type(lexer));
			bool is_variadic = lexer.expect(TT::Ellipsis);

			if(not lexer.expect(TT::RSquare))
				return ErrMsg(lexer.location(), "expected ']' in array type");

			return Ok(PType::array(std::move(elm), is_variadic));
		}
		else if(fst == TT::LParen)
		{
			lexer.next();

			std::vector<PType> types {};
			for(bool first = true; not lexer.expect(TT::RParen); first = false)
			{
				if(not first && not lexer.expect(TT::Comma))
					return ErrMsg(lexer.location(), "expected ',' or ')' in type specifier");

				types.push_back(TRY(parse_type(lexer)));
			}

			// TODO: support tuples
			if(not lexer.expect(TT::RArrow))
				return ErrMsg(lexer.location(), "TODO: tuples not implemented");

			auto return_type = TRY(parse_type(lexer));
			return Ok(PType::function(std::move(types), return_type));
		}

		return ErrMsg(lexer.location(), "invalid start of type specifier '{}'", lexer.peek().text);
	}

	static ErrorOrUniquePtr<interp::VariableDefn> parse_var_defn(Lexer& lexer, bool is_top_level)
	{
		auto kw = TRY(lexer.next());
		assert(kw == TT::KW_Global || kw == TT::KW_Let || kw == TT::KW_Var);

		bool is_global = false;
		if(kw == TT::KW_Global)
		{
			is_global = true;
			kw = TRY(lexer.next());
			if(kw != TT::KW_Let && kw != TT::KW_Var)
				return ErrMsg(lexer.location(), "expected either 'var' or 'let' after 'global' keyword");
		}

		if(is_top_level && not is_global)
			return ErrMsg(kw.loc, "variables at top level must be declared 'global'");

		// expect a name
		auto maybe_name = lexer.match(TT::Identifier);
		if(not maybe_name.has_value())
			return ErrMsg(lexer.location(), "expected identifier after '{}'", kw.text);

		std::unique_ptr<interp::Expr> initialiser {};
		std::optional<PType> explicit_type {};

		if(lexer.match(TT::Colon))
			explicit_type = TRY(parse_type(lexer));

		if(lexer.match(TT::Equal))
			initialiser = TRY(parse_expr(lexer));

		return Ok(std::make_unique<interp::VariableDefn>(kw.loc, maybe_name->str(), /* is_mutable: */ kw == TT::KW_Var,
		    is_global, std::move(initialiser), std::move(explicit_type)));
	}

	static ErrorOr<interp::FunctionDecl::Param> parse_param(Lexer& lexer)
	{
		auto name = lexer.match(TT::Identifier);
		if(not name.has_value())
			return ErrMsg(lexer.location(), "expected parameter name");

		if(not lexer.expect(TT::Colon))
			return ErrMsg(lexer.location(), "expected ':' after parameter name");

		auto type = TRY(parse_type(lexer));
		std::unique_ptr<interp::Expr> default_value = nullptr;

		if(lexer.expect(TT::Equal))
			default_value = TRY(parse_expr(lexer));

		return Ok(interp::FunctionDecl::Param {
		    .name = name->text.str(),
		    .type = type,
		    .default_value = std::move(default_value),
		    .loc = name->loc,
		});
	}

	static ErrorOrUniquePtr<interp::Block> parse_block_or_stmt(Lexer& lexer, bool is_top_level, bool mandatory_braces)
	{
		auto block = std::make_unique<interp::Block>(lexer.location());
		if(lexer.expect(TT::LBrace))
		{
			while(not lexer.expect(TT::RBrace))
			{
				if(lexer.expect(TT::Semicolon))
					continue;

				block->body.push_back(TRY(parse_stmt(lexer, is_top_level)));
			}
		}
		else
		{
			if(mandatory_braces)
				return ErrMsg(lexer.location(), "expected '{' to begin a block");

			block->body.push_back(TRY(parse_stmt(lexer, is_top_level)));
		}

		return OkMove(block);
	}

	static ErrorOrUniquePtr<interp::FunctionDefn> parse_function_defn(Lexer& lexer)
	{
		using namespace interp;
		auto loc = lexer.location();
		must_expect(lexer, TT::KW_Fn);

		std::string name;
		if(auto name_tok = lexer.match(TT::Identifier); not name_tok.has_value())
			return ErrMsg(lexer.location(), "expected identifier after 'fn'");
		else
			name = name_tok->text.str();

		if(not lexer.expect(TT::LParen))
			return ErrMsg(lexer.location(), "expected '(' in function definition");

		std::vector<FunctionDecl::Param> params {};
		for(bool first = true; not lexer.expect(TT::RParen); first = false)
		{
			if(not first && not lexer.expect(TT::Comma))
				return ErrMsg(lexer.location(), "expected ',' or ')' in function parameter list");

			params.push_back(TRY(parse_param(lexer)));
		}

		auto return_type = PType::named(TYPE_VOID);
		if(lexer.expect(TT::RArrow))
			return_type = TRY(parse_type(lexer));

		auto defn = std::make_unique<interp::FunctionDefn>(loc, name, std::move(params), return_type);
		if(lexer.peek() != TT::LBrace)
			return ErrMsg(lexer.location(), "expected '{' to begin function body");

		defn->body = TRY(parse_block_or_stmt(lexer, /* is_top_level: */ false, /* braces: */ true));
		return OkMove(defn);
	}

	static ErrorOrUniquePtr<interp::StructDefn> parse_struct_defn(Lexer& lexer)
	{
		auto loc = lexer.location();
		must_expect(lexer, TT::KW_Struct);

		std::string name;
		if(auto name_tok = lexer.match(TT::Identifier); not name_tok.has_value())
			return ErrMsg(lexer.location(), "expected identifier after 'struct'");
		else
			name = name_tok->text.str();


		if(not lexer.expect(TT::LBrace))
			return ErrMsg(lexer.location(), "expected '{' in struct body");

		std::vector<interp::StructDefn::Field> fields {};
		while(not lexer.expect(TT::RBrace))
		{
			auto field_name = lexer.match(TT::Identifier);
			if(not field_name.has_value())
				return ErrMsg(lexer.location(), "expected field name in struct");

			if(not lexer.expect(TT::Colon))
				return ErrMsg(lexer.location(), "expected ':' after field name");

			auto field_type = TRY(parse_type(lexer));
			std::unique_ptr<interp::Expr> field_initialiser {};

			if(lexer.expect(TT::Equal))
				field_initialiser = TRY(parse_expr(lexer));

			fields.push_back(interp::StructDefn::Field {
			    .name = field_name->text.str(),
			    .type = std::move(field_type),
			    .initialiser = std::move(field_initialiser),
			});

			if(not lexer.match(TT::Semicolon) && lexer.peek() != TT::RBrace)
				return ErrMsg(lexer.location(), "expected ';' or '}' in struct body");
		}

		return Ok(std::make_unique<interp::StructDefn>(loc, name, std::move(fields)));
	}

	static ErrorOrUniquePtr<interp::EnumDefn> parse_enum_defn(Lexer& lexer)
	{
		auto loc = lexer.location();
		must_expect(lexer, TT::KW_Enum);

		std::string name;
		if(auto name_tok = lexer.match(TT::Identifier); not name_tok.has_value())
			return ErrMsg(lexer.location(), "expected identifier after 'enum'");
		else
			name = name_tok->text.str();

		const auto pt_int = PType::named(TYPE_INT);
		auto enum_type = pt_int;

		if(lexer.expect(TT::Colon))
			enum_type = TRY(parse_type(lexer));

		if(not lexer.expect(TT::LBrace))
			return ErrMsg(lexer.location(), "expected '{' for enum body");

		std::vector<interp::EnumDefn::EnumeratorDefn> enumerators {};
		while(not lexer.expect(TT::RBrace))
		{
			auto enum_name = lexer.match(TT::Identifier);
			if(not enum_name.has_value())
				return ErrMsg(lexer.location(), "expected enumerator name");

			std::unique_ptr<interp::Expr> enum_value {};

			if(lexer.expect(TT::Equal))
				enum_value = TRY(parse_expr(lexer));

			enumerators.push_back(interp::EnumDefn::EnumeratorDefn(enum_name->loc, enum_name->text.str(),
			    std::move(enum_value)));

			if(not lexer.match(TT::Semicolon))
				return ErrMsg(lexer.location(), "expected ';' after enumerator");
		}

		return Ok(std::make_unique<interp::EnumDefn>(std::move(loc), std::move(name), std::move(enum_type),
		    std::move(enumerators)));
	}





	static ErrorOrUniquePtr<interp::IfStmt> parse_if_stmt(Lexer& lexer)
	{
		auto loc = lexer.location();
		must_expect(lexer, TT::KW_If);

		if(not lexer.expect(TT::LParen))
			return ErrMsg(lexer.location(), "expected '(' after 'if'");

		auto if_stmt = std::make_unique<interp::IfStmt>(loc);

		if_stmt->if_cond = TRY(parse_expr(lexer));
		if(not lexer.expect(TT::RParen))
			return ErrMsg(lexer.location(), "expected ')' after condition");

		if_stmt->if_body = TRY(parse_block_or_stmt(lexer, /* is_top_level: */ false, /* braces: */ false));

		if(lexer.expect(TT::KW_Else))
			if_stmt->else_body = TRY(parse_block_or_stmt(lexer, /* is_top_level: */ false, /* braces: */ false));

		return OkMove(if_stmt);
	}

	static ErrorOrUniquePtr<interp::WhileLoop> parse_while_loop(Lexer& lexer)
	{
		auto loc = lexer.location();
		must_expect(lexer, TT::KW_While);

		if(not lexer.expect(TT::LParen))
			return ErrMsg(lexer.location(), "expected '(' after 'while'");

		auto loop = std::make_unique<interp::WhileLoop>(loc);

		loop->condition = TRY(parse_expr(lexer));
		if(not lexer.expect(TT::RParen))
			return ErrMsg(lexer.location(), "expected ')' after condition");

		loop->body = TRY(parse_block_or_stmt(lexer, /* is_top_level: */ false, /* braces: */ false));
		return OkMove(loop);
	}

	static ErrorOrUniquePtr<interp::Stmt> parse_for_loop(Lexer& lexer)
	{
		auto loc = lexer.location();
		must_expect(lexer, TT::KW_For);

		if(not lexer.expect(TT::LParen))
			return ErrMsg(lexer.location(), "expected '(' after 'for'");

		auto loop = std::make_unique<interp::ForLoop>(loc);

		if(not lexer.expect(TT::Semicolon))
		{
			loop->init = TRY(parse_stmt(lexer, /* top level: */ false, /* expect_semi: */ false));
			if(not lexer.expect(TT::Semicolon))
				return ErrMsg(lexer.location(), "expected ';' after init statement in for-loop");
		}

		if(not lexer.expect(TT::Semicolon))
		{
			loop->cond = TRY(parse_expr(lexer));
			if(not lexer.expect(TT::Semicolon))
				return ErrMsg(lexer.location(), "expected ';' after conditional expression in for-loop");
		}

		if(not lexer.expect(TT::RParen))
		{
			loop->update = TRY(parse_stmt(lexer, /* top level: */ false, /* expect semi: */ false));
			if(not lexer.expect(TT::RParen))
				return ErrMsg(lexer.location(), "expected ')' after update statement in for-loop");
		}

		loop->body = TRY(parse_block_or_stmt(lexer, /* is_top_level: */ false, /* braces: */ false));
		return OkMove(loop);
	}


	static ErrorOrUniquePtr<interp::ReturnStmt> parse_return_stmt(Lexer& lexer)
	{
		auto loc = lexer.location();
		must_expect(lexer, TT::KW_Return);

		auto ret = std::make_unique<interp::ReturnStmt>(loc);
		if(lexer.peek() == TT::Semicolon)
			return OkMove(ret);

		ret->expr = TRY(parse_expr(lexer));
		return OkMove(ret);
	}

	static ErrorOrUniquePtr<interp::ImportStmt> parse_import_stmt(Lexer& lexer)
	{
		auto loc = lexer.location();
		must_expect(lexer, TT::KW_Import);

		auto maybe_str = lexer.match(TT::String);

		if(not maybe_str.has_value())
			return ErrMsg(lexer.location(), "expected string literal after 'import'");

		auto path = unicode::stringFromU32String(TRY(unescape_string(maybe_str->loc, maybe_str->text)));

		auto ret = std::make_unique<interp::ImportStmt>(loc);
		ret->file_path = std::move(path);

		return OkMove(ret);
	}

	static ErrorOrUniquePtr<interp::Block> parse_namespace(Lexer& lexer)
	{
		must_expect(lexer, TT::KW_Namespace);

		if(lexer.peek() != TT::Identifier)
			return ErrMsg(lexer.location(), "expected identifier after 'namespace'");

		auto [qid, len] = TRY(parse_qualified_id(lexer));

		qid.parents.push_back(std::move(qid.name));
		qid.name.clear();

		auto block = TRY(parse_block_or_stmt(lexer, /* is_top_level: */ true, /* braces: */ false));
		block->target_scope = std::move(qid);

		return OkMove(block);
	}

	static ErrorOrUniquePtr<interp::HookBlock> parse_hook_block(Lexer& lexer)
	{
		must_expect(lexer, TT::At);
		auto block = std::make_unique<interp::HookBlock>(lexer.location());
		block->phase = TRY(parse_processing_phase(lexer));
		block->body = TRY(parse_block_or_stmt(lexer, /* is_top_level: */ false, /* braces: */ false));
		return OkMove(block);
	}





	static ErrorOrUniquePtr<interp::Stmt> parse_stmt(Lexer& lexer, bool is_top_level, bool expect_semi)
	{
		auto lm = LexerModer(lexer, Lexer::Mode::Script);

		if(lexer.eof())
			return ErrMsg(lexer.location(), "unexpected end of file");

		// just consume empty statements
		while(expect_semi && lexer.expect(TT::Semicolon))
			;

		auto tok = lexer.peek();
		std::unique_ptr<interp::Stmt> stmt {};

		bool optional_semicolon = false;

		if(tok == TT::KW_Global || tok == TT::KW_Let || tok == TT::KW_Var)
		{
			stmt = TRY(parse_var_defn(lexer, is_top_level));
		}
		else if(tok == TT::KW_Fn)
		{
			stmt = TRY(parse_function_defn(lexer));
			optional_semicolon = true;
		}
		else if(tok == TT::KW_Struct)
		{
			stmt = TRY(parse_struct_defn(lexer));
			optional_semicolon = true;
		}
		else if(tok == TT::KW_Enum)
		{
			stmt = TRY(parse_enum_defn(lexer));
			optional_semicolon = true;
		}
		else if(tok == TT::KW_If)
		{
			stmt = TRY(parse_if_stmt(lexer));
			optional_semicolon = true;
		}
		else if(tok == TT::KW_Namespace)
		{
			stmt = TRY(parse_namespace(lexer));
			optional_semicolon = true;
		}
		else if(tok == TT::KW_For)
		{
			stmt = TRY(parse_for_loop(lexer));
			optional_semicolon = true;
		}
		else if(tok == TT::KW_While)
		{
			stmt = TRY(parse_while_loop(lexer));
			optional_semicolon = true;
		}
		else if(tok == TT::KW_Return)
		{
			stmt = TRY(parse_return_stmt(lexer));
		}
		else if(tok == TT::KW_Import)
		{
			stmt = TRY(parse_import_stmt(lexer));
		}
		else if(tok == TT::At)
		{
			stmt = TRY(parse_hook_block(lexer));
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
			stmt = TRY(parse_expr(lexer));
		}

		if(not stmt)
			return ErrMsg(tok.loc, "invalid start of statement");

		// check for assignment
		if(is_assignment_op(lexer.peek()))
		{
			if(auto expr = dynamic_cast<const interp::Expr*>(stmt.get()); expr != nullptr)
				stmt = TRY(parse_assignment(util::static_pointer_cast<interp::Expr>(std::move(stmt)), lexer));
		}

		if(expect_semi && not lexer.expect(TT::Semicolon) && not optional_semicolon)
			return ErrMsg(lexer.previous().loc, "expected ';' after statement, found '{}'", lexer.peek().text);

		return OkMove(stmt);
	}

	static ErrorOr<zst::SharedPtr<ScriptBlock>> parse_script_block(Lexer& lexer)
	{
		auto lm = LexerModer(lexer, Lexer::Mode::Script);
		must_expect(lexer, KW_SCRIPT_BLOCK);

		auto phase = ProcessingPhase::Layout;
		if(lexer.expect(TT::At))
			phase = TRY(parse_processing_phase(lexer));

		auto block = zst::make_shared<ScriptBlock>(phase);

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
					return ErrMsg(lexer.location(), "expected identifier in scope before '{'");

				qid.parents.push_back(id->text.str());
				if(not lexer.expect(TT::ColonColon))
					return ErrMsg(lexer.location(), "expected '::' after identifier");
			}

			target_scope = std::move(qid);
		}

		block->body = TRY(parse_block_or_stmt(lexer, /* is_top_level: */ false, /* braces: */ true));
		block->body->target_scope = std::move(target_scope);

		return OkMove(block);
	}


	static ErrorOr<std::pair<zst::SharedPtr<ScriptCall>, bool>> parse_script_call(Lexer& lexer)
	{
		using PP = std::pair<zst::SharedPtr<ScriptCall>, bool>;

		auto lm = LexerModer(lexer, Lexer::Mode::Script);

		auto loc = lexer.peek().loc;
		auto [qid, len] = TRY(parse_qualified_id(lexer));
		loc.length = len;

		auto lhs = std::make_unique<interp::Ident>(loc);
		lhs->name = std::move(qid);

		auto phase = ProcessingPhase::Layout;

		if(lexer.expect(TT::At))
			phase = TRY(parse_processing_phase(lexer));

		auto open_paren = lexer.peek();
		if(open_paren != TT::LParen)
			return ErrMsg(open_paren.loc, "expected '(' after qualified-id in inline script call");

		auto sc = zst::make_shared<ScriptCall>(phase);
		sc->call = TRY(parse_function_call(lexer, std::move(lhs)));

		// check if we have trailing things
		// TODO: support manually specifying inline \t
		bool is_block = false;

		// if we see a semicolon or a new paragraph, stop.
		if(lexer.expect(TT::Semicolon))
			return Ok<PP>(std::move(sc), /* saw semicolon: */ true);

		else if(lexer.peekWithMode(Lexer::Mode::Text) == TT::ParagraphBreak)
			return Ok<PP>(std::move(sc), /* saw semicolon: */ false);

		auto save = lexer.save();
		if(lexer.expect(TT::Backslash) && lexer.expect(KW_PARA_BLOCK))
		{
			is_block = true;
			if(lexer.peek() != TT::LBrace)
				return ErrMsg(lexer.location(), "expected '{' after '\\{}'", KW_PARA_BLOCK);
		}
		else
		{
			lexer.rewind(std::move(save));
		}

		if(lexer.expect(TT::LBrace))
		{
			auto loc = lexer.location();
			if(is_block)
			{
				auto obj = std::make_unique<interp::TreeBlockExpr>(loc);
				auto container = tree::Container::makeVertBox();
				auto inner = TRY(parse_top_level(lexer));

				container->contents().insert(container->contents().end(), std::move_iterator(inner.begin()),
				    std::move_iterator(inner.end()));

				obj->object = std::move(container);

				if(not lexer.expect(TT::RBrace))
					return ErrMsg(lexer.location(), "expected closing '}', got '{}'", lexer.peek().text);

				sc->call->arguments.push_back(interp::FunctionCall::Arg {
				    .name = std::nullopt,
				    .value = std::move(obj),
				});
			}
			else
			{
				auto obj = std::make_unique<interp::TreeInlineExpr>(loc);

				while(not lexer.expect(TT::RBrace))
				{
					auto [tmp, _] = TRY(parse_inline_obj(lexer));
					obj->objects.push_back(std::move(tmp));
				}

				sc->call->arguments.push_back(interp::FunctionCall::Arg {
				    .name = std::nullopt,
				    .value = std::move(obj),
				});
			}
		}
		else if(is_block)
		{
			return ErrMsg(lexer.location(), "expected '{' after '\\{}' in script call", KW_PARA_BLOCK);
		}

		bool semi = lexer.expect(TT::Semicolon);
		return Ok<PP>(std::move(sc), semi);
	}

	static ErrorOr<std::pair<zst::SharedPtr<tree::InlineObject>, bool>> parse_inline_obj(Lexer& lexer)
	{
		using PP = std::pair<zst::SharedPtr<tree::InlineObject>, bool>;

		auto _ = LexerModer(lexer, Lexer::Mode::Text);

		// for now, these must be text or calls
		auto tok = TRY(lexer.next());
		if(tok == TT::Text)
		{
			return Ok<PP>(zst::make_shared<Text>(TRY(escape_word_text(tok.loc, tok.text))), false);
		}
		else if(tok == TT::Backslash)
		{
			auto next = lexer.peekWithMode(Lexer::Mode::Script);
			if(next.text == KW_SCRIPT_BLOCK)
				return ErrMsg(next.loc, "script blocks are not allowed in an inline context");

			return Ok<PP>(TRY(parse_script_call(lexer)));
		}
		else
		{
			return ErrMsg(lexer.location(), "unexpected token '{}' in inline object", tok.text);
		}
	}


	static ErrorOr<std::optional<zst::SharedPtr<Paragraph>>> parse_paragraph(Lexer& lexer)
	{
		auto para = zst::make_shared<Paragraph>();
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
				{
					if(peek == TT::RawBlock)
						return ErrMsg(lexer.location(), "raw blocks cannot appear inside paragraphs");
					else
						return ErrMsg(lexer.location(), "unexpected token '{}' in inline object", peek.text);
				}

				break;
			}

			auto [obj, end_para] = TRY(parse_inline_obj(lexer));
			para->addObject(std::move(obj));

			if(end_para)
				break;
		}

		return OkMove(para);
	}

	static ErrorOr<std::vector<zst::SharedPtr<BlockObject>>> parse_top_level(Lexer& lexer)
	{
		auto lm = LexerModer(lexer, Lexer::Mode::Text);

		// this only needs to happen at the beginning
		lexer.skipWhitespaceAndComments();

		std::vector<zst::SharedPtr<BlockObject>> objs {};

		while(true)
		{
			if(lexer.eof())
				break;

			auto tok = lexer.peek();
			if(tok == TT::Backslash)
			{
				auto save = lexer.save();
				lexer.next();

				// the idea here is that all script things go into paragraphs, except
				// \script{} blocks (those remain at top level).
				if(lexer.peekWithMode(Lexer::Mode::Script).text == KW_SCRIPT_BLOCK)
				{
					objs.push_back(TRY(parse_script_block(lexer)));
				}
				else
				{
					lexer.rewind(std::move(save));
					goto parse_para;
				}
			}
			else if(tok == TT::Text)
			{
			parse_para:
				if(auto ret = TRY(parse_paragraph(lexer)); ret.has_value())
					objs.push_back(std::move(*ret));

				lexer.skipWhitespaceAndComments();
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
				if(auto x = lexer.peekWithMode(Lexer::Mode::Script); x == TT::Identifier && x.text == KW_SCRIPT_BLOCK)
				{
					objs.push_back(TRY(parse_script_block(lexer)));
					continue;
				}

				std::vector<zst::SharedPtr<InlineObject>> inline_objs {};

				while(true)
				{
					auto [script_obj, end_para] = TRY(parse_script_call(lexer));
					inline_objs.push_back(std::move(script_obj));

					// check if the next guy is a text
					if(end_para || lexer.peekWithMode(Lexer::Mode::Text) == TT::Text)
					{
						// eat a paragraph and go away. this means that this script call
						// (and all preceeding ones) are just part of a paragraph.
						zst::SharedPtr<Paragraph> para {};
						if(not end_para)
							para = TRY(parse_paragraph(lexer)).value_or(zst::make_shared<Paragraph>());
						else
							para = zst::make_shared<Paragraph>();

						std::move(inline_objs.begin(), inline_objs.end(), std::back_inserter(para->contents()));
						objs.push_back(std::move(para));
						inline_objs.clear();

						goto finish;
					}
					else if(lexer.peekWithMode(Lexer::Mode::Script) == TT::Backslash)
					{
						// we got another script-like thing, continue
						lexer.nextWithMode(Lexer::Mode::Script);

						// we are currently in paragraph context, *UNLESS* end_para is true
						auto tmp = lexer.peekWithMode(Lexer::Mode::Script);
						if(tmp == TT::Identifier && tmp.text == KW_SCRIPT_BLOCK)
						{
							if(end_para)
							{
								objs.push_back(TRY(parse_script_block(lexer)));
								goto finish;
							}
							else
							{
								return ErrMsg(tmp.loc, "script blocks not allowed in paragraphs");
							}
						}

						// go again, parsing another script call if necessary.
						continue;
					}

					break;
				}

				// if we got here, it means that we're out of (potential) inline objects.
				// put the current ones into a paragraph, then continue parsing other stuff.
				if(not inline_objs.empty())
				{
					auto para = zst::make_shared<Paragraph>();
					std::move(inline_objs.begin(), inline_objs.end(), std::back_inserter(para->contents()));

					objs.push_back(std::move(para));
				}

			finish:;
			}
			else if(tok == TT::LBrace)
			{
				lexer.next();

				auto container = tree::Container::makeVertBox();
				auto inner = TRY(parse_top_level(lexer));

				container->contents().insert(container->contents().end(), std::move_iterator(inner.begin()),
				    std::move_iterator(inner.end()));

				if(not lexer.expect(TT::RBrace))
					return ErrMsg(lexer.location(), "expected closing '}', got '{}'", lexer.peek().text);

				objs.push_back(std::move(container));
			}
			else if(tok == TT::RawBlock)
			{
				lexer.next();

				// the string we get from the lexer includes the ```, the attributes,
				// and the trailing ```.
				auto k = tok.text.find_first_not_of('`');
				auto attrs = tok.text.drop(k).take_until('\n');
				auto foo = attrs.size();

				attrs = attrs.trim_whitespace();

				auto body = tok.text.drop(k + foo + 1).drop_last(k);
				auto text = unicode::u32StringFromUtf8(body.bytes());

				// TODO: use attrs
				objs.push_back(zst::make_shared<tree::RawBlock>(std::move(text)));
				lexer.skipWhitespaceAndComments();
			}
			else
			{
				break;
			}
		}

		return OkMove(objs);
	}


	static ErrorOr<std::pair<std::vector<std::unique_ptr<interp::Stmt>>, bool>> parse_preamble(Lexer& lexer)
	{
		auto _ = LexerModer(lexer, Lexer::Mode::Script);

		bool saw_doc_start = false;
		std::vector<std::unique_ptr<interp::Stmt>> stmts {};

		while(not lexer.eof())
		{
			if(lexer.expect(TT::Backslash))
			{
				if(not lexer.expect(KW_START_DOCUMENT))
					return ErrMsg(lexer.location(), "expected '\\{}' in preamble (saw '\\')", KW_START_DOCUMENT);

				auto callee = interp::QualifiedId {
					.top_level = true,
					.parents = { "builtin" },
					.name = KW_START_DOCUMENT,
				};

				auto ident = std::make_unique<interp::Ident>(lexer.previous().loc);
				ident->name = std::move(callee);

				stmts.push_back(TRY(parse_function_call(lexer, std::move(ident))));
				saw_doc_start = true;

				if(lexer.expect(TT::Semicolon))
					;

				break;
			}
			else
			{
				stmts.push_back(TRY(parse_stmt(lexer, /* is_top_level: */ false)));
			}
		}

		using PP = std::pair<std::vector<std::unique_ptr<interp::Stmt>>, bool>;
		return Ok<PP>(std::move(stmts), saw_doc_start);
	}


	ErrorOr<Document> parse(zst::str_view filename, zst::str_view contents)
	{
		auto lexer = Lexer(filename, contents);

		auto [preamble, have_doc_start] = TRY(parse_preamble(lexer));
		auto document = Document(std::move(preamble), have_doc_start);

		while(not lexer.eof())
		{
			auto objs = TRY(parse_top_level(lexer));
			for(auto& obj : objs)
				document.addObject(std::move(obj));

			if(not lexer.eof())
				return ErrMsg(lexer.location(), "unexpected token '{}'", lexer.peek().text);
			else
				break;
		}

		return OkMove(document);
	}
}
