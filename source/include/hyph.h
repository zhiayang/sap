// hyph.h
// Copyright (c) 2022, not zhiayang
// SPDX-License-Identifier: Apache-2.0

#pragma once

#include "util.h"
#include <array>

namespace sap::hyph
{

	class AsciiHyph
	{
		using HyphenationPoints = std::array<uint8_t, 16>;

		struct Pats
		{
			util::hashmap<util::ShortString, HyphenationPoints> m_front_pats;
			util::hashmap<util::ShortString, HyphenationPoints> m_mid_pats;
			util::hashmap<util::ShortString, HyphenationPoints> m_back_pats;

			static Pats parse(zst::str_view contents);
		};

		void parseAndAddExceptions(zst::str_view contents);

		Pats m_pats;

		AsciiHyph(Pats pats) : m_pats(std::move(pats)) { }

		util::hashmap<std::string, std::vector<uint8_t>> m_hyphenation_cache;

	public:
		static AsciiHyph parseFromFile(const std::string& path);
		const std::vector<uint8_t>& computeHyphenationPointsForLowercaseAlphabeticalWord(zst::str_view word);
	};
}
