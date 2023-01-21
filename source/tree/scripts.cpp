// scripts.cpp
// Copyright (c) 2022, zhiayang
// SPDX-License-Identifier: Apache-2.0

#include "tree/base.h"
#include "tree/paragraph.h"

#include "layout/base.h"
#include "interp/interp.h"
#include "interp/evaluator.h"

namespace sap::tree
{
	auto ScriptCall::createLayoutObject(interp::Interpreter* cs, layout::PageCursor cursor, const Style* parent_style) const
	    -> LayoutResult
	{
		// steal the parent output context (since we don't actually contain stuff)
		auto _ = cs->evaluator().pushBlockContext(cursor, this, cs->evaluator().getBlockContext().output_context);

		auto value_or_err = cs->run(this->call.get());

		if(value_or_err.is_err())
			error("interp", "evaluation failed: {}", value_or_err.error());

		auto value_or_empty = value_or_err.take_value();
		if(not value_or_empty.hasValue())
			return { cursor, util::vectorOf<std::unique_ptr<layout::LayoutObject>>() };

		auto style = cs->evaluator().currentStyle().unwrap()->extendWith(parent_style);
		if(value_or_empty.get().isTreeBlockObj())
		{
			return cs->leakBlockObject(std::move(value_or_empty.get()).takeTreeBlockObj())
			    .createLayoutObject(cs, std::move(cursor), style);
		}
		else
		{
			auto tmp = cs->evaluator().convertValueToText(std::move(value_or_empty).take());
			if(tmp.is_err())
				error("interp", "convertion to text failed: {}", tmp.error());

			auto objs = tmp.take_value();
			auto new_para = std::make_unique<Paragraph>();
			new_para->addObjects(std::move(objs));

			return cs->leakBlockObject(std::move(new_para)).createLayoutObject(cs, std::move(cursor), style);
		}
	}



	auto ScriptBlock::createLayoutObject(interp::Interpreter* cs, layout::PageCursor cursor, const Style* parent_style) const
	    -> LayoutResult
	{
		if(auto ret = cs->run(this->body.get()); ret.is_err())
			ret.error().showAndExit();

		return { cursor, util::vectorOf<std::unique_ptr<layout::LayoutObject>>() };
	}


	// just throw these here
	DocumentObject::~DocumentObject()
	{
	}

	InlineObject::~InlineObject()
	{
	}
}
