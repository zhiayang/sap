// builtin_fns.h
// Copyright (c) 2022, zhiayang
// SPDX-License-Identifier: Apache-2.0

#pragma once

#include <memory>

#include "util.h"

#include "interp/value.h"       // for Interpreter, Value
#include "interp/eval_result.h" // for EvalResult

namespace sap::interp
{
	struct Expr;
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


		ErrorOr<EvalResult> make_span(Evaluator* ev, std::vector<Value>& args);
		ErrorOr<EvalResult> make_text(Evaluator* ev, std::vector<Value>& args);
		ErrorOr<EvalResult> make_line(Evaluator* ev, std::vector<Value>& args);
		ErrorOr<EvalResult> make_paragraph(Evaluator* ev, std::vector<Value>& args);

		ErrorOr<EvalResult> ref_object(Evaluator* ev, std::vector<Value>& args);
		ErrorOr<EvalResult> get_layout_object(Evaluator* ev, std::vector<Value>& args);
		ErrorOr<EvalResult> get_layout_object_position(Evaluator* ev, std::vector<Value>& args);

		ErrorOr<EvalResult> set_tio_width(Evaluator* ev, std::vector<Value>& args);
		ErrorOr<EvalResult> set_tbo_width(Evaluator* ev, std::vector<Value>& args);
		ErrorOr<EvalResult> set_tbo_height(Evaluator* ev, std::vector<Value>& args);
		ErrorOr<EvalResult> set_tbo_size(Evaluator* ev, std::vector<Value>& args);

		ErrorOr<EvalResult> offset_object_position(Evaluator* ev, std::vector<Value>& args);
		ErrorOr<EvalResult> override_object_position(Evaluator* ev, std::vector<Value>& args);

		ErrorOr<EvalResult> output_at_absolute_pos_tbo(Evaluator* ev, std::vector<Value>& args);
	}
}
