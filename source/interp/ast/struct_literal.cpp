// struct_literal.cpp
// Copyright (c) 2022, zhiayang
// SPDX-License-Identifier: Apache-2.0

#include "interp/ast.h"
#include "interp/misc.h"
#include "interp/interp.h"

namespace sap::interp
{
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
			    struct_defn->fields()[i].initialiser.get(),
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
				return ErrMsg(ts, "cannot infer type of struct literal");
			else if(not infer->isStruct())
				return ErrMsg(ts, "inferred non-struct type '{}' for struct literal", infer);

			struct_type = infer->toStruct();
		}
		else
		{
			auto t = TRY(ts->resolveType(frontend::PType::named(this->struct_name)));
			if(not t->isStruct())
				return ErrMsg(ts, "invalid non-struct type '{}' for struct literal", t);

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
		auto ordered = TRY(arrange_argument_types(ts, fields, processed_fields, "struct", "field", "field"));

		TRY(get_calling_cost(ts, fields, ordered, "struct", "field", "field"));

		return TCResult::ofRValue(struct_type);
	}

	ErrorOr<EvalResult> StructLit::evaluate_impl(Evaluator* ev) const
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
		auto ordered = TRY(arrange_argument_values(ev, fields, std::move(processed_fields), //
		    "struct", "field", "field"));

		std::vector<Value> field_values {};

		auto& defn_fields = struct_defn->fields();
		for(size_t i = 0; i < struct_type->getFields().size(); i++)
		{
			Value field {};
			if(auto it = ordered.find(i); it == ordered.end())
			{
				if(defn_fields[i].initialiser == nullptr)
					return ErrMsg(ev, "missing value for field '{}'", defn_fields[i].name);

				field = TRY_VALUE(defn_fields[i].initialiser->evaluate(ev));
			}
			else
			{
				field = std::move(it->second);
			}

			field_values.push_back(ev->castValue(std::move(field), struct_type->getFieldAtIndex(i)));
		}

		return EvalResult::ofValue(Value::structure(struct_type, std::move(field_values)));
	}
}
