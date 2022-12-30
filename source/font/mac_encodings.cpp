// mac_encodings.cpp
// Copyright (c) 2022, zhiayang
// SPDX-License-Identifier: Apache-2.0

namespace misc
{
	static constexpr const char* g_mac_roman_to_utf8[128] = {
		"\u00C4", // Ä
		"\u00C5", // Å
		"\u00C7", // Ç
		"\u00C9", // É
		"\u00D1", // Ñ
		"\u00D6", // Ö
		"\u00DC", // Ü
		"\u00E1", // á
		"\u00E0", // à
		"\u00E2", // â
		"\u00E4", // ä
		"\u00E3", // ã
		"\u00E5", // å
		"\u00E7", // ç
		"\u00E9", // é
		"\u00E8", // è
		"\u00EA", // ê
		"\u00EB", // ë
		"\u00ED", // í
		"\u00EC", // ì
		"\u00EE", // î
		"\u00EF", // ï
		"\u00F1", // ñ
		"\u00F3", // ó
		"\u00F2", // ò
		"\u00F4", // ô
		"\u00F6", // ö
		"\u00F5", // õ
		"\u00FA", // ú
		"\u00F9", // ù
		"\u00FB", // û
		"\u00FC", // ü
		"\u2020", // †
		"\u00B0", // °
		"\u00A2", // ¢
		"\u00A3", // £
		"\u00A7", // §
		"\u2022", // •
		"\u00B6", // ¶
		"\u00DF", // ß
		"\u00AE", // ®
		"\u00A9", // ©
		"\u2122", // ™
		"\u00B4", // ´
		"\u00A8", // ¨
		"\u2260", // ≠
		"\u00C6", // Æ
		"\u00D8", // Ø
		"\u221E", // ∞
		"\u00B1", // ±
		"\u2264", // ≤
		"\u2265", // ≥
		"\u00A5", // ¥
		"\u00B5", // µ
		"\u2202", // ∂
		"\u2211", // ∑
		"\u220F", // ∏
		"\u03C0", // π
		"\u222B", // ∫
		"\u00AA", // ª
		"\u00BA", // º
		"\u03A9", // Ω
		"\u00E6", // æ
		"\u00F8", // ø
		"\u00BF", // ¿
		"\u00A1", // ¡
		"\u00AC", // ¬
		"\u221A", // √
		"\u0192", // ƒ
		"\u2248", // ≈
		"\u2206", // ∆
		"\u00AB", // «
		"\u00BB", // »
		"\u2026", // …
		"\u00A0", // (nbsp)
		"\u00C0", // À
		"\u00C3", // Ã
		"\u00D5", // Õ
		"\u0152", // Œ
		"\u0153", // œ
		"\u2013", // –
		"\u2014", // —
		"\u201C", // “
		"\u201D", // ”
		"\u2018", // ‘
		"\u2019", // ’
		"\u00F7", // ÷
		"\u25CA", // ◊
		"\u00FF", // ÿ
		"\u0178", // Ÿ
		"\u2044", // ⁄
		"\u20AC", // €
		"\u2039", // ‹
		"\u203A", // ›
		"\uFB01", // ﬁ
		"\uFB02", // ﬂ
		"\u2021", // ‡
		"\u00B7", // ·
		"\u201A", // ‚
		"\u201E", // „
		"\u2030", // ‰
		"\u00C2", // Â
		"\u00CA", // Ê
		"\u00C1", // Á
		"\u00CB", // Ë
		"\u00C8", // È
		"\u00CD", // Í
		"\u00CE", // Î
		"\u00CF", // Ï
		"\u00CC", // Ì
		"\u00D3", // Ó
		"\u00D4", // Ô
		"\uF8FF", // 
		"\u00D2", // Ò
		"\u00DA", // Ú
		"\u00DB", // Û
		"\u00D9", // Ù
		"\u0131", // ı
		"\u02C6", // ˆ
		"\u02DC", // ˜
		"\u00AF", // ¯
		"\u02D8", // ˘
		"\u02D9", // ˙
		"\u02DA", // ˚
		"\u00B8", // ¸
		"\u02DD", // ˝
		"\u02DB", // ˛
		"\u02C7", // ˇ
	};

	std::string convert_mac_roman_to_utf8(zst::byte_span str)
	{
		std::string ret {};
		ret.reserve(str.size());

		for(uint8_t b : str)
		{
			if(b <= 128)
				ret += (char) b;
			else
				ret += g_mac_roman_to_utf8[b - 128];
		}

		return ret;
	}
}
