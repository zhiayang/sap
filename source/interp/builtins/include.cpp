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
		zpr::println("including file '{}'", file_path);

		auto file_contents = util::readEntireFile(file_path);
		auto doc = frontend::parse(ev->keepStringAlive(file_path), file_contents.span().chars());

		std::vector<std::unique_ptr<tree::BlockObject>> objs {};
		for(auto& obj : doc.objects())
		{
			if(auto blk = dynamic_cast<tree::BlockObject*>(obj.get()); blk != nullptr)
			{
				objs.push_back(util::static_pointer_cast<tree::BlockObject>(std::move(obj)));
			}
			else
			{
				return ErrMsg(ev, "unsupported object at top-level of included file");
			}
		}

		auto blk_container = std::make_unique<tree::VertBox>();
		blk_container->contents().swap(objs);

		return EvalResult::ofValue(Value::treeBlockObject(std::move(blk_container)));
	}
}
