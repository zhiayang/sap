// cff.h
// Copyright (c) 2022, zhiayang
// SPDX-License-Identifier: Apache-2.0

#pragma once

#include <stdint.h>
#include <stddef.h>

#include <zpr.h>
#include <zst.h>

#include <vector>
#include <optional>
#include <unordered_set>

#include "types.h"

namespace font
{
	struct FontFile;
	struct GlyphMetrics;
}

namespace font::cff
{
	// these are all 32-bit, because that's the maximum size it's specified to be.
	// no reason to use larger types than we'll ever need.
	struct IndexTable
	{
		uint16_t count;
		uint8_t offset_bytes;

		// of course, since we don't know beforehand the offset size, just use the largest (4 bytes)
		std::vector<uint32_t> offsets;

		zst::byte_span data;

		inline zst::byte_span get_item(size_t idx) const
		{
			if(idx >= this->count)
				return {};

			return data.drop(offsets[idx]).take(offsets[idx + 1] - offsets[idx]);
		}
	};

	struct Operand
	{
		int type;
		union
		{
			int32_t _integer;
			double _decimal;
		};

		inline uint16_t string_id() const { return _integer & 0xFFFF; }
		inline int32_t integer() const { return _integer; }
		inline double decimal() const { return _decimal; }
		inline double fixed() const { return ((_integer >> 16) & 0xFFFF) + (_integer & 0xFFFF) / 65536.0; }

		inline Operand& string_id(uint16_t value)
		{
			type = TYPE_INTEGER;
			_integer = value;
			return *this;
		}
		inline Operand& integer(int32_t value)
		{
			type = TYPE_INTEGER;
			_integer = value;
			return *this;
		}
		inline Operand& decimal(double value)
		{
			type = TYPE_DECIMAL;
			_decimal = value;
			return *this;
		}
		inline Operand& fixed(int32_t value)
		{
			type = TYPE_FIXED;
			_integer = value;
			return *this;
		}

		inline bool operator==(const Operand& other) const
		{
			return this->type == other.type &&
			       (this->type == TYPE_DECIMAL ? (this->_decimal == other._decimal) : (this->_integer == other._integer));
		}

		inline bool operator!=(const Operand& other) const { return !(*this == other); }

		static constexpr int TYPE_INTEGER = 0;
		static constexpr int TYPE_DECIMAL = 1;
		static constexpr int TYPE_FIXED = 2;
	};

	enum class DictKey : uint16_t;
	struct Dictionary
	{
		std::map<DictKey, std::vector<Operand>> values;

		inline bool contains(DictKey key) const { return values.find(key) != values.end(); }

		std::vector<Operand> get(DictKey key) const;
		uint16_t string_id(DictKey key) const;
		int32_t integer(DictKey key) const;
		double decimal(DictKey key) const;
		double fixed(DictKey key) const;
	};

	struct Subroutine
	{
		zst::byte_span charstring {};
		bool used = false;
	};


	struct Glyph
	{
		uint16_t gid;

		uint16_t cid;
		zst::str_view glyph_name;
		zst::byte_span charstring;

		// which Font DICT to use, for CID fonts.
		uint8_t font_dict_idx;
	};

	struct FontDict
	{
		Dictionary dict {};
		Dictionary private_dict {};
		std::vector<Subroutine> local_subrs {};
	};

	/*
	    Note: this in-memory representation is only suitable for CFF fonts embedded in OTF files,
	    and *NOT* for general-purpose CFF files.

	    For example, we only support 1 Top DICT (ie. no fontsets)
	*/
	struct CFFData
	{
		zst::byte_span bytes {};

		bool cff2 = false;
		bool is_cidfont = false;

		zst::byte_span name {};

		IndexTable charstrings_table {};

		std::optional<zst::byte_span> encoding_data {};

		std::optional<IndexTable> fdarray_table {};
		std::optional<zst::byte_span> fdselect_data {};

		Dictionary top_dict {};
		std::vector<Subroutine> global_subrs {};

