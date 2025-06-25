// features.h
// Copyright (c) 2021, yuki
// SPDX-License-Identifier: Apache-2.0

#pragma once

#include "types.h"

#include "font/tag.h"
#include "font/metrics.h"
#include "font/font_scalar.h"

namespace font
{
	struct FeatureSet
	{
		Tag script;
		Tag language;

		util::hashset<Tag> enabled_features {};
		util::hashset<Tag> disabled_features {};

		bool is_disabled(Tag feat) const { return disabled_features.contains(feat); }
		bool is_enabled(Tag feat) const
		{
			return enabled_features.contains(feat) && not disabled_features.contains(feat);
		}
	};

#define DECLARE_FEATURE(name) inline constexpr auto name = Tag(#name)
	namespace feature
	{
		DECLARE_FEATURE(kern); // kerning
		DECLARE_FEATURE(cpsp); // capital spacing (all-caps words have larger advance)

		DECLARE_FEATURE(aalt); // access all alternates
		DECLARE_FEATURE(calt); // contextual alternates
		DECLARE_FEATURE(salt); // stylistic alternates
		DECLARE_FEATURE(rclt); // required contextual alternates

		DECLARE_FEATURE(liga); // standard ligatures
		DECLARE_FEATURE(dlig); // discretionary ligatures
		DECLARE_FEATURE(clig); // contextual ligatures
		DECLARE_FEATURE(rlig); // required ligatures
		DECLARE_FEATURE(hlig); // historical ligatures

		DECLARE_FEATURE(smcp); // substitute lowercase for small caps
		DECLARE_FEATURE(pcap); // substitute lowercase for petite caps
		DECLARE_FEATURE(c2sc); // substitute uppercase for small caps
		DECLARE_FEATURE(c2pc); // substitute uppercase for petite caps

		// of course, case is a keyword
		inline constexpr auto case_ = Tag("case"); // case sensitive forms

		DECLARE_FEATURE(ss01); // stylistic set 01
		DECLARE_FEATURE(ss02); // stylistic set 02
		DECLARE_FEATURE(ss03); // stylistic set 03
		DECLARE_FEATURE(ss04); // stylistic set 04
		DECLARE_FEATURE(ss05); // stylistic set 05
		DECLARE_FEATURE(ss06); // stylistic set 06
		DECLARE_FEATURE(ss07); // stylistic set 07
		DECLARE_FEATURE(ss08); // stylistic set 08
		DECLARE_FEATURE(ss09); // stylistic set 09
		DECLARE_FEATURE(ss10); // stylistic set 10
		DECLARE_FEATURE(ss11); // stylistic set 11
		DECLARE_FEATURE(ss12); // stylistic set 12
		DECLARE_FEATURE(ss13); // stylistic set 13
		DECLARE_FEATURE(ss14); // stylistic set 14
		DECLARE_FEATURE(ss15); // stylistic set 15
		DECLARE_FEATURE(ss16); // stylistic set 16
		DECLARE_FEATURE(ss17); // stylistic set 17
		DECLARE_FEATURE(ss18); // stylistic set 18
		DECLARE_FEATURE(ss19); // stylistic set 19
		DECLARE_FEATURE(ss20); // stylistic set 20

		DECLARE_FEATURE(swsh); // swash
		DECLARE_FEATURE(cswh); // contextual swash

		DECLARE_FEATURE(sups); // superscript
		DECLARE_FEATURE(subs); // subscript
		DECLARE_FEATURE(ordn); // ordinals
		DECLARE_FEATURE(sinf); // scientific inferiors (eg. the '2' in H2O)
		DECLARE_FEATURE(zero); // slashed zero
		DECLARE_FEATURE(lnum); // lining numbers -- glyphs that look better with capitals
		DECLARE_FEATURE(tnum); // tabular numbers -- uniform-width number glyphs
		DECLARE_FEATURE(onum); // old-style numerals
		DECLARE_FEATURE(pnum); // proportional-width numbers

		DECLARE_FEATURE(frac); // diagonal fractions
		DECLARE_FEATURE(afrc); // alternate (stacked) fractions
		DECLARE_FEATURE(mgrk); // mathematical greek

		DECLARE_FEATURE(pkna); // proportional kana
		DECLARE_FEATURE(hkna); // horizontal kana alternates
		DECLARE_FEATURE(vkna); // vertical kana alternates
		DECLARE_FEATURE(ruby); // furigana

		DECLARE_FEATURE(fwid); // substitute proportional glyphs with full-width ones
		DECLARE_FEATURE(hwid); // subst. prop glyphs with half-width ones
		DECLARE_FEATURE(twid); // ... third-width
		DECLARE_FEATURE(qwid); // ... quarter-width
		DECLARE_FEATURE(pwid); // subst. uniform-width glyphs with proportional ones
		DECLARE_FEATURE(halt); // alternate half-width
		DECLARE_FEATURE(palt); // alternate proportional

		DECLARE_FEATURE(smpl); // simplified kanji
		DECLARE_FEATURE(trad); // traditional kanji
		DECLARE_FEATURE(tnam); // traditional kanji for use in names
		DECLARE_FEATURE(expt); // expert kanji forms
		DECLARE_FEATURE(hojo); // hojo kanji
		DECLARE_FEATURE(nlck); // NLC kanji
		DECLARE_FEATURE(jp78); // JIS1978 kanji
		DECLARE_FEATURE(jp83); // JIS1983 kanji
		DECLARE_FEATURE(jp90); // JIS1990 kanji
		DECLARE_FEATURE(jp04); // JIS2004 kanji

		DECLARE_FEATURE(hngl); // transliterate hanja to hangul
	}

#undef DECLARE_FEATURE

	namespace aat
	{
		struct Feature;
	}

	std::vector<aat::Feature> convertOFFFeatureToAAT(Tag feature);
}
