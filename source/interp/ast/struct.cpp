// struct.cpp
// Copyright (c) 2022, zhiayang
// SPDX-License-Identifier: Apache-2.0

#include "interp/ast.h"
#include "interp/misc.h"
#include "interp/interp.h"

namespace sap::interp
{
	static ErrorOr<void> ensure_bridge_types_correspond(const StructType* ty, const std::vector<StructType::Field>& b)
	{
		auto& a = ty->getFields();
		if(a.size() != b.size())
			return ErrFmt("mismatched field count: expected {}, got {}", a.size(), b.size());

		for(size_t i = 0; i < a.size(); i++)
		{
			if(a[i].name != b[i].name)
				return ErrFmt("mismatched name for field {}: expected '{}', got '{}'", i + 1, a[i].name, b[i].name);
			else if(a[i].type != b[i].type)
				return ErrFmt("mismatched type for field {}: expected '{}', got '{}'", i + 1, a[i].type, b[i].type);
		}

		return Ok();
	}


	ErrorOr<TCResult> StructDecl::typecheck_impl(Typechecker* ts, const Type* infer) const
	{
		TRY(ts->current()->declare(this));

		if(auto it = this->attributes.find(ATTRIBUTE_BRIDGE); it != this->attributes.end())
		{
			if(it->second.args.size() != 1)
				return ErrFmt("expected exactly 1 argument for 'bridge' attribute");

			auto bridge_type = ts->getBridgedType(it->second.args[0]);
			assert(bridge_type != nullptr);

			return TCResult::ofRValue(bridge_type);
		}

		// when declaring a struct, we just make an empty field list.
		// the fields will be set later on.
		return TCResult::ofRValue(Type::makeStruct(this->name, {}));
	}

	ErrorOr<TCResult> StructDefn::typecheck_impl(Typechecker* ts, const Type* infer) const
	{
		// transfer the attributes to the declaration before we typecheck it.
		bool was_bridged = this->attributes.contains(ATTRIBUTE_BRIDGE);
		this->declaration->attributes = std::move(this->attributes);

		this->declaration->resolved_defn = this;
		auto struct_type = TRY(this->declaration->typecheck(ts)).type()->toStruct();

		std::unordered_set<std::string> seen_names;
		std::vector<StructType::Field> field_types {};

		for(auto& [name, type, init_value] : this->fields)
		{
			if(seen_names.contains(name))
				return ErrFmt("duplicate field '{}' in struct '{}'", name, struct_type->name());

			seen_names.insert(name);

			auto field_type = TRY(ts->resolveType(type));
			if(field_type == struct_type)
				return ErrFmt("recursive struct not allowed");
			else if(field_type->isVoid())
				return ErrFmt("field cannot have type 'void'");

			if(init_value != nullptr)
			{
				auto initialiser_type = TRY(init_value->typecheck(ts)).type();
				if(not ts->canImplicitlyConvert(initialiser_type, field_type))
					return ErrFmt("cannot initialise field of type '{}' with value of type '{}'", field_type, initialiser_type);
			}

			field_types.push_back(StructType::Field {
			    .name = name,
			    .type = field_type,
			});
		}

		if(was_bridged)
			TRY(ensure_bridge_types_correspond(struct_type, field_types));
		else
			const_cast<StructType*>(struct_type)->setFields(std::move(field_types));


		TRY(ts->addTypeDefinition(struct_type, this));
		return TCResult::ofRValue(struct_type);
	}




	ErrorOr<EvalResult> StructDecl::evaluate(Evaluator* ev) const
	{
		// do nothing
		return EvalResult::ofVoid();
	}

	ErrorOr<EvalResult> StructDefn::evaluate(Evaluator* ev) const
	{
		// this also doesn't do anything
		return EvalResult::ofVoid();
	}







	static std::vector<std::tuple<std::string, const Type*, const Expr*>> get_field_things(const StructDefn* struct_defn,
	    const StructType* struct_type)
	{
		std::vector<std::tuple<std::string, const Type*, const Expr*>> fields {};

		auto& struct_fields = struct_type->getFields();
		for(size_t i = 0; i < struct_fields.size(); i++)
		{
			fields.push_back({
			    struct_fields[i].name,
			    struct_fields[i].type,
			    struct_defn->fields[i].initialiser.get(),
			});
		}

		return fields;
	}

	ErrorOr<TCResult> StructLit::typecheck_impl(Typechecker* ts, const Type* infer) const
	{
		const StructType* struct_type = nullptr;
		if(struct_name.name.empty())
		{
			if(infer == nullptr)
				return ErrFmt("cannot infer type of struct literal");
			else if(not infer->isStruct())
				return ErrFmt("inferred non-struct type '{}' for struct literal", infer);

			struct_type = infer->toStruct();
		}
		else
		{
			auto t = TRY(ts->resolveType(frontend::PType::named(this->struct_name)));
			if(not t->isStruct())
				return ErrFmt("invalid non-struct type '{}' for struct literal", t);

			struct_type = t->toStruct();
		}

		m_struct_defn = dynamic_cast<const StructDefn*>(TRY(ts->getDefinitionForType(struct_type)));
		assert(m_struct_defn != nullptr);

		// make sure the struct has all the things
		std::vector<ArrangeArg<const Type*>> processed_fields {};
		for(auto& f : this->field_inits)
		{
			processed_fields.push_back({
			    .name = f.name,
			    .value = TRY(f.value->typecheck(ts)).type(),
			});
		}

		auto fields = get_field_things(m_struct_defn, struct_type);
		auto ordered = TRY(arrange_argument_types(fields, processed_fields, "struct", "field", "field"));

		TRY(get_calling_cost(ts, fields, ordered, "struct", "field", "field"));

		return TCResult::ofRValue(struct_type);
	}

	ErrorOr<EvalResult> StructLit::evaluate(Evaluator* ev) const
	{
		assert(this->get_type()->isStruct());
		auto struct_type = this->get_type()->toStruct();

		auto struct_defn = dynamic_cast<const StructDefn*>(m_struct_defn);
		assert(struct_defn != nullptr);

		std::vector<ArrangeArg<Value>> processed_fields {};
		for(auto& f : this->field_inits)
		{
			processed_fields.push_back({
			    .name = f.name,
			    .value = TRY_VALUE(f.value->evaluate(ev)),
			});
		}

		auto fields = get_field_things(m_struct_defn, struct_type);
		auto ordered = TRY(arrange_argument_values(fields, std::move(processed_fields), //
		    "struct", "field", "field"));

		std::vector<Value> field_values {};

		for(size_t i = 0; i < struct_type->getFields().size(); i++)
		{
			if(auto it = ordered.find(i); it == ordered.end())
			{
				if(struct_defn->fields[i].initialiser == nullptr)
					return ErrFmt("missing value for field '{}'", struct_defn->fields[i].name);

				field_values.push_back(TRY_VALUE(struct_defn->fields[i].initialiser->evaluate(ev)));
			}
			else
			{
				field_values.push_back(std::move(it->second));
			}
		}

		return EvalResult::ofValue(Value::structure(struct_type, std::move(field_values)));
	}
}
