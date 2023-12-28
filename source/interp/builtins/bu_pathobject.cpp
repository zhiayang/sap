// bu_pathobject.cpp
// Copyright (c) 2023, zhiayang
// SPDX-License-Identifier: Apache-2.0

#include "interp/interp.h"
#include "interp/builtin_fns.h"
#include "interp/builtin_types.h"

namespace sap::interp::builtin
{
	using PT = frontend::PType;

	const Type* builtin::BU_PathObject::type = nullptr;

	std::vector<ast::UnionDefn::Case> BU_PathObject::variants()
	{
		using P = std::pair<std::string, PT>;

		auto posT = ptype_for_builtin<BS_Pos2d>();
		auto sizeT = ptype_for_builtin<BS_Size2d>();

		auto ret = std::vector<ast::UnionDefn::Case>();

		// keep these in the same order as the Kind defn in PathObject
		ret.push_back(make_builtin_union_variant("Move", P("pos", posT)));
		ret.push_back(make_builtin_union_variant("Line", P("pos", posT)));
		ret.push_back(make_builtin_union_variant("CubicBezier", P("c1", posT), P("c2", posT), P("end", posT)));
		ret.push_back(make_builtin_union_variant("CubicBezierIC1", P("c2", posT), P("end", posT)));
		ret.push_back(make_builtin_union_variant("CubicBezierIC2", P("c1", posT), P("end", posT)));
		ret.push_back(make_builtin_union_variant("Rectangle", P("pos", posT), P("size", sizeT)));
		ret.push_back(make_builtin_union_variant("Close"));

		return ret;
	}

	Value builtin::BU_PathObject::make(Evaluator* ev, PathObject obj)
	{
		using K = PathObject::Kind;

		auto union_type = type->toUnion();
		auto [variant_idx, variant_value] = [&obj, ev, union_type]() -> std::pair<size_t, Value> {
			auto [vt, vi] = [&obj, union_type]() -> std::pair<const StructType*, size_t> {
				const char* name = nullptr;
				switch(obj.kind())
				{
					case K::Move: name = "Move"; break;
					case K::Line: name = "Line"; break;
					case K::CubicBezier: name = "CubicBezier"; break;
					case K::CubicBezierIC1: name = "CubicBezierIC1"; break;
					case K::CubicBezierIC2: name = "CubicBezierIC2"; break;
					case K::Rectangle: name = "Rectangle"; break;
					case K::Close: name = "Close"; break;
				};
				return { union_type->getCaseNamed(name), union_type->getCaseIndex(name) };
			}();

			switch(obj.kind())
			{
				case K::Move: return { vi, StructMaker(vt).set("pos", BS_Pos2d::make(ev, obj.points()[0])).make() };
				case K::Line: return { vi, StructMaker(vt).set("pos", BS_Pos2d::make(ev, obj.points()[0])).make() };
				case K::CubicBezier:
					return {
						vi,
						StructMaker(vt)
						    .set("c1", BS_Pos2d::make(ev, obj.points()[0]))
						    .set("c2", BS_Pos2d::make(ev, obj.points()[1]))
						    .set("end", BS_Pos2d::make(ev, obj.points()[2]))
						    .make(),
					};

				case K::CubicBezierIC1:
					return {
						vi,
						StructMaker(vt)
						    .set("c2", BS_Pos2d::make(ev, obj.points()[0]))
						    .set("end", BS_Pos2d::make(ev, obj.points()[1]))
						    .make(),
					};

				case K::CubicBezierIC2:
					return {
						vi,
						StructMaker(vt)
						    .set("c1", BS_Pos2d::make(ev, obj.points()[0]))
						    .set("end", BS_Pos2d::make(ev, obj.points()[1]))
						    .make(),
					};

				case K::Rectangle:
					return {
						vi,
						StructMaker(vt)
						    .set("pos", BS_Pos2d::make(ev, obj.points()[0]))
						    .set("size", BS_Size2d::make(ev, obj.points()[1]))
						    .make(),
					};

				case K::Close: return { vi, StructMaker(vt).make() };
			}
		}();

		return Value::unionVariant(union_type, variant_idx, std::move(variant_value));
	}

	PathObject builtin::BU_PathObject::unmake(Evaluator* ev, const Value& value)
	{
		if(not value.isUnion())
			sap::internal_error("expected type '{}', got '{}'", BU_PathObject::type, value.type());

		auto ut = BU_PathObject::type->toUnion();

		auto variant_idx = value.getUnionVariantIndex();
		auto& variant_struct = value.getUnionUnderlyingStruct();

		if(variant_idx >= ut->getCases().size())
			sap::internal_error("variant index '{}' out of bounds", variant_idx);

		auto pp = [ev, &variant_struct](zst::str_view field) {
			return BS_Pos2d::unmake(ev, variant_struct.getStructField(field));
		};

		using K = PathObject::Kind;
		switch(static_cast<PathObject::Kind>(variant_idx))
		{
			case K::Move: return PathObject::move(pp("pos"));
			case K::Line: return PathObject::line(pp("pos"));
			case K::CubicBezier: return PathObject::cubicBezier(pp("c1"), pp("c2"), pp("end"));
			case K::CubicBezierIC1: return PathObject::cubicBezierIC1(pp("c2"), pp("end"));
			case K::CubicBezierIC2: return PathObject::cubicBezierIC2(pp("c1"), pp("end"));
			case K::Rectangle: return PathObject::rectangle(pp("pos"), pp("size"));
			case K::Close: return PathObject::close();
		}
	}
}
