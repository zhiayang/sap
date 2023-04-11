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
		define_builtin_struct<builtin::BS_OutlineItem>(interp);
		define_builtin_struct<builtin::BS_LinkAnnotation>(interp);

		define_builtin_struct<builtin::BS_DocumentProxy>(interp);
	}



	template <std::same_as<FunctionDecl::Param>... P>
	static std::vector<FunctionDecl::Param> PL(P&&... params)
	{
		std::vector<FunctionDecl::Param> ret {};
		(ret.push_back(std::move(params)), ...);

		return ret;
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

		const auto T_P = [](const PType& t) { return PType::pointer(t, false); };
		const auto T_MP = [](const PType& t) { return PType::pointer(t, true); };

		const auto T_O = [](const PType& t) { return PType::optional(t); };
		const auto T_VARR = [](const PType& t) { return PType::variadicArray(t); };

		const auto t_bfont = PType::named(make_builtin_name(builtin::BS_Font::name));
		const auto t_bstate = PType::named(make_builtin_name(builtin::BS_State::name));
		const auto t_bstyle = PType::named(make_builtin_name(builtin::BS_Style::name));
		const auto t_bsize2d = PType::named(make_builtin_name(builtin::BS_Size2d::name));
		const auto t_bposition = PType::named(make_builtin_name(builtin::BS_Position::name));
		const auto t_bfontfamily = PType::named(make_builtin_name(builtin::BS_FontFamily::name));
		const auto t_abspos = PType::named(make_builtin_name(builtin::BS_AbsPosition::name));
		const auto t_bdocsettings = PType::named(make_builtin_name(builtin::BS_DocumentSettings::name));
		const auto t_bdocproxy = PType::named(make_builtin_name(builtin::BS_DocumentProxy::name));
		const auto t_blinkannot = PType::named(make_builtin_name(builtin::BS_LinkAnnotation::name));

		const auto DEF = [&](auto&&... xs) {
			auto ret = std::make_unique<BFD>(Location::builtin(), std::forward<decltype(xs)>(xs)...);
			ts->addBuiltinDefinition(std::move(ret))->typecheck(ts).expect("builtin decl failed");
		};

		const auto P = [](const char* name, const PType& t, std::unique_ptr<Expr> default_val = nullptr) {
			return Param { .name = name, .type = t, .default_value = std::move(default_val) };
		};

		namespace B = builtin;

		// FIXME: make this not take &[void] when we get generics
		DEF("size", PL(P("_", T_P(PType::array(t_void)))), t_int,
		    [](Evaluator* ev, std::vector<Value>& args) -> ErrorOr<EvalResult> {
			    assert(args.size() == 1);
			    if(not args[0].isPointer() || !args[0].type()->pointerElement()->isArray())
				    return ErrMsg(ev, ".size() can only be used on arrays (got {})", args[0].type());

			    return EvalResult::ofValue(Value::integer(checked_cast<
			        int64_t>(args[0].getPointer()->getArray().size())));
		    });

		auto _ = ts->pushTree(builtin_ns);

		DEF("start_document",
		    PL(Param {
		        .name = "_",
		        .type = PType::optional(t_bdocsettings),
		        .default_value = make_null(),
		    }),
		    t_void, &builtin::start_document);

		DEF("state", PL(), t_bstate, [](auto* ev, auto& args) {
			assert(args.size() == 0);
			return EvalResult::ofValue(builtin::BS_State::make(ev, ev->state()));
		});

		DEF("document", PL(), T_MP(t_bdocproxy), [](auto* ev, auto& args) {
			return EvalResult::ofValue(Value::mutablePointer(builtin::BS_DocumentProxy::type, &ev->documentProxy()));
		});

		DEF("to_string", PL(P("_", t_any)), t_str, &B::to_string);

		DEF("vspace", PL(P("_", t_length)), t_tbo, &B::vspace);
		DEF("hspace", PL(P("_", t_length)), t_tbo, &B::hspace);
		DEF("page_break", PL(), t_tbo, &B::page_break);


		DEF("bold1", PL(P("_", t_any)), t_tio, &B::bold1);
		DEF("italic1", PL(P("_", t_any)), t_tio, &B::italic1);
		DEF("bold_italic1", PL(P("_", t_any)), t_tio, &B::bold_italic1);

		DEF("request_layout", PL(), t_void, &B::request_layout);

		DEF("make_hbox", PL(P("1", T_VARR(t_tbo))), t_tbo, &B::make_hbox);
		DEF("make_zbox", PL(P("1", T_VARR(t_tbo))), t_tbo, &B::make_zbox);
		DEF("make_vbox", PL(P("1", T_VARR(t_tbo)), P("glue", t_bool, make_bool(false))), t_tbo, &B::make_vbox);

		DEF("make_span", PL(P("1", T_VARR(t_tio)), P("glue", t_bool, make_bool(false))), t_tio, &B::make_span);
		DEF("make_text", PL(P("1", T_VARR(t_str)), P("glue", t_bool, make_bool(false))), t_tio, &B::make_text);
		DEF("make_line", PL(P("1", T_VARR(t_tio))), t_tbo, &B::make_line);
		DEF("make_paragraph", PL(P("1", T_VARR(t_tio))), t_tbo, &B::make_paragraph);

		DEF("apply_style", PL(P("obj", t_tio), P("style", t_bstyle)), t_tio, &B::apply_style_tio);
		DEF("apply_style", PL(P("obj", t_tbo), P("style", t_bstyle)), t_tbo, &B::apply_style_tbo);

		DEF("apply_style", PL(P("style", t_bstyle), P("obj", t_tio)), t_tio, &B::apply_style_tio);
		DEF("apply_style", PL(P("style", t_bstyle), P("obj", t_tbo)), t_tbo, &B::apply_style_tbo);

		DEF("load_image", PL(P("1", t_str), P("2", t_length), P("3", T_O(t_length), make_null())), t_tbo,
		    &B::load_image);

		DEF("layout_object", PL(P("_", T_P(t_tbo))), T_O(t_lo_ref), &B::get_layout_object);
		DEF("layout_object", PL(P("_", T_P(t_tbo_ref))), T_O(t_lo_ref), &B::get_layout_object);

		DEF("position", PL(P("_", T_P(t_lo_ref))), t_abspos, &B::get_layout_object_position);

		DEF("size", PL(P("_", T_P(t_lo_ref))), t_bsize2d, &B::get_layout_object_size);

		DEF("set_size", PL(P("_", T_P(t_tbo)), P("size", t_bsize2d)), t_void, &B::set_tbo_size);
		DEF("set_size", PL(P("_", T_P(t_tbo_ref)), P("size", t_bsize2d)), t_void, &B::set_tbo_size);

		DEF("set_width", PL(P("_", T_P(t_tbo)), P("width", t_length)), t_void, &B::set_tbo_width);
		DEF("set_width", PL(P("_", T_P(t_tbo_ref)), P("width", t_length)), t_void, &B::set_tbo_width);

		DEF("set_height", PL(P("_", T_P(t_tbo)), P("height", t_length)), t_void, &B::set_tbo_height);
		DEF("set_height", PL(P("_", T_P(t_tbo_ref)), P("height", t_length)), t_void, &B::set_tbo_height);

		DEF("set_width", PL(P("_", T_MP(t_tio)), P("width", t_length)), T_MP(t_tio), &B::set_tio_width);
		DEF("raise", PL(P("_", T_MP(t_tio)), P("width", t_length)), T_MP(t_tio), &B::raise_tio);

		DEF("set_width", PL(P("_", t_tio), P("width", t_length)), t_tio, &B::set_tio_width);
		DEF("raise", PL(P("_", t_tio), P("width", t_length)), t_tio, &B::raise_tio);


		DEF("link_to", PL(P("_", T_MP(t_tbo)), P("pos", t_abspos)), t_void, &B::set_tbo_link_annotation);
		DEF("link_to", PL(P("_", T_MP(t_tbo)), P("obj", t_tbo_ref)), t_void, &B::set_tbo_link_annotation);
		DEF("link_to", PL(P("_", T_MP(t_tbo_ref)), P("pos", t_abspos)), t_void, &B::set_tbo_link_annotation);
		DEF("link_to", PL(P("_", T_MP(t_tbo_ref)), P("obj", t_tbo_ref)), t_void, &B::set_tbo_link_annotation);

		DEF("link_to", PL(P("_", T_MP(t_lo)), P("obj", t_abspos)), t_void, &B::set_lo_link_annotation);
		DEF("link_to", PL(P("_", T_MP(t_lo)), P("obj", t_tbo_ref)), t_void, &B::set_lo_link_annotation);
		DEF("link_to", PL(P("_", T_MP(t_lo_ref)), P("obj", t_abspos)), t_void, &B::set_lo_link_annotation);
		DEF("link_to", PL(P("_", T_MP(t_lo_ref)), P("obj", t_tbo_ref)), t_void, &B::set_lo_link_annotation);

		DEF("link_to", PL(P("_", T_MP(t_tio)), P("obj", t_abspos)), t_void, &B::set_tio_link_annotation);
		DEF("link_to", PL(P("_", T_MP(t_tio)), P("obj", t_tbo_ref)), t_void, &B::set_tio_link_annotation);
		DEF("link_to", PL(P("_", T_MP(t_tio_ref)), P("obj", t_abspos)), t_void, &B::set_tio_link_annotation);
		DEF("link_to", PL(P("_", T_MP(t_tio_ref)), P("obj", t_tbo_ref)), t_void, &B::set_tio_link_annotation);



		DEF("offset_position", PL(P("_", T_P(t_tbo)), P("offset", t_bsize2d)), t_void, &B::offset_object_position);
		DEF("offset_position", PL(P("_", T_P(t_tbo_ref)), P("offset", t_bsize2d)), t_void, &B::offset_object_position);
		DEF("offset_position", PL(P("_", T_P(t_lo_ref)), P("offset", t_bsize2d)), t_void, &B::offset_object_position);

		DEF("override_position", PL(P("_", T_P(t_tbo)), P("pos", t_abspos)), t_void, &B::override_object_position);
		DEF("override_position", PL(P("_", T_P(t_tbo_ref)), P("pos", t_abspos)), t_void, &B::override_object_position);
		DEF("override_position", PL(P("_", T_P(t_lo_ref)), P("pos", t_abspos)), t_void, &B::override_object_position);

		DEF("ref", PL(P("_", T_P(t_tio))), t_tio_ref, &B::ref_object);
		DEF("ref", PL(P("_", T_P(t_tbo))), t_tbo_ref, &B::ref_object);
		DEF("ref", PL(P("_", T_P(t_lo))), t_lo_ref, &B::ref_object);

		DEF("include", PL(P("1", t_str)), t_tbo, &B::include_file);

		DEF("push_style", PL(P("1", t_bstyle)), t_void, &B::push_style);

		DEF("pop_style", PL(), t_bstyle, &B::pop_style);
		DEF("current_style", PL(), t_bstyle, &B::current_style);

		DEF("output_at_absolute", PL(P("pos", t_abspos), P("obj", t_tbo)), t_void, &B::output_at_absolute_pos_tbo);


		DEF("print", PL(P("_", t_any)), t_void, &B::print);
		DEF("println", PL(P("_", t_any)), t_void, &B::println);

		DEF("find_font",
		    PL(                                           //
		        P("names", PType::array(t_str)),          //
		        P("weight", T_O(t_int), make_null()),     //
		        P("italic", T_O(t_bool), make_null()),    //
		        P("stretch", T_O(t_float), make_null())), //
		    T_O(t_bfont), &B::find_font);

		DEF("find_font_family", PL(P("names", PType::array(t_str))), T_O(t_bfontfamily), &B::find_font_family);
	}

	void defineBuiltins(Interpreter* interp, DefnTree* ns)
	{
		define_builtin_types(interp, ns);
		define_builtin_funcs(interp, ns);
	}
}
