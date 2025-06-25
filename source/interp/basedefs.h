// basedefs.h
// Copyright (c) 2022, yuki
// SPDX-License-Identifier: Apache-2.0

#pragma once

namespace sap::interp
{
	struct QualifiedId
	{
		bool top_level = false;
		std::vector<std::string> parents;
		std::string name;

		bool operator==(const QualifiedId&) const = default;
		bool operator!=(const QualifiedId&) const = default;

		std::string str() const;

		QualifiedId parentScopeFor(std::string name) const;
		QualifiedId withoutLastComponent() const;

		static QualifiedId named(std::string name);
	};
}
