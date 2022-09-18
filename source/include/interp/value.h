// value.h
// Copyright (c) 2022, zhiayang
// SPDX-License-Identifier: Apache-2.0

#pragma once

#include <vector>

namespace sap::interp
{
	struct Value
	{

	private:
		union
		{
			// only one non-trivial class,
			std::vector<Value> v_value_list;

			bool v_bool;
			uint32_t v_char;
			int64_t v_integer;
			double v_floating;
			Value* v_pointer;
		};
	};
}
