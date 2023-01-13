// basedefs.h
// Copyright (c) 2022, zhiayang
// SPDX-License-Identifier: Apache-2.0

#pragma once

namespace sap::interp
{
	struct QualifiedId
	{
		bool top_level = false;
		std::vector<std::string> parents;
		std::string name;
	};
}
