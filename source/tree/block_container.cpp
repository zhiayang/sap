// block_container.cpp
// Copyright (c) 2022, zhiayang
// SPDX-License-Identifier: Apache-2.0

#include "tree/container.h"

#include "layout/base.h"

namespace sap::tree
{
	layout::LineCursor BlockContainer::layout_fn(interp::Interpreter* cs, layout::LayoutBase* layout, layout::LineCursor cursor,
	    const Style* style, const DocumentObject* obj_)
	{
		auto container = static_cast<BlockContainer*>(const_cast<DocumentObject*>(obj_));
		for(auto& obj : container->contents())
		{
			auto layout_fn = obj->getLayoutFunction();
			if(not layout_fn.has_value())
				continue;

			cursor = (*layout_fn)(cs, layout, cursor, style, obj.get());
			cursor = cursor.newLine(0);
		}

		return cursor;
	}

	auto BlockContainer::getLayoutFunction() const -> std::optional<LayoutFn>
	{
		return &BlockContainer::layout_fn;
	}
}
