// types.h
// Copyright (c) 2022, zhiayang
// SPDX-License-Identifier: Apache-2.0

#pragma once

#include <cstdint>

enum class GlyphId : uint32_t { notdef = 0 };
enum class Codepoint : uint32_t { };

constexpr inline GlyphId operator""_gid(unsigned long long x) { return static_cast<GlyphId>(x); }
constexpr inline Codepoint operator""_codepoint(unsigned long long x) { return static_cast<Codepoint>(x); }
