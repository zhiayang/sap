// builtins.cpp
// Copyright (c) 2022, zhiayang
// SPDX-License-Identifier: Apache-2.0

#include "sap/style.h"
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

	static auto get_null()
	{
		return std::make_unique<NullLit>();
	}

	template <typename T>
	static std::optional<T> get_field(const Value& str, zst::str_view field)
	{
		auto& fields = str.getStructFields();
		auto idx = str.type()->toStruct()->getFieldIndex(field);

		auto& f = fields[idx];
		if(not f.haveOptionalValue())
			return std::nullopt;

		return (*f.getOptional())->get<T>();
	}

	const Type* builtin::BStyle::type = nullptr;
	std::vector<Field> builtin::BStyle::fields()
	{
		return util::vectorOf(                                                                        //
		    Field { .name = "font_size", .type = PT::optional(pt_float), .initialiser = get_null() }, //
		    Field { .name = "line_spacing", .type = PT::optional(pt_float), .initialiser = get_null() });
	}

	Value builtin::BStyle::make(const Style* style)
	{
		return StructMaker(BStyle::type->toStruct()) //
		    .set("font_size", Value::floating(style->font_size().value()))
		    .set("line_spacing", Value::floating(style->line_spacing()))
		    .make();
	}

	const Style* builtin::BStyle::unmake(const Value& value)
	{
		auto style = util::make<Style>();

		auto get_scalar = [&value](zst::str_view field) -> std::optional<sap::Length> {
			auto f = get_field<double>(value, field);
			if(f.has_value())
				return sap::Length(*f);

			return std::nullopt;
		};

		style->set_font_size(get_scalar("font_size"));
		style->set_line_spacing(get_field<double>(value, "line_spacing"));

		return style;
	}







	template <typename T>
	void define_builtin_struct(Typechecker* ts)
	{
		auto str = std::make_unique<StructDefn>(T::name, T::fields());
		T::type = ts->addBuiltinDefinition(std::move(str))->typecheck(ts).unwrap().type();
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

		auto t_any = PType::named(TYPE_ANY);
		auto t_str = PType::named(TYPE_STRING);
		auto t_void = PType::named(TYPE_VOID);
		auto t_float = PType::named(TYPE_FLOAT);
		auto t_length = PType::named(TYPE_LENGTH);

		auto t_tbo = PType::named(TYPE_TREE_BLOCK);
		auto t_tio = PType::named(TYPE_TREE_INLINE);

		auto t_bstyle = PType::named(builtin::BStyle::name);

		auto define_builtin = [&](auto&&... xs) {
			auto ret = std::make_unique<BFD>(std::forward<decltype(xs)>(xs)...);
			ts->addBuiltinDefinition(std::move(ret))->typecheck(ts);
		};

		auto _ = ts->pushTree(builtin_ns);

		define_builtin("bold1", makeParamList(Param { .name = "_", .type = t_any }), t_tio, &builtin::bold1);
		define_builtin("italic1", makeParamList(Param { .name = "_", .type = t_any }), t_tio, &builtin::italic1);
		define_builtin("bold_italic1", makeParamList(Param { .name = "_", .type = t_any }), t_tio, &builtin::bold_italic1);

		define_builtin("apply_style",
		    makeParamList(                               //
		        Param { .name = "1", .type = t_bstyle }, //
		        Param { .name = "2", .type = t_tio }),
		    t_tio, &builtin::apply_style);

		define_builtin("load_image",
		    makeParamList(                               //
		        Param { .name = "1", .type = t_str },    //
		        Param { .name = "2", .type = t_length }, //
		        Param { .name = "3", .type = PType::optional(t_length), .default_value = std::make_unique<interp::NullLit>() }),
		    t_tbo, &builtin::load_image);

		define_builtin("push_style",
		    makeParamList( //
		        Param { .name = "1", .type = t_bstyle }),
		    t_void, &builtin::push_style);

		define_builtin("pop_style", makeParamList(), t_bstyle, &builtin::pop_style);
		define_builtin("current_style", makeParamList(), t_bstyle, &builtin::current_style);

		define_builtin("centre", makeParamList(Param { .name = "1", .type = t_tbo }), t_tbo, &builtin::centred_block);

		// TODO: make these variadic
		define_builtin("print", makeParamList(Param { .name = "_", .type = t_any }), t_void, &builtin::print);
		define_builtin("println", makeParamList(Param { .name = "_", .type = t_any }), t_void, &builtin::println);
	}

	void defineBuiltins(Typechecker* ts, DefnTree* ns)
	{
		define_builtin_types(ts, ns);
		define_builtin_funcs(ts, ns);
	}
}