		std::vector<Glyph> glyphs {};

		std::vector<std::string> string_ids {};
		std::map<std::string, uint16_t> known_strings {};

		// Font DICTs referenced in the FDArray for CID fonts. For non-CID fonts, there is always
		// one FD in here, referencing the top-level Private and local_subrs data.
		std::vector<FontDict> font_dicts {};

		uint16_t get_or_add_string(zst::str_view str);
		zst::str_view get_string(uint16_t sid) const;
	};


	/*
	    Parse CFF data from the given buffer
	*/
	CFFData* parseCFFData(FontFile* font, zst::byte_span cff_data);

	/*
	    Read a number from a *Type 2* CharString. For the 5-byte encoding which represents a
	    16.16 fixed point, the high 16 bits of the i32 are the integer part, and the low 16 bits
	    are the fractional part.

	    If the first byte in the buffer is not one of the number operand bytes, then an empty
	    optional is returned; this allows decoding optional operands.
	*/
	std::optional<Operand> readNumberFromCharString(zst::byte_span& buf);

	/*
	    Read a number operand from a DICT data stream.
	*/
	std::optional<Operand> readNumberFromDICT(zst::byte_span& buf);

	/*
	    Read operands from a DICT, up to (but excluding) the key operand itself.
	*/
	std::vector<Operand> readOperandsFromDICT(zst::byte_span& buf);

	/*
	    Read operands from a Type 2 CharString, up to (but excluding) the command itself
	*/
	std::vector<Operand> readOperandsFromCharString(zst::byte_span& buf);

	/*
	    Read a DICT, extracting all keys and values.
	*/
	Dictionary readDictionary(zst::byte_span dict);

	/*
	    Read an INDEX, returning the total size of the table (including the header and all data)
	*/
	IndexTable readIndexTable(zst::byte_span buf, size_t* total_size = nullptr);

	/*
	    Fill the dictionary with default values for keys that didn't exist in the file.
	*/
	void populateDefaultValuesForTopDict(Dictionary& dict);

	/*
	    Returns (optionally) the default value for the given dictionary key.
	*/
	std::optional<std::vector<Operand>> getDefaultValueForDictKey(DictKey key);

	/*
	    Read the charset from the buffer, returning a mapping from glyph IDs to glyph names (SIDs) or CIDs
	*/
	std::map<uint16_t, uint16_t> readCharsetTable(size_t num_glyphs, zst::byte_span dict);

	/*
	    Create the cmap corresponding to the subset CFF file. Don't call this directly.
	*/
	zst::byte_buffer createCMapForCFFSubset(FontFile* file);

	/*
	    Get one of the predefined charsets, returning the mapping from glyph ID to SIDs.

	    0 = ISOAdobe
	    1 = Expert
	    2 = ExpertSubset
	*/
	std::map<uint16_t, uint16_t> getPredefinedCharset(int num);

	/*
	    Interpret the given charstring, and mark any used subroutines.
	*/
	void interpretCharStringAndMarkSubrs(zst::byte_span charstring, std::vector<Subroutine>& global_subrs,
		std::vector<Subroutine>& local_subrs);
}

namespace font::cff
{
	struct CFFSubset
	{
		zst::byte_buffer cff;
		zst::byte_buffer cmap;
	};


	/*
	    Subset the CFF font (given in `file`), including only the used_glyphs. Returns a new CFF and cmap table
	    for embedding into the OTF font.
	*/
	CFFSubset createCFFSubset(FontFile* file, zst::str_view subset_name, const std::unordered_set<GlyphId>& used_glyphs);


	/*
	    Build an INDEX by appending the following data items.
	*/
	struct IndexTableBuilder
	{
		IndexTableBuilder& add(zst::byte_span data);
		IndexTableBuilder& add(const IndexTable& table);
		void writeInto(zst::byte_buffer& buf);

	private:
		std::vector<uint32_t> m_offsets {};
		zst::byte_buffer m_data {};
	};


