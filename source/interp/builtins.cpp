// builtins.cpp
// Copyright (c) 2022, zhiayang
// SPDX-License-Identifier: Apache-2.0

#include "sap/style.h"
#include "sap/frontend.h"

#include "layout/base.h"

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

		T::type = interp->typechecker()
		              .addBuiltinDefinition(std::move(s))
		              ->typecheck(&interp->typechecker())
		              .unwrap()
		              .type();
		defn->evaluate(&interp->evaluator()).expect("builtin decl failed");
	}

	template <typename T>
	void define_builtin_enum(Interpreter* interp)
	{
		auto e = std::make_unique<EnumDefn>(Location::builtin(), T::name, T::enumeratorType(), T::enumerators());
		auto defn = e.get();

		T::type = interp->typechecker()
		              .addBuiltinDefinition(std::move(e))
		              ->typecheck(&interp->typechecker())
		              .unwrap()
		              .type();
		defn->evaluate(&interp->evaluator()).expect("builtin decl failed");
	}

	static void define_builtin_types(Interpreter* interp, DefnTree* builtin_ns)
	{
		auto _ = interp->typechecker().pushTree(builtin_ns);

		define_builtin_enum<builtin::BE_Alignment>(interp);

		// this needs a careful ordering
		define_builtin_struct<builtin::BS_Size2d>(interp);
		define_builtin_struct<builtin::BS_Position>(interp);
		define_builtin_struct<builtin::BS_AbsPosition>(interp);
		define_builtin_struct<builtin::BS_Font>(interp);
		define_builtin_struct<builtin::BS_FontFamily>(interp);
		define_builtin_struct<builtin::BS_Style>(interp);

		define_builtin_struct<builtin::BS_DocumentMargins>(interp);
		define_builtin_struct<builtin::BS_DocumentSettings>(interp);

		define_builtin_struct<builtin::BS_State>(interp);
	}



	static void define_builtin_funcs(Interpreter* interp, DefnTree* builtin_ns)
	{
		auto ts = &interp->typechecker();

		using namespace sap::frontend;

		using BFD = BuiltinFunctionDefn;
		using Param = FunctionDecl::Param;

		const auto t_any = PType::named(TYPE_ANY);
		const auto t_int = PType::named(TYPE_INT);
		const auto t_str = PType::named(TYPE_STRING);
		const auto t_bool = PType::named(TYPE_BOOL);
		const auto t_char = PType::named(TYPE_CHAR);
		const auto t_void = PType::named(TYPE_VOID);
		const auto t_float = PType::named(TYPE_FLOAT);
		const auto t_length = PType::named(TYPE_LENGTH);

		const auto t_tbo = PType::named(TYPE_TREE_BLOCK);
		const auto t_tio = PType::named(TYPE_TREE_INLINE);
		const auto t_lo = PType::named(TYPE_LAYOUT_OBJECT);

		const auto t_tbo_ref = PType::named(TYPE_TREE_BLOCK_REF);
		const auto t_tio_ref = PType::named(TYPE_TREE_INLINE_REF);
		const auto t_lo_ref = PType::named(TYPE_LAYOUT_OBJECT_REF);

		const auto make_builtin_name = [](const char* name) -> QualifiedId {
			return QualifiedId {
				.top_level = true,
				.parents = { "builtin" },
				.name = name,
			};
		};

		const auto make_null = []() { return std::make_unique<interp::NullLit>(Location::builtin()); };
		const auto make_bool = [](bool x) { return std::make_unique<interp::BooleanLit>(Location::builtin(), x); };

		const auto t_ptr = [](const PType& t, bool mut = false) { return PType::pointer(t, mut); };

		const auto t_opt = [](const PType& t) { return PType::optional(t); };

		const auto t_bfont = PType::named(make_builtin_name(builtin::BS_Font::name));
		const auto t_bstate = PType::named(make_builtin_name(builtin::BS_State::name));
		const auto t_bstyle = PType::named(make_builtin_name(builtin::BS_Style::name));
		const auto t_bsize2d = PType::named(make_builtin_name(builtin::BS_Size2d::name));
		const auto t_bposition = PType::named(make_builtin_name(builtin::BS_Position::name));
		const auto t_bfontfamily = PType::named(make_builtin_name(builtin::BS_FontFamily::name));
		const auto t_babsposition = PType::named(make_builtin_name(builtin::BS_AbsPosition::name));
		const auto t_bdocsettings = PType::named(make_builtin_name(builtin::BS_DocumentSettings::name));

		const auto define_builtin = [&](auto&&... xs) {
			auto ret = std::make_unique<BFD>(Location::builtin(), std::forward<decltype(xs)>(xs)...);
			ts->addBuiltinDefinition(std::move(ret))->typecheck(ts).expect("builtin decl failed");
		};

		const auto P = [](const char* name, const PType& t, std::unique_ptr<Expr> default_val = nullptr) {
			return Param { .name = name, .type = t, .default_value = std::move(default_val) };
		};

		namespace B = builtin;

		// TODO: this is a huge hack!!!!!!!
		// since we don't have generics, we make this accept 'any'...
		define_builtin("size", makeParamList(P("_", t_any)), t_int,
			[](Evaluator* ev, std::vector<Value>& args) -> ErrorOr<EvalResult> {
				assert(args.size() == 1);
				if(not args[0].isPointer() || !args[0].type()->pointerElement()->isArray())
					return ErrMsg(ev, ".size() can only be used on arrays (got {})", args[0].type());

				return EvalResult::ofValue(Value::integer(checked_cast<
					int64_t>(args[0].getPointer()->getArray().size())));
			});


		auto _ = ts->pushTree(builtin_ns);

		define_builtin("start_document",
			makeParamList(Param {
				.name = "_",
				.type = PType::optional(t_bdocsettings),
				.default_value = make_null(),
			}),
			t_void, &builtin::start_document);

		define_builtin("state", makeParamList(), t_bstate, [](auto* ev, auto& args) {
			assert(args.size() == 0);
			return EvalResult::ofValue(builtin::BS_State::make(ev, ev->state()));
		});

		define_builtin("ref", makeParamList(P("_", t_ptr(t_tio))), t_tio_ref, &B::ref_object);
		define_builtin("ref", makeParamList(P("_", t_ptr(t_tbo))), t_tbo_ref, &B::ref_object);
		define_builtin("ref", makeParamList(P("_", t_ptr(t_lo))), t_lo_ref, &B::ref_object);

		define_builtin("vspace", makeParamList(P("_", t_length)), t_tbo, &B::vspace);
		define_builtin("hspace", makeParamList(P("_", t_length)), t_tbo, &B::hspace);

		define_builtin("layout_object", makeParamList(P("_", t_ptr(t_tbo))), t_opt(t_lo_ref), &B::get_layout_object);
		define_builtin("layout_object", makeParamList(P("_", t_ptr(t_tbo_ref))), t_opt(t_lo_ref),
			&B::get_layout_object);

		define_builtin("position", makeParamList(P("_", t_ptr(t_lo_ref))), t_babsposition,
			&B::get_layout_object_position);

		define_builtin("set_size", makeParamList(P("_", t_ptr(t_tbo)), P("size", t_bsize2d)), t_void, &B::set_tbo_size);
		define_builtin("set_size", makeParamList(P("_", t_ptr(t_tbo_ref)), P("size", t_bsize2d)), t_void,
			&B::set_tbo_size);

		define_builtin("set_size_x", makeParamList(P("_", t_ptr(t_tbo)), P("size", t_length)), t_void,
			&B::set_tbo_size_x);
		define_builtin("set_size_x", makeParamList(P("_", t_ptr(t_tbo_ref)), P("size", t_length)), t_void,
			&B::set_tbo_size_x);

		define_builtin("set_size_y", makeParamList(P("_", t_ptr(t_tbo)), P("size", t_length)), t_void,
			&B::set_tbo_size_y);
		define_builtin("set_size_y", makeParamList(P("_", t_ptr(t_tbo_ref)), P("size", t_length)), t_void,
			&B::set_tbo_size_y);



		define_builtin("offset_position", makeParamList(P("_", t_ptr(t_tbo)), P("offset", t_bsize2d)), t_void,
			&B::offset_object_position);
		define_builtin("offset_position", makeParamList(P("_", t_ptr(t_tbo_ref)), P("offset", t_bsize2d)), t_void,
			&B::offset_object_position);
		define_builtin("offset_position", makeParamList(P("_", t_ptr(t_lo_ref)), P("offset", t_bsize2d)), t_void,
			&B::offset_object_position);

		define_builtin("override_position", makeParamList(P("_", t_ptr(t_tbo)), P("pos", t_babsposition)), t_void,
			&B::override_object_position);
		define_builtin("override_position", makeParamList(P("_", t_ptr(t_tbo_ref)), P("pos", t_babsposition)), t_void,
			&B::override_object_position);
		define_builtin("override_position", makeParamList(P("_", t_ptr(t_lo_ref)), P("pos", t_babsposition)), t_void,
			&B::override_object_position);



		define_builtin("bold1", makeParamList(P("_", t_any)), t_tio, &B::bold1);
		define_builtin("italic1", makeParamList(P("_", t_any)), t_tio, &B::italic1);
		define_builtin("bold_italic1", makeParamList(P("_", t_any)), t_tio, &B::bold_italic1);

		define_builtin("request_layout", makeParamList(), t_void, &B::request_layout);

		define_builtin("make_hbox", makeParamList(P("1", PType::variadicArray(t_tbo))), t_tbo, &B::make_hbox);
		define_builtin("make_zbox", makeParamList(P("1", PType::variadicArray(t_tbo))), t_tbo, &B::make_zbox);
		define_builtin("make_vbox",
			makeParamList(P("1", PType::variadicArray(t_tbo)), P("glue", t_bool, make_bool(false))), t_tbo,
			&B::make_vbox);

		define_builtin("make_text", makeParamList(P("1", PType::variadicArray(t_str))), t_tio, &B::make_text);
		define_builtin("make_line", makeParamList(P("1", PType::variadicArray(t_tio))), t_tbo, &B::make_line);
		define_builtin("make_paragraph", makeParamList(P("1", PType::variadicArray(t_tio))), t_tbo, &B::make_paragraph);

		define_builtin("apply_style", makeParamList(P("1", t_bstyle), P("2", t_tio)), t_tio, &B::apply_style_tio);
		define_builtin("apply_style", makeParamList(P("1", t_bstyle), P("2", t_tbo)), t_tbo, &B::apply_style_tbo);

		define_builtin("load_image",
			makeParamList(P("1", t_str), P("2", t_length), P("3", t_opt(t_length), make_null())), t_tbo,
			&B::load_image);

		define_builtin("include", makeParamList(P("1", t_str)), t_tbo, &B::include_file);

		define_builtin("push_style", makeParamList(P("1", t_bstyle)), t_void, &B::push_style);

		define_builtin("pop_style", makeParamList(), t_bstyle, &B::pop_style);
		define_builtin("current_style", makeParamList(), t_bstyle, &B::current_style);

		define_builtin("output_at_absolute", makeParamList(P("pos", t_babsposition), P("obj", t_tbo)), t_void,
			&B::output_at_absolute_pos_tbo);


		define_builtin("print", makeParamList(P("_", t_any)), t_void, &B::print);
		define_builtin("println", makeParamList(P("_", t_any)), t_void, &B::println);

		define_builtin("to_string", makeParamList(P("_", t_any)), t_str, &B::to_string);

		define_builtin("find_font",
			makeParamList(                                  //
				P("names", PType::array(t_str)),            //
				P("weight", t_opt(t_int), make_null()),     //
				P("italic", t_opt(t_bool), make_null()),    //
				P("stretch", t_opt(t_float), make_null())), //
			t_opt(t_bfont), &B::find_font);

		define_builtin("find_font_family", makeParamList(P("names", PType::array(t_str))), t_opt(t_bfontfamily),
			&B::find_font_family);
	}

	void defineBuiltins(Interpreter* interp, DefnTree* ns)
	{
		define_builtin_types(interp, ns);
		define_builtin_funcs(interp, ns);
	}
}
