// misc.cpp
// Copyright (c) 2022, zhiayang
// SPDX-License-Identifier: Apache-2.0

#include <array>

#include "pdf/builtin_font.h"

#include "./builtin_afms/times_roman.h"
#include "./builtin_afms/times_bold.h"
#include "./builtin_afms/times_italic.h"
#include "./builtin_afms/times_bold_italic.h"

#include "./builtin_afms/courier.h"
#include "./builtin_afms/courier_bold.h"
#include "./builtin_afms/courier_oblique.h"
#include "./builtin_afms/courier_bold_oblique.h"

#include "./builtin_afms/helvetica.h"
#include "./builtin_afms/helvetica_bold.h"
#include "./builtin_afms/helvetica_oblique.h"
#include "./builtin_afms/helvetica_bold_oblique.h"

#include "./builtin_afms/symbol.h"
#include "./builtin_afms/zapf_dingbats.h"

namespace pdf
{
	template <size_t N>
	constexpr static std::pair<const uint8_t*, size_t> split_array(const uint8_t (&arr)[N])
	{
		return { &arr[0], sizeof(arr) - 1 };
	}

	std::pair<const uint8_t*, size_t> get_compressed_afm(BuiltinFont::Core14 font)
	{
		using C14 = BuiltinFont::Core14;
		switch(font)
		{
			case C14::TimesRoman: return split_array(AFM_TIMES_ROMAN);
			case C14::TimesBold: return split_array(AFM_TIMES_BOLD);
			case C14::TimesItalic: return split_array(AFM_TIMES_ITALIC);
			case C14::TimesBoldItalic: return split_array(AFM_TIMES_BOLD_ITALIC);
			case C14::Courier: return split_array(AFM_COURIER);
			case C14::CourierBold: return split_array(AFM_COURIER_BOLD);
			case C14::CourierOblique: return split_array(AFM_COURIER_OBLIQUE);
			case C14::CourierBoldOblique: return split_array(AFM_COURIER_BOLD_OBLIQUE);
			case C14::Helvetica: return split_array(AFM_HELVETICA);
			case C14::HelveticaBold: return split_array(AFM_HELVETICA_BOLD);
			case C14::HelveticaOblique: return split_array(AFM_HELVETICA_OBLIQUE);
			case C14::HelveticaBoldOblique: return split_array(AFM_HELVETICA_BOLD_OBLIQUE);
			case C14::Symbol: return split_array(AFM_SYMBOL);
			case C14::ZapfDingbats: return split_array(AFM_ZAPF_DINGBATS);
		}
	}
}
