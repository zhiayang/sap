// include.cpp
// Copyright (c) 2022, zhiayang
// SPDX-License-Identifier: Apache-2.0

#include "sap/frontend.h"

#include "tree/base.h"
#include "tree/document.h"
#include "tree/container.h"

#include "interp/interp.h"
#include "interp/builtin_fns.h"

namespace sap::interp::builtin
{
	ErrorOr<EvalResult> include_file(Evaluator* ev, std::vector<Value>& args)
	{
		assert(args.size() == 1);

		auto file_path = args[0].getUtf8String();

		auto file = ev->interpreter()->loadFile(file_path);
		auto doc = frontend::parse(ev->keepStringAlive(file_path), file.chars());

		util::log("included file '{}'", file_path);
		return EvalResult::ofValue(Value::treeBlockObject(std::move(doc).takeContainer()));
	}
}
