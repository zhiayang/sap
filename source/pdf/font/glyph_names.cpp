// glyph_names.cpp
// Copyright (c) 2022, zhiayang
// SPDX-License-Identifier: Apache-2.0

#include "util.h"

namespace pdf
{
	static util::hashmap<std::string, char32_t> g_glyph_names = {
		{ "space", U' ' },
		{ "exclam", U'!' },
		{ "quotedbl", U'"' },
		{ "numbersign", U'#' },
		{ "dollar", U'$' },
		{ "percent", U'%' },
		{ "ampersand", U'&' },
		{ "quoteright", U'\'' },
		{ "parenleft", U'(' },
		{ "parenright", U')' },
		{ "asterisk", U'*' },
		{ "plus", U'+' },
		{ "comma", U',' },
		{ "hyphen", U'-' },
		{ "period", U'.' },
		{ "slash", U'/' },
		{ "zero", U'0' },
		{ "one", U'1' },
		{ "two", U'2' },
		{ "three", U'3' },
		{ "four", U'4' },
		{ "five", U'5' },
		{ "six", U'6' },
		{ "seven", U'7' },
		{ "eight", U'8' },
		{ "nine", U'9' },
		{ "colon", U':' },
		{ "semicolon", U';' },
		{ "less", U'<' },
		{ "equal", U'=' },
		{ "greater", U'>' },
		{ "question", U'?' },
		{ "at", U'@' },
		{ "A", U'A' },
		{ "B", U'B' },
		{ "C", U'C' },
		{ "D", U'D' },
		{ "E", U'E' },
		{ "F", U'F' },
		{ "G", U'G' },
		{ "H", U'H' },
		{ "I", U'I' },
		{ "J", U'J' },
		{ "K", U'K' },
		{ "L", U'L' },
		{ "M", U'M' },
		{ "N", U'N' },
		{ "O", U'O' },
		{ "P", U'P' },
		{ "Q", U'Q' },
		{ "R", U'R' },
		{ "S", U'S' },
		{ "T", U'T' },
		{ "U", U'U' },
		{ "V", U'V' },
		{ "W", U'W' },
		{ "X", U'X' },
		{ "Y", U'Y' },
		{ "Z", U'Z' },
		{ "bracketleft", U'[' },
		{ "backslash", U'\\' },
		{ "bracketright", U']' },
		{ "asciicircum", U'^' },
		{ "underscore", U'_' },
		{ "quoteleft", U'`' },
		{ "a", U'a' },
		{ "b", U'b' },
		{ "c", U'c' },
		{ "d", U'd' },
		{ "e", U'e' },
		{ "f", U'f' },
		{ "g", U'g' },
		{ "h", U'h' },
		{ "i", U'i' },
		{ "j", U'j' },
		{ "k", U'k' },
		{ "l", U'l' },
		{ "m", U'm' },
		{ "n", U'n' },
		{ "o", U'o' },
		{ "p", U'p' },
		{ "q", U'q' },
		{ "r", U'r' },
		{ "s", U's' },
		{ "t", U't' },
		{ "u", U'u' },
		{ "v", U'v' },
		{ "w", U'w' },
		{ "x", U'x' },
		{ "y", U'y' },
		{ "z", U'z' },
		{ "braceleft", U'{' },
		{ "bar", U'|' },
		{ "braceright", U'}' },
		{ "asciitilde", U'~' },
		{ "exclamdown", U'\u00A1' },
		{ "cent", U'\u00A2' },
		{ "sterling", U'\u00A3' },
		{ "fraction", U'\u2044' },
		{ "yen", U'\u00A5' },
		{ "florin", U'\u0192' },
		{ "section", U'\u00A7' },
		{ "currency", U'\u00A4' },
		{ "quotesingle", U'\u0027' },
		{ "quotedblleft", U'\u201C' },
		{ "guillemotleft", U'\u00AB' },
		{ "guilsinglleft", U'\u2039' },
		{ "guilsinglright", U'\u203A' },
		{ "fi", U'\uFB01' },
		{ "fl", U'\uFB02' },
		{ "endash", U'\u2013' },
		{ "dagger", U'\u2020' },
		{ "daggerdbl", U'\u2021' },
		{ "periodcentered", U'\u00B7' },
		{ "paragraph", U'\u00B6' },
		{ "bullet", U'\u2022' },
		{ "quotesinglbase", U'\u201A' },
		{ "quotedblbase", U'\u201E' },
		{ "quotedblright", U'\u201D' },
		{ "guillemotright", U'\u00BB' },
		{ "ellipsis", U'\u2026' },
		{ "perthousand", U'\u2030' },
		{ "questiondown", U'\u00BF' },
		{ "grave", U'\u0060' },
		{ "acute", U'\u00B4' },
		{ "circumflex", U'\u02C6' },
		{ "tilde", U'\u02DC' },
		{ "macron", U'\u00AF' },
		{ "breve", U'\u02D8' },
		{ "dotaccent", U'\u02D9' },
		{ "dieresis", U'\u00A8' },
		{ "ring", U'\u02DA' },
		{ "cedilla", U'\u00B8' },
		{ "hungarumlaut", U'\u02DD' },
		{ "ogonek", U'\u02DB' },
		{ "caron", U'\u02C7' },
		{ "emdash", U'\u2014' },
		{ "AE", U'\u00C6' },
		{ "ordfeminine", U'\u00AA' },
		{ "Lslash", U'\u0141' },
		{ "Oslash", U'\u00D8' },
		{ "OE", U'\u0152' },
		{ "ordmasculine", U'\u00BA' },
		{ "ae", U'\u00E6' },
		{ "dotlessi", U'\u0131' },
		{ "lslash", U'\u0142' },
		{ "oslash", U'\u00F8' },
		{ "oe", U'\u0153' },
		{ "germandbls", U'\u00DF' },
	};

	char32_t get_codepoint_for_glyph_name(zst::str_view name)
	{
		if(auto it = g_glyph_names.find(name); it != g_glyph_names.end())
			return it->second;

		return 0;
	}
}
