// builtin_types.h
// Copyright (c) 2022, yuki
// SPDX-License-Identifier: Apache-2.0

#pragma once

#include "sap/path.h"
#include "sap/annotation.h"
#include "sap/document_settings.h"

#include "layout/cursor.h"

#include "interp/ast.h"
#include "interp/value.h"
#include "interp/parser_type.h"

namespace pdf
{
	struct PdfFont;
}

namespace sap
{
	struct Style;
	struct DynLength;
	enum class Alignment;

	namespace interp
	{
		struct Value;
		struct GlobalState;
		struct DocumentProxy;
	}

	namespace layout
	{
		struct Document;
	}
}

namespace sap::interp::builtin
{
	// BS: builtin struct
	// BE: builtin enum

	struct BS_State
	{
		static constexpr auto name = "State";

		static const Type* type;
		static std::vector<ast::StructDefn::Field> fields();

		static Value make(Evaluator* ev, const GlobalState& style);

		// note: we don't have an unmake, since there shouldn't ever be a reason
		// to make a State -- we only ever pass that to the user scripts, not the other way around.
	};

	struct BS_Style
	{
		static constexpr auto name = "Style";

		static const Type* type;
		static std::vector<ast::StructDefn::Field> fields();

		static Value make(Evaluator* ev, const Style& style);
		static ErrorOr<Style> unmake(Evaluator* ev, const Value& value);
	};

	struct BS_Font
	{
		static constexpr auto name = "Font";

		static const Type* type;
		static std::vector<ast::StructDefn::Field> fields();

		static Value make(Evaluator* ev, pdf::PdfFont* font);
		static ErrorOr<pdf::PdfFont*> unmake(Evaluator* ev, const Value& value);
	};

	struct BS_FontFamily
	{
		static constexpr auto name = "FontFamily";

		static const Type* type;
		static std::vector<ast::StructDefn::Field> fields();

		static Value make(Evaluator* ev, sap::FontFamily font);
		static ErrorOr<sap::FontFamily> unmake(Evaluator* ev, const Value& value);
	};

	struct BS_PagePosition
	{
		static constexpr auto name = "PagePosition";

		static const Type* type;
		static std::vector<ast::StructDefn::Field> fields();

		static Value make(Evaluator* ev, layout::RelativePos pos);
		static layout::RelativePos unmake(Evaluator* ev, const Value& value);
	};

	struct BS_AbsPosition
	{
		static constexpr auto name = "AbsPosition";

		static const Type* type;
		static std::vector<ast::StructDefn::Field> fields();

		static Value make(Evaluator* ev, layout::AbsolutePagePos pos);
		static layout::AbsolutePagePos unmake(Evaluator* ev, const Value& value);
	};

	struct BS_Size2d
	{
		static constexpr auto name = "Size2d";

		static const Type* type;
		static std::vector<ast::StructDefn::Field> fields();

		static Value make(Evaluator* ev, Size2d pos);
		static Value make(Evaluator* ev, DynLength2d pos);

		static DynLength2d unmake(Evaluator* ev, const Value& value);
	};

	struct BS_Pos2d
	{
		static constexpr auto name = "Pos2d";

		static const Type* type;
		static std::vector<ast::StructDefn::Field> fields();

		static Value make(Evaluator* ev, Position pos);
		static Value make(Evaluator* ev, DynLength2d pos);
		static Position unmake(Evaluator* ev, const Value& value);
	};

	struct BS_DocumentSettings
	{
		static constexpr auto name = "DocumentSettings";

		static const Type* type;
		static std::vector<ast::StructDefn::Field> fields();

		static Value make(Evaluator* ev, DocumentSettings pos);
		static ErrorOr<DocumentSettings> unmake(Evaluator* ev, const Value& value);
	};

	struct BS_DocumentMargins
	{
		static constexpr auto name = "DocumentMargins";

		static const Type* type;
		static std::vector<ast::StructDefn::Field> fields();

		static Value make(Evaluator* ev, DocumentSettings::Margins pos);
		static DocumentSettings::Margins unmake(Evaluator* ev, const Value& value);
	};

	struct BS_DocumentProxy
	{
		static constexpr auto name = "DocumentProxy";

		static const Type* type;
		static std::vector<ast::StructDefn::Field> fields();

		static Value make(Evaluator* ev, layout::Document* doc);
		static DocumentProxy unmake(Evaluator* ev, const Value& value);
	};

	struct BS_OutlineItem
	{
		static constexpr auto name = "OutlineItem";

		static const Type* type;
		static std::vector<ast::StructDefn::Field> fields();

		static Value make(Evaluator* ev, OutlineItem pos);
		static OutlineItem unmake(Evaluator* ev, const Value& value);
	};

