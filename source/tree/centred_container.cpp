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
		auto obj = static_cast<CentredContainer*>(const_cast<DocumentObject*>(obj_));
		auto layout_fn = obj->inner().getLayoutFunction();
		if(not layout_fn.has_value())
			return cursor;

		cursor = cursor.newLine(0);

		auto centred_layout = std::make_unique<layout::CentredLayout>(layout, std::move(cursor));
		auto inner_cursor = (*layout_fn)(cs, centred_layout.get(), centred_layout->newCursor(), style, obj->m_inner.get());

		centred_layout->computeLayoutSize();

		auto tmp = centred_layout.get();
		layout->addObject(std::move(centred_layout));

		return tmp->parentCursor();
	}

	CentredContainer::CentredContainer(std::unique_ptr<BlockObject> inner) : m_inner(std::move(inner))
	{
	}

	auto CentredContainer::getLayoutFunction() const -> std::optional<LayoutFn>
	{
		return &CentredContainer::layout_fn;
	}
}
