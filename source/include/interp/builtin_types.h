// builtin_types.h
// Copyright (c) 2022, zhiayang
// SPDX-License-Identifier: Apache-2.0

#pragma once

#include "util.h"

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
	}
}

namespace sap::interp::builtin
{
	// BS: builtin struct
	// BE: builtin enum

	struct BS_Style
	{
		static constexpr auto name = "Style";

		static const Type* type;
		static std::vector<StructDefn::Field> fields();

		static Value make(Evaluator* ev, const Style* style);
		static ErrorOr<const Style*> unmake(Evaluator* ev, const Value& value);
	};

	struct BS_Font
	{
		static constexpr auto name = "Font";

		static const Type* type;
		static std::vector<StructDefn::Field> fields();

		static Value make(Evaluator* ev, pdf::PdfFont* font);
		static ErrorOr<pdf::PdfFont*> unmake(Evaluator* ev, const Value& value);
	};

	struct BS_FontFamily
	{
		static constexpr auto name = "FontFamily";

		static const Type* type;
		static std::vector<StructDefn::Field> fields();

		static Value make(Evaluator* ev, sap::FontFamily font);
		static ErrorOr<sap::FontFamily> unmake(Evaluator* ev, const Value& value);
	};

	struct BS_Position
	{
		static constexpr auto name = "Position";

		static const Type* type;
		static std::vector<StructDefn::Field> fields();

		static Value make(Evaluator* ev, layout::RelativePos pos);
		static ErrorOr<layout::RelativePos> unmake(Evaluator* ev, const Value& value);
	};

	struct BS_Size2d
	{
		static constexpr auto name = "Size2d";

		static const Type* type;
		static std::vector<StructDefn::Field> fields();

		static Value make(Evaluator* ev, DynLength2d pos);
		static ErrorOr<DynLength2d> unmake(Evaluator* ev, const Value& value);
	};

	struct BS_DocumentSettings
	{
		static constexpr auto name = "DocumentSettings";

		static const Type* type;
		static std::vector<StructDefn::Field> fields();

		static Value make(Evaluator* ev, DocumentSettings pos);
		static ErrorOr<DocumentSettings> unmake(Evaluator* ev, const Value& value);
	};

	struct BS_DocumentMargins
	{
		static constexpr auto name = "DocumentMargins";

		static const Type* type;
		static std::vector<StructDefn::Field> fields();

		static Value make(Evaluator* ev, DocumentSettings::Margins pos);
		static ErrorOr<DocumentSettings::Margins> unmake(Evaluator* ev, const Value& value);
	};





	struct BE_Alignment
	{
		static constexpr auto name = "Alignment";

		static const Type* type;

		static frontend::PType enumeratorType();
		static std::vector<EnumDefn::EnumeratorDefn> enumerators();

		static Value make(Alignment alignment);
		static Alignment unmake(const Value& value);
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

		template <typename T>
		StructMaker& setOptional(zst::str_view field, std::optional<T> value, Evaluator* ev, Value (*fn)(Evaluator*, T))
		{
			auto field_type = m_type->getFieldNamed(field)->optionalElement();
			if(value.has_value())
				return this->set(field, Value::optional(field_type, fn(ev, std::move(*value))));
			else
				return this->set(field, Value::optional(field_type, std::nullopt));
		}



	private:
		const StructType* m_type;
		std::vector<Value> m_fields;
	};


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
};
