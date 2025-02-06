// polymorph.cpp
// Copyright (c) 2023, yuki / zhiayang
// SPDX-License-Identifier: Apache-2.0

#include "interp/ast.h"
#include "interp/type.h"
#include "interp/interp.h"
#include "interp/polymorph.h"
#include "interp/overload_resolution.h"

namespace sap::interp::polymorph
{
	using PSOItem = cst::PartiallyResolvedOverloadSet::Item;

	static std::pair<PSOItem, bool> match_generic_arguments(Typechecker* ts,
	    const Type* infer,
	    const cst::Declaration* decl,
	    const ast::FunctionDefn* fn,
	    const std::vector<ast::FunctionCall::Arg>& explicit_args)
	{
		bool failed = false;
		PSOItem item { .decl = decl };

#define _fail_with(loc, msg, ...)                                                            \
	__extension__({                                                                          \
		failed = true;                                                                       \
		item.exclusion_reason.addInfo((loc), zpr::sprint((msg) __VA_OPT__(, ) __VA_ARGS__)); \
		continue;                                                                            \
	})

		util::hashset<size_t> seen_param_indices {};
		util::hashmap<std::string, size_t> expected_arg_names {};
		for(size_t i = 0; i < fn->generic_params.size(); i++)
			expected_arg_names[fn->generic_params[i].name] = i;

		bool saw_named_arg = false;
		size_t positional_param_idx = 0;
		for(auto& arg : explicit_args)
		{
			if(arg.name.has_value())
			{
				saw_named_arg = true;

				const auto it = expected_arg_names.find(*arg.name);
				if(it == expected_arg_names.end())
					_fail_with(arg.value->loc(), "Entity has no parameter named '{}'", *arg.name);

				const size_t param_idx = it->second;
				if(seen_param_indices.contains(param_idx))
					_fail_with(arg.value->loc(), "Duplicate argument for paramter '{}'", *arg.name);

				seen_param_indices.insert(param_idx);

				auto te = arg.value->typecheck(ts);
				if(te.is_err())
					_fail_with(arg.value->loc(), "{}", te.error().string());

				item.applied_types[*arg.name] = te->type();
			}
			else
			{
				if(saw_named_arg)
					_fail_with(arg.value->loc(), "Positional arguments are not allowed after named ones");

				if(positional_param_idx > expected_arg_names.size())
				{
					_fail_with(arg.value->loc(), "too many argumentss provided; expected at most {}",
					    expected_arg_names.size());
				}

				const auto param_idx = positional_param_idx++;
				seen_param_indices.insert(param_idx);

				auto te = arg.value->typecheck(ts);
				if(te.is_err())
					_fail_with(arg.value->loc(), "{}", te.error().string());

				item.applied_types[fn->generic_params[param_idx].name] = te->type();
			}
		}

		return { std::move(item), not failed };

#undef _fail_with
	}


	ErrorOr<TCResult> tryInstantiateGenericFunction(Typechecker* ts,
	    const Type* infer,
	    const QualifiedId& name,
	    std::vector<const cst::Declaration*> declarations,
	    const std::vector<ast::FunctionCall::Arg>& explicit_args)
	{
		auto set = std::make_unique<cst::PartiallyResolvedOverloadSet>(ts->loc(), name);

		for(const auto* decl : declarations)
		{
			if(decl->generic_func)
			{
				auto [item, ok] = match_generic_arguments(ts, infer, decl, decl->generic_func, explicit_args);
				if(ok)
					set->items.push_back(std::move(item));
				else
					set->excluded_items.push_back(std::move(item));
			}
			else
			{
				set->excluded_items.push_back(PSOItem {
				    .decl = decl,
				    .exclusion_reason = ErrorMessage(decl->location,
				        zpr::sprint("is not a generic function and does not accept type arguments", decl->name)),
				});
			}
		}

		if(set->items.empty())
		{
			auto e = ErrorMessage(ts->loc(), zpr::sprint("Failed to resolve reference to generic entity '{}'", name));
			for(auto& f : set->excluded_items)
				e.addInfo(f.decl->location, zpr::sprint("Candidate failed: {}", f.exclusion_reason.string()));

			return Err(std::move(e));
		}

		return TCResult::ofRValue(std::move(set));
	}
}
