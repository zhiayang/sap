// fontset.h
// Copyright (c) 2022, zhiayang
// SPDX-License-Identifier: Apache-2.0

#pragma once

#include "pdf/font.h" // for Font

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
		FontSet(pdf::PdfFont* regular, pdf::PdfFont* italic, pdf::PdfFont* bold, pdf::PdfFont* bold_italic)
		    : m_regular_font(regular)
		    , m_italic_font(italic)
		    , m_bold_font(bold)
		    , m_bold_italic_font(bold_italic)
		{
		}

		pdf::PdfFont* getFontForStyle(FontStyle style) const
		{
			switch(style)
			{
				case FontStyle::Regular: return m_regular_font;
				case FontStyle::Italic: return m_italic_font;
				case FontStyle::Bold: return m_bold_font;
				case FontStyle::BoldItalic: return m_bold_italic_font;
			}
		}

		pdf::PdfFont* regular() const { return m_regular_font; }
		pdf::PdfFont* italic() const { return m_italic_font; }
		pdf::PdfFont* bold() const { return m_bold_font; }
		pdf::PdfFont* boldItalic() const { return m_bold_italic_font; }

		constexpr bool operator==(const FontSet&) const = default;

	private:
		pdf::PdfFont* m_regular_font;
		pdf::PdfFont* m_italic_font;
		pdf::PdfFont* m_bold_font;
		pdf::PdfFont* m_bold_italic_font;
	};
}