	struct BS_LinkAnnotation
	{
		static constexpr auto name = "LinkAnnotation";

		static const Type* type;
		static std::vector<ast::StructDefn::Field> fields();

		static Value make(Evaluator* ev, LinkAnnotation pos);
		static LinkAnnotation unmake(Evaluator* ev, const Value& value);
	};

	struct BS_Colour
	{
		static constexpr auto name = "Colour";

		static const Type* type;
		static std::vector<ast::StructDefn::Field> fields();

		static Value make(Evaluator* ev, Colour colour);
		static Colour unmake(Evaluator* ev, const Value& value);
	};

	struct BS_ColourRGB
	{
		static constexpr auto name = "ColourRGB";

		static const Type* type;
		static std::vector<ast::StructDefn::Field> fields();

		static Value make(Evaluator* ev, Colour::RGB colour);
		static Colour::RGB unmake(Evaluator* ev, const Value& value);
	};

	struct BS_ColourCMYK
	{
		static constexpr auto name = "ColourCMYK";

		static const Type* type;
		static std::vector<ast::StructDefn::Field> fields();

		static Value make(Evaluator* ev, Colour::CMYK colour);
		static Colour::CMYK unmake(Evaluator* ev, const Value& value);
	};


	struct GlyphSpacingAdjustment
	{
		std::vector<std::vector<char32_t>> match;
		std::vector<double> adjust;
	};

	struct BS_GlyphSpacingAdjustment
	{
		static constexpr auto name = "GlyphSpacingAdjustment";

		static const Type* type;
		static std::vector<ast::StructDefn::Field> fields();

		static Value make(Evaluator* ev, const GlyphSpacingAdjustment& colour);
		static GlyphSpacingAdjustment unmake(Evaluator* ev, const Value& value);
	};

	struct BS_BorderStyle
	{
		static constexpr auto name = "BorderStyle";

		static const Type* type;
		static std::vector<ast::StructDefn::Field> fields();

		static Value make(Evaluator* ev, const BorderStyle& colour);
		static BorderStyle unmake(Evaluator* ev, const Value& value);
	};






	struct BE_LineJoinStyle
	{
		static constexpr auto name = "LineJoinStyle";

		static const Type* type;

		static frontend::PType enumeratorType();
		static std::vector<ast::EnumDefn::Enumerator> enumerators();

		static Value make(PathStyle::JoinStyle alignment);
		static PathStyle::JoinStyle unmake(const Value& value);
	};

	struct BE_LineCapStyle
	{
		static constexpr auto name = "LineCapStyle";

		static const Type* type;

		static frontend::PType enumeratorType();
		static std::vector<ast::EnumDefn::Enumerator> enumerators();

		static Value make(PathStyle::CapStyle alignment);
		static PathStyle::CapStyle unmake(const Value& value);
	};

	struct BS_PathStyle
	{
		static constexpr auto name = "PathStyle";

		static const Type* type;
		static std::vector<ast::StructDefn::Field> fields();

		static Value make(Evaluator* ev, const PathStyle& style);
		static PathStyle unmake(Evaluator* ev, const Value& value);
	};





	struct BE_Alignment
	{
		static constexpr auto name = "Alignment";

		static const Type* type;

		static frontend::PType enumeratorType();
		static std::vector<ast::EnumDefn::Enumerator> enumerators();

		static Value make(Alignment alignment);
		static Alignment unmake(const Value& value);
	};

	struct BE_ColourType
	{
		static constexpr auto name = "ColourType";

		static const Type* type;

		static frontend::PType enumeratorType();
		static std::vector<ast::EnumDefn::Enumerator> enumerators();

		static Value make(Colour::Type alignment);
		static Colour::Type unmake(const Value& value);
	};

	struct BU_PathSegment
	{
		static constexpr auto name = "PathSegment";

		static const Type* type;

		static std::vector<ast::UnionDefn::Case> variants();

		static Value make(Evaluator* ev, PathSegment alignment);
		static PathSegment unmake(Evaluator* ev, const Value& value);
	};




	// o no builder pattern
	struct StructMaker
	{
		explicit StructMaker(const StructType* type);

		Value make();
		StructMaker& set(zst::str_view field, Value value);

		template <typename T>
		StructMaker& setOptional(zst::str_view field, std::optional<T> value, Value (*value_factory)(T value))
		{
			auto field_type = m_type->getFieldNamed(field)->optionalElement();

			if(value.has_value())
				return this->set(field, Value::optional(field_type, value_factory(std::move(*value))));
			else
				return this->set(field, Value::optional(field_type, std::nullopt));
		}

