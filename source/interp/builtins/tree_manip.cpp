// tree_manip.cpp
// Copyright (c) 2022, zhiayang
// SPDX-License-Identifier: Apache-2.0

#include <memory> // for make_unique, unique_ptr

#include "defs.h" // for ErrorOr, Ok

#include "sap/style.h"   // for Style
#include "sap/fontset.h" // for FontStyle, FontStyle::Bold

#include "interp/tree.h"    // for Text, InlineObject
#include "interp/state.h"   // for Interpreter
#include "interp/value.h"   // for Value
#include "interp/builtin.h" // for bold1

namespace sap::interp::builtin
{
	static const Style g_bold_style = []() {
		Style style {};
		style.set_font_style(FontStyle::Bold);
		return style;
	}();

	static const Style g_italic_style = []() {
		Style style {};
		style.set_font_style(FontStyle::Italic);
		return style;
	}();

	static const Style g_bold_italic_style = []() {
		Style style {};
		style.set_font_style(FontStyle::BoldItalic);
		return style;
	}();


	static ErrorOr<std::optional<Value>> apply_style1(Interpreter* cs, std::vector<Value>& args, const Style& style)
	{
		// TODO: maybe don't assert?
		assert(args.size() == 1);

		auto& value = args[0];

		if(value.isTreeInlineObj())
		{
			auto tio = value.getTreeInlineObj();
			auto new_style = Style::combine(tio->style(), &style);
			value.getTreeInlineObj()->setStyle(new_style);

			return Ok(std::move(value));
		}
		else
		{
			auto word = std::make_unique<tree::Text>(value.toString());
			word->setStyle(&style);

			return Ok(Value::treeInlineObject(std::move(word)));
		}
	}


	static ErrorOr<std::optional<Value>> bold1(Interpreter* cs, std::vector<Value>& args)
	{
		return apply_style1(cs, args, g_bold_style);
	}

	static ErrorOr<std::optional<Value>> italic1(Interpreter* cs, std::vector<Value>& args)
	{
		return apply_style1(cs, args, g_italic_style);
	}

	static ErrorOr<std::optional<Value>> bold_italic1(Interpreter* cs, std::vector<Value>& args)
	{
		return apply_style1(cs, args, g_bold_italic_style);
	}
}


namespace sap::interp
{
	void defineBuiltins(DefnTree* builtin_ns)
	{
		using BFD = BuiltinFunctionDefn;
		using Param = FunctionDecl::Param;

		auto tio = Type::makeTreeInlineObj();
		auto num = Type::makeNumber();
		auto str = Type::makeString();

		builtin_ns->define(std::make_unique<BFD>("__bold1", Type::makeFunction({ tio }, tio),
		    makeParamList(Param { .name = "_", .type = tio }), &builtin::bold1));

		builtin_ns->define(std::make_unique<BFD>("__bold1", Type::makeFunction({ num }, tio),
		    makeParamList(Param { .name = "_", .type = num }), &builtin::bold1));

		builtin_ns->define(std::make_unique<BFD>("__bold1", Type::makeFunction({ str }, tio),
		    makeParamList(Param { .name = "_", .type = str }), &builtin::bold1));


		builtin_ns->define(std::make_unique<BFD>("__italic1", Type::makeFunction({ tio }, tio),
		    makeParamList(Param { .name = "_", .type = tio }), &builtin::italic1));

		builtin_ns->define(std::make_unique<BFD>("__italic1", Type::makeFunction({ num }, tio),
		    makeParamList(Param { .name = "_", .type = num }), &builtin::italic1));

		builtin_ns->define(std::make_unique<BFD>("__italic1", Type::makeFunction({ str }, tio),
		    makeParamList(Param { .name = "_", .type = str }), &builtin::italic1));


		builtin_ns->define(std::make_unique<BFD>("__bold_italic1", Type::makeFunction({ tio }, tio),
		    makeParamList(Param { .name = "_", .type = tio }), &builtin::bold_italic1));

		builtin_ns->define(std::make_unique<BFD>("__bold_italic1", Type::makeFunction({ num }, tio),
		    makeParamList(Param { .name = "_", .type = num }), &builtin::bold_italic1));

		builtin_ns->define(std::make_unique<BFD>("__bold_italic1", Type::makeFunction({ str }, tio),
		    makeParamList(Param { .name = "_", .type = str }), &builtin::bold_italic1));
	}
}
