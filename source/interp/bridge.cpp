// bridge.cpp
// Copyright (c) 2022, zhiayang
// SPDX-License-Identifier: Apache-2.0

#include "sap/frontend.h"

#include "interp/ast.h"
#include "interp/bridge.h"
#include "interp/builtin.h"
#include "interp/typechecker.h"

namespace sap::interp
{
	std::vector<StructDefn::Field> bridge::getFieldsForBridgedType(Typechecker* ts, zst::str_view name)
	{
		using namespace frontend;

		using F = StructDefn::Field;
		using PT = frontend::PType;

		auto pt_float = PT::named(TYPE_FLOAT);

		if(name == bridge::STYLE)
		{
			return util::vectorOf(                                    //
			    F { .name = STYLE_F0_FONT_SIZE, .type = pt_float },   //
			    F { .name = STYLE_F1_LINE_SPACING, .type = pt_float } //
			);
		}
		else
		{
			error("interp", "unknown bridged type '{}'", name);
		}
	}

	static void define_builtin_funcs(Typechecker* ts, DefnTree* builtin_ns)
	{
		using namespace sap::frontend;

		using BFD = BuiltinFunctionDefn;
		using Param = FunctionDecl::Param;

		auto any = PType::named(TYPE_ANY);
		auto tio = PType::named(TYPE_TREE_INLINE);

		auto bstyle_t = PType::named(bridge::STYLE);

		auto define_builtin = [&](auto&&... xs) {
			auto ret = std::make_unique<BFD>(std::forward<decltype(xs)>(xs)...);
			ts->addBuiltinDefinition(std::move(ret))->typecheck(ts);
		};

		auto _ = ts->pushTree(builtin_ns);

		define_builtin("bold1", makeParamList(Param { .name = "_", .type = any }), tio, &builtin::bold1);
		define_builtin("italic1", makeParamList(Param { .name = "_", .type = any }), tio, &builtin::italic1);
		define_builtin("bold_italic1", makeParamList(Param { .name = "_", .type = any }), tio, &builtin::bold_italic1);

		define_builtin("apply_style",
		    makeParamList(                               //
		        Param { .name = "1", .type = bstyle_t }, //
		        Param { .name = "2", .type = tio }),
		    tio, &builtin::apply_style);


		// TODO: make these variadic
		define_builtin("print", makeParamList(Param { .name = "_", .type = any }), tio, &builtin::print);
		define_builtin("println", makeParamList(Param { .name = "_", .type = any }), tio, &builtin::println);
	}

	void defineBuiltins(Typechecker* ts, DefnTree* ns)
	{
		define_builtin_funcs(ts, ns);
	}
}
