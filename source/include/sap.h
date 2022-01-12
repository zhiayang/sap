// sap.h
// Copyright (c) 2021, zhiayang
// SPDX-License-Identifier: Apache-2.0

#include <cstddef>
#include <cstdint>

#include <vector>
#include <utility>

#include <zst.h>

#include "sap/style.h"

#include "pdf/units.h"
#include "pdf/document.h"

namespace pdf
{
	struct Font;
	struct Page;
	struct Document;
	struct Writer;
}

namespace sap
{
	using Scalar = pdf::Scalar;
	using Vector = pdf::Vector;

	/*
		for now, the Word acts as the smallest indivisible unit of text in sap; we do not plan to adjust
		intra-word (ie. letter) spacing at this time.

		since the Word is used as the unit of typesetting, it needs to hold and *cache* a bunch of important
		information, notably the bounding box (metrics) and the glyph ids, the former of which can only be
		computed with the latter.

		since we need to run ligature substitution and kerning adjustments to determine the bounding box,
		these will be done on a Word. Again, since letter spacing is not really changing here, these computed
		values will be cached so less has to be computed on the PDF layer.
	*/
	struct Word : Stylable
	{
		explicit inline Word(int kind, const Style* style = nullptr)
			: Stylable(style), kind(kind) { }

		inline Word(int kind, zst::str_view sv, const Style* style = nullptr)
			: Word(kind, sv.str(), style) { }

		inline Word(int kind, std::string str, const Style* style = nullptr)
			: Stylable(style), kind(kind), text(std::move(str)) { }

		// this fills in the `size` and the `glyphs`
		void computeMetrics(const Style* parent_style);

		int kind = 0;
		std::string text { };

		// these are in sap units, which is in mm. note that `position` is not set internally, but rather
		// is used by whoever is laying out the Word to store its position temporarily (instead of forcing
		// some external associative container)
		Vector position { };
		Vector size { };

		// first element is the glyph id, second one is the adjustment to make for kerning (0 if none)
		std::vector<std::pair<uint32_t, int>> glyphs { };

		// the kind of word. this affects automatic handling of certain things during paragraph
		// layout, eg. whether or not to insert a space (or how large of a space).
		static constexpr int KIND_LATIN = 0;
		static constexpr int KIND_CJK   = 1;
		static constexpr int KIND_PUNCT = 2;

		friend struct Paragraph;

	private:
		const Style* m_style;
	};


	// for now we are not concerned with lines.
	struct Paragraph : Stylable
	{
		explicit inline Paragraph(const Style* style = nullptr) : Stylable(style) { }

		void add(Word word);
		void computeMetrics(const Style* parent_style);

	private:
		std::vector<Word> m_words {};
	};


	struct Page : Stylable
	{
		explicit inline Page(const Style* style = nullptr) : Stylable(style) { }

		Page(const Page&) = delete;
		Page& operator= (const Page&) = delete;

		Page(Page&&) = default;
		Page& operator= (Page&&) = default;

		pdf::Page* finalise();

		// add a paragraph to the page, optionally returning the remainder if
		// the entire thing could not fit. `Paragraph` is a value type, so this
		// is totally fine without pointers.
		std::optional<Paragraph> add(Paragraph para);

		std::vector<Paragraph> m_paragraphs { };

	private:
	};

	struct Document : Stylable
	{
		Document();

		Document(const Document&) = delete;
		Document& operator= (const Document&) = delete;

		Document(Document&&) = default;
		Document& operator= (Document&&) = default;

		void add(Page&& para);

		pdf::Document& pdfDocument();
		const pdf::Document& pdfDocument() const;

		pdf::Document finalise();

	private:
		pdf::Document m_pdf_document {};
		std::vector<Page> m_pages {};
	};
}
