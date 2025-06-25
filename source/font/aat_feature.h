// aat_feature.h
// Copyright (c) 2022, yuki
// SPDX-License-Identifier: Apache-2.0

#pragma once

namespace font::aat
{
	struct Feature
	{
		uint16_t type;
		uint16_t selector;

		constexpr bool operator==(Feature f) const { return type == f.type && selector == f.selector; }
		constexpr bool operator!=(Feature f) const { return !(*this == f); }
	};
}