		StructMaker& setOptional(zst::str_view field,
		    std::optional<DynLength> value,
		    Value (*value_factory)(DynLength value))
		{
			auto field_type = m_type->getFieldNamed(field)->optionalElement();

			if(value.has_value())
				return this->set(field, Value::optional(field_type, value_factory(std::move(*value))));
			else
				return this->set(field, Value::optional(field_type, std::nullopt));
		}


		template <typename T>
		StructMaker& setOptional(zst::str_view field, std::optional<T> value, Evaluator* ev, Value (*fn)(Evaluator*, T))
		{
			auto field_type = m_type->getFieldNamed(field)->optionalElement();
			if(value.has_value())
				return this->set(field, Value::optional(field_type, fn(ev, std::move(*value))));
			else
				return this->set(field, Value::optional(field_type, std::nullopt));
		}

		template <typename T>
		StructMaker& setOptional(zst::str_view field,
		    std::optional<T> value,
		    Evaluator* ev,
		    Value (*fn)(Evaluator*, const T&))
		{
			auto field_type = m_type->getFieldNamed(field)->optionalElement();
			if(value.has_value())
				return this->set(field, Value::optional(field_type, fn(ev, *value)));
			else
				return this->set(field, Value::optional(field_type, std::nullopt));
		}



	private:
		const StructType* m_type;
		std::vector<std::optional<Value>> m_fields;
	};

	template <typename BS>
	frontend::PType ptype_for_builtin()
	{
		return frontend::PType::named(QualifiedId {
		    .top_level = true,
		    .parents = { "builtin" },
		    .name = BS::name,
		});
	}

	inline ast::EnumDefn::Enumerator make_builtin_enumerator(std::string name, int enum_value)
	{
		auto val = std::make_unique<ast::NumberLit>(Location::builtin());
		val->is_floating = false;
		val->int_value = enum_value;
		return ast::EnumDefn::Enumerator {
			.location = Location::builtin(),
			.name = std::move(name),
			.value = std::move(val),
			.declaration = nullptr,
		};
	}

	template <typename... Args>
	    requires(std::same_as<Args, std::pair<std::string, frontend::PType>> && ...)
	inline ast::UnionDefn::Case make_builtin_union_variant(std::string name, Args&&... params)
	{
		auto ret = ast::UnionDefn::Case {
			.location = Location::builtin(),
			.name = std::move(name),
		};

		(ret.params.push_back({
		     .name = std::move(params.first),
		     .type = std::move(params.second),
		     .loc = Location::builtin(),
		 }),
		    ...);

		return ret;
	}




	template <typename T>
	T get_struct_field(const Value& str, zst::str_view field)
	{
		auto& f = str.getStructField(field);
		return f.get<T>();
	}

	template <typename T>
	T get_struct_field(const Value& str, zst::str_view field, T (Value::*getter_method)() const)
	{
		auto& f = str.getStructField(field);

		return (f.*getter_method)();
	}



	template <typename T>
	std::optional<T> get_optional_struct_field(const Value& str, zst::str_view field)
	{
		auto& f = str.getStructField(field);
		if(not f.haveOptionalValue())
			return std::nullopt;

		return (*f.getOptional())->get<T>();
	}

	template <typename T>
	std::optional<T> get_optional_struct_field(const Value& str, zst::str_view field, T (Value::*getter_method)() const)
	{
		auto& f = str.getStructField(field);
		if(not f.haveOptionalValue())
			return std::nullopt;

		return ((*f.getOptional())->*getter_method)();
	}

	template <typename T>
	static std::optional<T> get_optional_enumerator_field(const Value& val, zst::str_view field)
	{
		auto& fields = val.getStructFields();
		auto idx = val.type()->toStruct()->getFieldIndex(field);

		auto& f = fields[idx];
		if(not f.haveOptionalValue())
			return std::nullopt;

		return static_cast<T>((*f.getOptional())->getEnumerator().getInteger());
	};




	template <typename T>
	T unwrap_optional(const Value& val, const T& default_value)
	{
		assert(val.isOptional());
		if(val.haveOptionalValue())
			return val.get<T>();
		else
			return default_value;
	}

	template <typename T>
	T unwrap_optional(Value&& val, const T& default_value)
	{
		assert(val.isOptional());
		if(std::move(val).haveOptionalValue())
			return std::move(val).get<T>();
		else
			return default_value;
	}


	template <typename T, typename Cb>
	static Value make_array(const Type* elm_type, const std::vector<T>& arr, Cb&& each_elem)
	{
		std::vector<Value> values {};
		for(auto& x : arr)
			values.push_back(each_elem(x));

		return Value::array(elm_type, std::move(values));
	}

	template <typename T, typename Cb>
	static std::vector<T> unmake_array(const Value& value, Cb&& each_elem)
	{
		std::vector<T> ret {};
		for(auto& val : value.getArray())
			ret.push_back(each_elem(val));

		return ret;
	}
};
