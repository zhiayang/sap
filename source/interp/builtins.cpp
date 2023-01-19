// builtins.cpp
// Copyright (c) 2022, zhiayang
// SPDX-License-Identifier: Apache-2.0

#include "sap/style.h"
#include "sap/frontend.h"

#include "interp/ast.h"
#include "interp/interp.h"
#include "interp/builtin_fns.h"
#include "interp/builtin_types.h"

namespace sap::interp
{
	template <typename T>
	void define_builtin_struct(Interpreter* interp)
	{
		auto s = std::make_unique<StructDefn>(Location::builtin(), T::name, T::fields());
		auto defn = s.get();

		T::type = interp->typechecker().addBuiltinDefinition(std::move(s))->typecheck(&interp->typechecker()).unwrap().type();
		defn->evaluate(&interp->evaluator());
	}

	template <typename T>
	void define_builtin_enum(Interpreter* interp)
	{
		auto e = std::make_unique<EnumDefn>(Location::builtin(), T::name, T::enumeratorType(), T::enumerators());
		auto defn = e.get();

		T::type = interp->typechecker().addBuiltinDefinition(std::move(e))->typecheck(&interp->typechecker()).unwrap().type();
		defn->evaluate(&interp->evaluator());
	}

	static void define_builtin_types(Interpreter* interp, DefnTree* builtin_ns)
	{
		auto _ = interp->typechecker().pushTree(builtin_ns);

		define_builtin_enum<builtin::BE_Alignment>(interp);

		// this needs a careful ordering
		define_builtin_struct<builtin::BS_Font>(interp);
		define_builtin_struct<builtin::BS_FontFamily>(interp);
		define_builtin_struct<builtin::BS_Style>(interp);
	}



	static void define_builtin_funcs(Interpreter* interp, DefnTree* builtin_ns)
	{
		auto ts = &interp->typechecker();

		using namespace sap::frontend;

		using BFD = BuiltinFunctionDefn;
		using Param = FunctionDecl::Param;

		auto t_any = PType::named(TYPE_ANY);
		auto t_int = PType::named(TYPE_INT);
		auto t_str = PType::named(TYPE_STRING);
		auto t_bool = PType::named(TYPE_BOOL);
		auto t_char = PType::named(TYPE_CHAR);
		auto t_void = PType::named(TYPE_VOID);
		auto t_float = PType::named(TYPE_FLOAT);
		auto t_length = PType::named(TYPE_LENGTH);

		auto t_tbo = PType::named(TYPE_TREE_BLOCK);
		auto t_tio = PType::named(TYPE_TREE_INLINE);

		auto make_builtin_name = [](const char* name) -> QualifiedId {
			return QualifiedId {
				.top_level = true,
				.parents = { "builtin" },
				.name = name,
			};
		};

		auto make_null = []() {
			return std::make_unique<interp::NullLit>(Location::builtin());
		};


		auto t_bfont = PType::named(make_builtin_name(builtin::BS_Font::name));
		auto t_bstyle = PType::named(make_builtin_name(builtin::BS_Style::name));
		auto t_bfontfamily = PType::named(make_builtin_name(builtin::BS_FontFamily::name));

		auto define_builtin = [&](auto&&... xs) {
			auto ret = std::make_unique<BFD>(Location::builtin(), std::forward<decltype(xs)>(xs)...);
			ts->addBuiltinDefinition(std::move(ret))->typecheck(ts);
		};

		auto _ = ts->pushTree(builtin_ns);

		define_builtin("bold1", makeParamList(Param { .name = "_", .type = t_any }), t_tio, &builtin::bold1);
		define_builtin("italic1", makeParamList(Param { .name = "_", .type = t_any }), t_tio, &builtin::italic1);
		define_builtin("bold_italic1", makeParamList(Param { .name = "_", .type = t_any }), t_tio, &builtin::bold_italic1);

		define_builtin("make_text", makeParamList(Param { .name = "1", .type = PType::array(t_str, false) }), t_tio,
		    &builtin::make_text);

		define_builtin("make_paragraph", makeParamList(Param { .name = "1", .type = PType::array(t_tio, false) }), t_tbo,
		    &builtin::make_paragraph);


		define_builtin("apply_style",
		    makeParamList(                               //
		        Param { .name = "1", .type = t_bstyle }, //
		        Param { .name = "2", .type = t_tio }),
		    t_tio, &builtin::apply_style_tio);

		define_builtin("apply_style",
		    makeParamList(                               //
		        Param { .name = "1", .type = t_bstyle }, //
		        Param { .name = "2", .type = t_tbo }),
		    t_tbo, &builtin::apply_style_tbo);

		define_builtin("load_image",
		    makeParamList(                               //
		        Param { .name = "1", .type = t_str },    //
		        Param { .name = "2", .type = t_length }, //
		        Param {
		            .name = "3",
		            .type = PType::optional(t_length),
		            .default_value = make_null(),
		        }),
		    t_tbo, &builtin::load_image);

		define_builtin("include", makeParamList(Param { .name = "1", .type = t_str }), t_tbo, &builtin::include_file);

		define_builtin("push_style",
		    makeParamList( //
		        Param { .name = "1", .type = t_bstyle }),
		    t_void, &builtin::push_style);

		define_builtin("pop_style", makeParamList(), t_bstyle, &builtin::pop_style);
		define_builtin("current_style", makeParamList(), t_bstyle, &builtin::current_style);

		define_builtin("print", makeParamList(Param { .name = "_", .type = t_any }), t_void, &builtin::print);
		define_builtin("println", makeParamList(Param { .name = "_", .type = t_any }), t_void, &builtin::println);

		define_builtin("to_string", makeParamList(Param { .name = "_", .type = t_any }), t_str, &builtin::to_string);

		define_builtin("find_font",
		    makeParamList(                                                                                    //
		        Param { .name = "names", .type = PType::array(t_str, false) },                                //
		        Param { .name = "weight", .type = PType::optional(t_int), .default_value = make_null() },     //
		        Param { .name = "italic", .type = PType::optional(t_bool), .default_value = make_null() },    //
		        Param { .name = "stretch", .type = PType::optional(t_float), .default_value = make_null() }), //
		    PType::optional(t_bfont), &builtin::find_font);

		define_builtin("find_font_family",
		    makeParamList( //
		        Param { .name = "names", .type = PType::array(t_str, false) }),
		    PType::optional(t_bfontfamily), &builtin::find_font_family);
	}

	void defineBuiltins(Interpreter* interp, DefnTree* ns)
	{
		define_builtin_types(interp, ns);
		define_builtin_funcs(interp, ns);
	}
}
