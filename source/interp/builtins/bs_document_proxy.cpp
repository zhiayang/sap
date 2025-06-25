// bs_document_proxy.cpp
// Copyright (c) 2022, yuki
// SPDX-License-Identifier: Apache-2.0

#include "layout/document.h"

#include "interp/builtin_fns.h"
#include "interp/builtin_types.h"
#include "interp/interp.h"

namespace sap::interp::builtin
{
	using PT = frontend::PType;
	using Field = ast::StructDefn::Field;

	const Type* builtin::BS_DocumentProxy::type = nullptr;
	std::vector<Field> builtin::BS_DocumentProxy::fields()
	{
		auto pt_int = PT::named(frontend::TYPE_INT);
		auto pt_outline_item = ptype_for_builtin<BS_OutlineItem>();
		auto pt_link_annotation = ptype_for_builtin<BS_LinkAnnotation>();
		auto pt_size2d = ptype_for_builtin<BS_Size2d>();

		return util::vectorOf(                                                     //
		    Field { .name = "page_count", .type = pt_int },                        //
		    Field { .name = "page_size", .type = pt_size2d },                      //
		    Field { .name = "content_size", .type = pt_size2d },                   //
		    Field { .name = "outline_items", .type = PT::array(pt_outline_item) }, //
		    Field { .name = "link_annotations", .type = PT::array(pt_link_annotation) });
	}

	Value builtin::BS_DocumentProxy::make(Evaluator* ev, layout::Document* doc)
	{
		return StructMaker(BS_DocumentProxy::type->toStruct())
		    .set("page_count",
		        Value::fromGenerator(Type::makeInteger(),
		            [doc]() { return Value::integer(checked_cast<int64_t>(doc->pageLayout().pageCount())); }))
		    .set("page_size",
		        Value::fromGenerator(BS_Size2d::type,
		            [ev, doc]() { return BS_Size2d::make(ev, doc->pageLayout().pageSize()); }))
		    .set("content_size",
		        Value::fromGenerator(BS_Size2d::type,
		            [ev, doc]() { return BS_Size2d::make(ev, doc->pageLayout().contentSize()); }))
		    .set("outline_items",
		        Value::array(BS_OutlineItem::type,
		            util::map(doc->outlineItems(), [ev](auto&& item) { return BS_OutlineItem::make(ev, item); })))
		    .set("link_annotations",
		        Value::array(BS_LinkAnnotation::type,
		            util::map(doc->annotations(), [ev](auto&& item) { return BS_LinkAnnotation::make(ev, item); })))
		    .make();
	}

	DocumentProxy builtin::BS_DocumentProxy::unmake(Evaluator* ev, const Value& value)
	{
		// ignore "page_count" and "page_size"
		auto outline_items = util::map(value.getStructField("outline_items").getArray(), [ev](const auto& val) {
			return BS_OutlineItem::unmake(ev, val);
		});

		auto annotations = util::map(value.getStructField("link_annotations").getArray(), [ev](const auto& val) {
			return BS_LinkAnnotation::unmake(ev, val);
		});

		return DocumentProxy {
			.outline_items = std::move(outline_items),
			.link_annotations = std::move(annotations),
		};
	}
}
