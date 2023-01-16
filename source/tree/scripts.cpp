// scripts.cpp
// Copyright (c) 2022, zhiayang
// SPDX-License-Identifier: Apache-2.0

#include "tree/base.h"

#include "layout/base.h"

namespace sap::tree
{
	auto ScriptObject::createLayoutObject(interp::Interpreter* cs, layout::LineCursor cursor, const Style* parent_style) const
	    -> LayoutResult
	{
		return { cursor, std::nullopt };
	}

	// just throw these here
	DocumentObject::~DocumentObject()
	{
	}

	InlineObject::~InlineObject()
	{
	}
}
