// fontset.h
// Copyright (c) 2022, zhiayang
// SPDX-License-Identifier: Apache-2.0

#pragma once

#include "pdf/font.h"

namespace sap
{
	enum class FontStyle
	{
		Regular = 0,
		Italic,
		Bold,
		BoldItalic,
	};

	struct FontSet
	{
		FontSet(pdf::Font* regular, pdf::Font* italic, pdf::Font* bold, pdf::Font* bold_italic)
		    : m_regular_font(regular)
		    , m_italic_font(italic)
		    , m_bold_font(bold)
		    , m_bold_italic_font(bold_italic)
		{
		}

		pdf::Font* getFontForStyle(FontStyle style) const
		{
			switch(style)
			{
				case FontStyle::Regular:
					return m_regular_font;
				case FontStyle::Italic:
					return m_italic_font;
				case FontStyle::Bold:
					return m_bold_font;
				case FontStyle::BoldItalic:
					return m_bold_italic_font;
			}
		}

		pdf::Font* regular() const { return m_regular_font; }
		pdf::Font* italic() const { return m_italic_font; }
		pdf::Font* bold() const { return m_bold_font; }
		pdf::Font* boldItalic() const { return m_bold_italic_font; }

	private:
		pdf::Font* m_regular_font;
		pdf::Font* m_italic_font;
		pdf::Font* m_bold_font;
		pdf::Font* m_bold_italic_font;
	};
}
