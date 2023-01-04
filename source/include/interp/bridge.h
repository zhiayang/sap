// bridge.h
// Copyright (c) 2022, zhiayang
// SPDX-License-Identifier: Apache-2.0

#pragma once

#include "interp/ast.h"

namespace sap::interp::bridge
{
	inline constexpr auto STYLE = "Style";

	inline constexpr auto STYLE_F0_FONT_SIZE = "font_size";
	inline constexpr auto STYLE_F1_LINE_SPACING = "line_spacing";

	std::vector<StructDefn::Field> getFieldsForBridgedType(Typechecker* ts, zst::str_view name);
}