	/*
	    Build a DICT
	*/
	struct DictBuilder
	{
		DictBuilder();
		explicit DictBuilder(const Dictionary& from_dict);

		DictBuilder& set(DictKey key, std::vector<Operand> values);
		DictBuilder& setInteger(DictKey key, int32_t value);
		DictBuilder& setStringId(DictKey key, uint16_t value);
		DictBuilder& setIntegerPair(DictKey key, int32_t a, int32_t b);
		DictBuilder& erase(DictKey key);

		void writeInto(zst::byte_buffer& buf);
		zst::byte_buffer serialise();
		size_t computeSize();

	private:
		std::map<DictKey, std::vector<Operand>> m_values;
	};
}






template <>
struct zpr::print_formatter<font::cff::Operand>
{
	template <typename Cb>
	void print(font::cff::Operand op, Cb&& cb, format_args args)
	{
		using namespace font::cff;

		if(op.type == Operand::TYPE_INTEGER || op.type == Operand::TYPE_FIXED)
			detail::print_one(static_cast<Cb&&>(cb), static_cast<format_args&&>(args), op.integer());
		else if(op.type == Operand::TYPE_FIXED)
			detail::print_one(static_cast<Cb&&>(cb), static_cast<format_args&&>(args), op.fixed());
		else
			detail::print_one(static_cast<Cb&&>(cb), static_cast<format_args&&>(args), op.decimal());
	}
};


// CFF DICT operators
namespace font::cff
{
	static constexpr size_t NUM_STANDARD_STRINGS = 391;

	// keys are encoded as 0xAABB, where AA is 00 if the operator is 1 byte (then BB is simply
	// that byte), and AA is 0C if the operator is 2 bytes (then BB is the second byte)
	enum class DictKey : uint16_t
	{
		// 1-byte operators
		version = 0x00'00,
		Notice = 0x00'01,
		FullName = 0x00'02,
		FamilyName = 0x00'03,
		Weight = 0x00'04,
		FontBBox = 0x00'05,
		BlueValues = 0x00'06,
		OtherBlues = 0x00'07,
		FamilyBlues = 0x00'08,
		FamilyOtherBlues = 0x00'09,
		StdHW = 0x00'0A,
		StdVW = 0x00'0B,
		UniqueID = 0x00'0D,
		XUID = 0x00'0E,
		charset = 0x00'0F,
		Encoding = 0x00'10,
		CharStrings = 0x00'11,
		Private = 0x00'12,
		Subrs = 0x00'13,
		defaultWidthX = 0x00'14,
		nominalWidthX = 0x00'15,

		// 2-byte operators
		Copyright = 0x0C'00,
		isFixedPitch = 0x0C'01,
		ItalicAngle = 0x0C'02,
		UnderlinePosition = 0x0C'03,
		UnderlineThickness = 0x0C'04,
		PaintType = 0x0C'05,
		CharstringType = 0x0C'06,
		FontMatrix = 0x0C'07,
		StrokeWidth = 0x0C'08,
		BlueScale = 0x0C'09,
		BlueShift = 0x0C'0A,
		BlueFuzz = 0x0C'0B,
		StemSnapH = 0x0C'0C,
		StemSnapV = 0x0C'0D,
		ForceBold = 0x0C'0E, // 0f and 10 are reserved
		LanguageGroup = 0x0C'11,
		ExpansionFactor = 0x0C'12,
		initialRandomSeed = 0x0C'13,
		SyntheticBase = 0x0C'14,
		PostScript = 0x0C'15,
		BaseFontName = 0x0C'16,
		BaseFontBlend = 0x0C'17,
		ROS = 0x0C'1e, // 18 to 1d are reserved
		CIDFontVersion = 0x0C'1f,
		CIDFontRevision = 0x0C'20,
		CIDFontType = 0x0C'21,
		CIDCount = 0x0C'22,
		UIDBase = 0x0C'23,
		FDArray = 0x0C'24,
		FDSelect = 0x0C'25,
		FontName = 0x0C'26,
	};
}
