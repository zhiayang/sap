// hyphenator.h
// Copyright (c) 2022, yuki
// SPDX-License-Identifier: Apache-2.0

#pragma once

#include <array>

#include "util.h"
#include "misc/short_string.h"

namespace sap::hyph
{
	struct Hyphenator
	{
		static Hyphenator parseFromFile(const std::string& path);

		const std::vector<uint8_t>& computeHyphenationPoints(zst::wstr_view word);

	private:
		using HyphenationPoints = std::array<uint8_t, 16>;
		struct Pats
		{
			util::hashmap<util::ShortString<char32_t>, HyphenationPoints> m_front_pats;
			util::hashmap<util::ShortString<char32_t>, HyphenationPoints> m_mid_pats;
			util::hashmap<util::ShortString<char32_t>, HyphenationPoints> m_back_pats;

			static Pats parse(zst::wstr_view contents);
		};

		Hyphenator(Pats pats) : m_pats(std::move(pats)) { }
		void parseAndAddExceptions(zst::wstr_view contents);

		Pats m_pats;
		util::hashmap<std::u32string, std::vector<uint8_t>> m_hyphenation_cache;
	};
}
