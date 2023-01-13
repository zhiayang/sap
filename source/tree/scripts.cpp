// scripts.cpp
// Copyright (c) 2022, zhiayang
// SPDX-License-Identifier: Apache-2.0

#include "tree/base.h"

namespace sap::tree
{
	auto ScriptObject::getLayoutFunction() const -> std::optional<LayoutFn>
	{
		// scripts don't layout
		return std::nullopt;
	}

	// just throw these here
	DocumentObject::~DocumentObject()
	{
	}

	InlineObject::~InlineObject()
	{
	}
}
