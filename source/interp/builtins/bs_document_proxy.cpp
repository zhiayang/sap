// bs_document_proxy.cpp
// Copyright (c) 2022, zhiayang
// SPDX-License-Identifier: Apache-2.0

#include "layout/document.h"

#include "interp/interp.h"
#include "interp/builtin_fns.h"
#include "interp/builtin_types.h"

namespace sap::interp::builtin
{
	using PT = frontend::PType;
	using Field = StructDefn::Field;

	const Type* builtin::BS_DocumentProxy::type = nullptr;
	std::vector<Field> builtin::BS_DocumentProxy::fields()
	{
		auto pt_int = PT::named(frontend::TYPE_INT);
		auto pt_outline_item = ptype_for_builtin<BS_OutlineItem>();
		auto pt_size2d = ptype_for_builtin<BS_Size2d>();

		return util::vectorOf(                                                    //
			Field { .name = "page_count", .type = pt_int },                       //
			Field { .name = "page_size", .type = pt_size2d },                     //
			Field { .name = "outline_items", .type = PT::array(pt_outline_item) } //
		);
	}

	Value builtin::BS_DocumentProxy::make(Evaluator* ev, layout::Document* doc)
	{
		auto& pl = doc->pageLayout();
		auto page_size = BS_Size2d::make(ev,
			DynLength2d {
				.x = DynLength(pl.pageSize().x()),
				.y = DynLength(pl.pageSize().y()),
			});

		return StructMaker(BS_DocumentProxy::type->toStruct())
		    .set("page_count", Value::integer(checked_cast<int64_t>(pl.pageCount())))
		    .set("page_size", std::move(page_size))
		    .set("outline_items",
				Value::array(BS_OutlineItem::type,
					util::map(doc->outlineItems(), [ev](auto&& item) { return BS_OutlineItem::make(ev, item); })))
		    .make();
	}

	DocumentProxy builtin::BS_DocumentProxy::unmake(Evaluator* ev, const Value& value)
	{
		// ignore "page_count" and "page_size"
		auto& x = value.getStructField("outline_items");
		auto outline_items = util::map(x.getArray(), [ev](const auto& val) { return BS_OutlineItem::unmake(ev, val); });

		return DocumentProxy {
			.outline_items = std::move(outline_items),
		};
	}
}
