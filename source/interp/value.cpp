// value.cpp
// Copyright (c) 2022, zhiayang
// SPDX-License-Identifier: Apache-2.0

#include "interp/tree.h"
#include "interp/value.h"

namespace sap::interp
{
	auto Value::clone_tios(const InlineObjects& from) -> InlineObjects
	{
		InlineObjects ret {};
		ret.reserve(from.size());

		for(auto& tio : from)
		{
			// whatever is left here should only be text!
			if(auto txt = dynamic_cast<const tree::Text*>(tio.get()); txt)
				ret.emplace_back(new tree::Text(txt->contents(), txt->style()));

			else
				sap::internal_error("????? non-text TIOs leaked out!");
		}

		return ret;
	}
}
