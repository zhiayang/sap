// interp.h
// Copyright (c) 2022, zhiayang
// SPDX-License-Identifier: Apache-2.0

#pragma once

#include <stdint.h>
#include <stddef.h>

#include <vector>
#include <string>

namespace sap::interp
{
	struct Stmt
	{
		virtual ~Stmt();
	};

	struct Expr : Stmt
	{
	};


	struct QualifiedId
	{
		std::string name;
	};

	struct FunctionCall : Expr
	{
		struct Arg
		{
			std::optional<std::string> name;
			std::unique_ptr<Expr> value;
		};

		QualifiedId callee;
		std::vector<Arg> arguments;
	};
}
