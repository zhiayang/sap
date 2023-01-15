// centred_container.cpp
// Copyright (c) 2022, zhiayang
// SPDX-License-Identifier: Apache-2.0

#include "tree/container.h"

#include "layout/base.h"

namespace sap::tree
{
	layout::LineCursor CentredContainer::layout_fn(interp::Interpreter* cs, layout::LayoutBase* layout, layout::LineCursor cursor,
	    const Style* style, const DocumentObject* obj_)
	{
		auto container = static_cast<CentredContainer*>(const_cast<DocumentObject*>(obj_));
		auto layout_fn = container->inner().getLayoutFunction();
		if(not layout_fn.has_value())
			return cursor;

		// TODO: inter-object margin!
		cursor = (*layout_fn)(cs, layout, cursor, style, &container->inner());
		cursor = cursor.newLine(0);

		return cursor;
	}

	CentredContainer::CentredContainer(std::unique_ptr<BlockObject> inner) : m_inner(std::move(inner))
	{
	}

	auto CentredContainer::getLayoutFunction() const -> std::optional<LayoutFn>
	{
		return &CentredContainer::layout_fn;
	}
}
