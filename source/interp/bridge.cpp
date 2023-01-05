// bridge.cpp
// Copyright (c) 2022, zhiayang
// SPDX-License-Identifier: Apache-2.0

#include "sap/frontend.h"

#include "interp/ast.h"
#include "interp/typechecker.h"
#include "interp/builtin_fns.h"
#include "interp/builtin_types.h"

namespace sap::interp
{
	using PT = frontend::PType;
	using Field = StructDefn::Field;

	static auto pt_float = PT::named(frontend::TYPE_FLOAT);

	std::vector<Field> builtin::BStyle::fields()
	{
		return util::vectorOf(                                             //
		    Field { .name = "font_size", .type = PT::optional(pt_float) }, //
		    Field { .name = "line_spacing", .type = PT::optional(pt_float) });
	}


	template <typename T>
	void define_builtin_struct(Typechecker* ts)
	{
		auto str = std::make_unique<StructDefn>(T::name, T::fields());
		ts->addBuiltinDefinition(std::move(str))->typecheck(ts);
	}


	static void define_builtin_types(Typechecker* ts, DefnTree* builtin_ns)
	{
		auto _ = ts->pushTree(builtin_ns);
		define_builtin_struct<builtin::BStyle>(ts);
	}



	static void define_builtin_funcs(Typechecker* ts, DefnTree* builtin_ns)
	{
		using namespace sap::frontend;

		using BFD = BuiltinFunctionDefn;
		using Param = FunctionDecl::Param;

		auto any = PType::named(TYPE_ANY);
		auto tio = PType::named(TYPE_TREE_INLINE);

		auto bstyle_t = PType::named(builtin::BStyle::name);

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
		define_builtin_types(ts, ns);
		define_builtin_funcs(ts, ns);
	}
}
