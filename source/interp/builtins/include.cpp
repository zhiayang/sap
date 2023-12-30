// include.cpp
// Copyright (c) 2022, zhiayang
// SPDX-License-Identifier: Apache-2.0

#include "sap/config.h"
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

		auto resolved = TRY(sap::paths::resolveLibrary(ev->loc(), file_path));
		if(auto e = watch::addFileToWatchList(resolved); e.is_err())
			return ErrMsg(ev, "{}", e.error());

		auto cs = ev->interpreter();
		auto file = cs->loadFile(resolved);
		auto doc = TRY(frontend::parse(cs->keepStringAlive(resolved), file.chars()));

		util::log("included file '{}'", resolved);
		return EvalResult::ofValue(ev->addToHeapAndGetPointer(Value::treeBlockObject(std::move(doc).takeContainer())));
	}
}
