// builtin_fns.h
// Copyright (c) 2022, yuki
// SPDX-License-Identifier: Apache-2.0

#pragma once

#include "interp/value.h"
#include "interp/eval_result.h"

namespace sap::interp
{
	namespace ast
	{
		struct Expr;
	}

	struct Value;
	struct DefnTree;
	struct Evaluator;
	struct Interpreter;

	namespace builtin
	{
		ErrorOr<EvalResult> start_document(Evaluator* ev, std::vector<Value>& args);
		ErrorOr<EvalResult> request_layout(Evaluator* ev, std::vector<Value>& args);

		ErrorOr<EvalResult> bold1(Evaluator* ev, std::vector<Value>& args);
		ErrorOr<EvalResult> italic1(Evaluator* ev, std::vector<Value>& args);
		ErrorOr<EvalResult> bold_italic1(Evaluator* ev, std::vector<Value>& args);

		ErrorOr<EvalResult> set_style_tio(Evaluator* ev, std::vector<Value>& args);
		ErrorOr<EvalResult> set_style_tbo(Evaluator* ev, std::vector<Value>& args);

		ErrorOr<EvalResult> apply_style_tio(Evaluator* ev, std::vector<Value>& args);
		ErrorOr<EvalResult> apply_style_tbo(Evaluator* ev, std::vector<Value>& args);

		ErrorOr<EvalResult> print(Evaluator* ev, std::vector<Value>& args);
		ErrorOr<EvalResult> println(Evaluator* ev, std::vector<Value>& args);

		ErrorOr<EvalResult> load_image(Evaluator* ev, std::vector<Value>& args);
		ErrorOr<EvalResult> include_file(Evaluator* ev, std::vector<Value>& args);

		ErrorOr<EvalResult> current_style(Evaluator* ev, std::vector<Value>& args);
		ErrorOr<EvalResult> push_style(Evaluator* ev, std::vector<Value>& args);
		ErrorOr<EvalResult> pop_style(Evaluator* ev, std::vector<Value>& args);

		ErrorOr<EvalResult> to_string(Evaluator* ev, std::vector<Value>& args);

		ErrorOr<EvalResult> find_font(Evaluator* ev, std::vector<Value>& args);
		ErrorOr<EvalResult> find_font_family(Evaluator* ev, std::vector<Value>& args);

		ErrorOr<EvalResult> make_hbox(Evaluator* ev, std::vector<Value>& args);
		ErrorOr<EvalResult> make_vbox(Evaluator* ev, std::vector<Value>& args);
		ErrorOr<EvalResult> make_zbox(Evaluator* ev, std::vector<Value>& args);

		ErrorOr<EvalResult> hspace(Evaluator* ev, std::vector<Value>& args);
		ErrorOr<EvalResult> vspace(Evaluator* ev, std::vector<Value>& args);
		ErrorOr<EvalResult> page_break(Evaluator* ev, std::vector<Value>& args);


		ErrorOr<EvalResult> make_path(Evaluator* ev, std::vector<Value>& args);
		ErrorOr<EvalResult> make_span(Evaluator* ev, std::vector<Value>& args);
		ErrorOr<EvalResult> make_text(Evaluator* ev, std::vector<Value>& args);
		ErrorOr<EvalResult> make_line(Evaluator* ev, std::vector<Value>& args);
		ErrorOr<EvalResult> make_paragraph(Evaluator* ev, std::vector<Value>& args);
		ErrorOr<EvalResult> make_deferred_block(Evaluator* ev, std::vector<Value>& args);

		ErrorOr<EvalResult> ref_object(Evaluator* ev, std::vector<Value>& args);
		ErrorOr<EvalResult> get_layout_object(Evaluator* ev, std::vector<Value>& args);
		ErrorOr<EvalResult> get_layout_object_size(Evaluator* ev, std::vector<Value>& args);
		ErrorOr<EvalResult> get_layout_object_position(Evaluator* ev, std::vector<Value>& args);

		ErrorOr<EvalResult> raise_tio(Evaluator* ev, std::vector<Value>& args);
		ErrorOr<EvalResult> set_tio_width(Evaluator* ev, std::vector<Value>& args);

		ErrorOr<EvalResult> set_tbo_width(Evaluator* ev, std::vector<Value>& args);
		ErrorOr<EvalResult> set_tbo_height(Evaluator* ev, std::vector<Value>& args);
		ErrorOr<EvalResult> set_tbo_size(Evaluator* ev, std::vector<Value>& args);

		ErrorOr<EvalResult> set_lo_link_annotation(Evaluator* ev, std::vector<Value>& args);
		ErrorOr<EvalResult> set_tbo_link_annotation(Evaluator* ev, std::vector<Value>& args);
		ErrorOr<EvalResult> set_tio_link_annotation(Evaluator* ev, std::vector<Value>& args);

		ErrorOr<EvalResult> offset_object_position(Evaluator* ev, std::vector<Value>& args);
		ErrorOr<EvalResult> override_object_position(Evaluator* ev, std::vector<Value>& args);

		ErrorOr<EvalResult> output_at_absolute_pos_tbo(Evaluator* ev, std::vector<Value>& args);

		ErrorOr<EvalResult> set_border_style(Evaluator* ev, std::vector<Value>& args);

		ErrorOr<EvalResult> adjust_glyph_spacing(Evaluator* ev, std::vector<Value>& args);
	}
}
