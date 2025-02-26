// aat_lookup.cpp
// Copyright (c) 2022, yuki / zhiayang
// SPDX-License-Identifier: Apache-2.0

#include "font/aat.h"
#include "font/misc.h"

namespace font::aat
{
	static auto parse_binsearch_header(zst::byte_span& buf, size_t mandatory_segment_bytes)
	{
		auto elem_size = consume_u16(buf);
		auto num_elems = consume_u16(buf);
		buf.remove_prefix(3 * sizeof(uint16_t));

		auto lookup_value_size = elem_size - mandatory_segment_bytes;

		assert(lookup_value_size <= sizeof(uint64_t));
		assert(lookup_value_size >= sizeof(uint8_t));

		struct
		{
			size_t num_elems;
			size_t lookup_size;
		} ret { .num_elems = num_elems, .lookup_size = lookup_value_size };

		return ret;
	}

	static uint64_t read_lookup(zst::byte_span& buf, size_t lookup_size)
	{
		switch(lookup_size)
		{
			case 1: return consume_u8(buf);
			case 2: return consume_u16(buf);
			case 4: return consume_u32(buf);
			case 8: return consume_u64(buf);
		}

		util::unreachable();
	}


	static Lookup read_lookup_f0(zst::byte_span buf, size_t num_glyphs)
	{
		Lookup ret {};
		for(size_t i = 0; i < num_glyphs; i++)
			ret.map[GlyphId(i)] = consume_u16(buf);

		return ret;
	}

	static Lookup read_lookup_f2(zst::byte_span buf)
	{
		auto [num_elems, lookup_size] = parse_binsearch_header(buf, 2 * sizeof(uint16_t));

		Lookup ret {};
		for(auto i = 0u; i < num_elems; i++)
		{
			auto last_glyph = consume_u16(buf);
			auto first_glyph = consume_u16(buf);

			auto lookup_value = read_lookup(buf, lookup_size);
			for(uint16_t g = first_glyph; g <= last_glyph; g++)
				ret.map[GlyphId(g)] = lookup_value;
		}

		return ret;
	}

	static Lookup read_lookup_f4(zst::byte_span buf, zst::byte_span table_start)
	{
		// similar to format 2 but with one more indirection.
		auto [num_elems, lookup_size] = parse_binsearch_header(buf, 2 * sizeof(uint16_t));
		assert(lookup_size == sizeof(uint16_t));

		Lookup ret {};
		for(auto i = 0u; i < num_elems; i++)
		{
			auto last_glyph = consume_u16(buf);
			auto first_glyph = consume_u16(buf);

			auto offset = consume_u16(buf);
			auto data = table_start.drop(offset);

			for(uint16_t g = first_glyph; g <= last_glyph; g++)
				ret.map[GlyphId(g)] = consume_u16(data);
		}

		return ret;
	}

	static Lookup read_lookup_f6(zst::byte_span buf)
	{
		auto [num_elems, lookup_size] = parse_binsearch_header(buf, sizeof(uint16_t));

		Lookup ret {};
		for(auto i = 0u; i < num_elems; i++)
		{
			auto gid = consume_u16(buf);
			auto val = read_lookup(buf, lookup_size);
			ret.map[GlyphId(gid)] = val;
		}

		return ret;
	}

	static Lookup read_lookup_f8(zst::byte_span buf)
	{
		auto first_glyph = consume_u16(buf);
		auto num_glyphs = consume_u16(buf);

		Lookup ret {};
		for(auto i = 0u; i < num_glyphs; i++)
			ret.map[GlyphId(first_glyph + i)] = consume_u16(buf);

		return ret;
	}

	static Lookup read_lookup_f10(zst::byte_span buf)
	{
		auto elem_size = consume_u16(buf);
		assert(elem_size == 1 || elem_size == 2 || elem_size == 4 || elem_size == 8);

		auto first_glyph = consume_u16(buf);
		auto num_glyphs = consume_u16(buf);

		Lookup ret {};
		for(auto i = 0u; i < num_glyphs; i++)
			ret.map[GlyphId(first_glyph + i)] = read_lookup(buf, elem_size);

		return ret;
	}

	std::optional<Lookup> parseLookupTable(zst::byte_span buf, size_t num_glyphs)
	{
		auto table_start = buf;

		auto format = consume_u16(buf);
		switch(format)
		{
			case 0: return read_lookup_f0(buf, num_glyphs);
			case 2: return read_lookup_f2(buf);
			case 4: return read_lookup_f4(buf, table_start);
			case 6: return read_lookup_f6(buf);
			case 8: return read_lookup_f8(buf);
			case 10: return read_lookup_f10(buf);
			default: //
				sap::warn("font/aat", "unknown LookupTable format {}", format);
				return std::nullopt;
		}
	}









	static std::optional<uint64_t> search_lookup_f0(zst::byte_span buf, size_t num_glyphs, GlyphId target)
	{
		if(static_cast<size_t>(target) >= num_glyphs)
			return std::nullopt;

		return peek_u16(buf.drop(static_cast<size_t>(target) * sizeof(uint16_t)));
	}

	static std::optional<uint64_t> search_lookup_f2(zst::byte_span buf, GlyphId target)
	{
		auto [num_elems, lookup_size] = parse_binsearch_header(buf, 2 * sizeof(uint16_t));
		auto RecordSize = lookup_size + 2 * sizeof(uint16_t);

		size_t low = 0;
		size_t high = num_elems;

		while(low < high)
		{
			auto mid = (low + high) / 2u;
			auto last = GlyphId(peek_u16(buf.drop(mid * RecordSize)));
			auto first = GlyphId(peek_u16(buf.drop(mid * RecordSize + sizeof(uint16_t))));

			if(first <= target && target <= last)
			{
				auto tmp = buf.drop(mid * RecordSize + 2 * sizeof(uint16_t));
				return read_lookup(tmp, lookup_size);
			}
			else if(target > last)
			{
				low = mid + 1;
			}
			else
			{
				high = mid;
			}
		}

		return std::nullopt;
	}

	static std::optional<uint64_t> search_lookup_f4(zst::byte_span buf, zst::byte_span table_start, GlyphId target)
	{
		// similar to format 2 but with one more indirection.
		auto [num_elems, lookup_size] = parse_binsearch_header(buf, 2 * sizeof(uint16_t));
		assert(lookup_size == sizeof(uint16_t));

		auto RecordSize = 3 * sizeof(uint16_t);

		size_t low = 0;
		size_t high = num_elems;

		while(low < high)
		{
			auto mid = (low + high) / 2u;
			auto last = GlyphId(peek_u16(buf.drop(mid * RecordSize)));
			auto first = GlyphId(peek_u16(buf.drop(mid * RecordSize + sizeof(uint16_t))));

			if(first <= target && target <= last)
			{
				auto offset = peek_u16(buf.drop(mid * RecordSize + 2 * sizeof(uint16_t)));
				auto data = table_start.drop(offset);

				auto tmp = data.drop(static_cast<size_t>(target - first) * sizeof(uint16_t));
				return peek_u16(tmp);
			}
			else if(target > last)
			{
				low = mid + 1;
			}
			else
			{
				high = mid;
			}
		}

		return std::nullopt;
	}

	static std::optional<uint64_t> search_lookup_f6(zst::byte_span buf, GlyphId target)
	{
		auto [num_elems, lookup_size] = parse_binsearch_header(buf, sizeof(uint16_t));
		auto RecordSize = lookup_size + sizeof(uint16_t);

		size_t low = 0;
		size_t high = num_elems;

		while(low < high)
		{
			auto mid = (low + high) / 2u;
			auto glyph = GlyphId(peek_u16(buf.drop(mid * RecordSize)));

			if(target == glyph)
			{
				auto tmp = buf.drop(mid * RecordSize + sizeof(uint16_t));
				return read_lookup(tmp, lookup_size);
			}
			else if(target > glyph)
			{
				low = mid + 1;
			}
			else
			{
				high = mid;
			}
		}

		return std::nullopt;
	}

	static std::optional<uint64_t> search_lookup_f8(zst::byte_span buf, GlyphId target)
	{
		auto first_glyph = GlyphId(consume_u16(buf));
		auto num_glyphs = consume_u16(buf);

		if(target < first_glyph || (first_glyph + num_glyphs) <= target)
			return std::nullopt;

		return peek_u16(buf.drop(static_cast<size_t>(target - first_glyph) * sizeof(uint16_t)));
	}

	static std::optional<uint64_t> search_lookup_f10(zst::byte_span buf, GlyphId target)
	{
		auto elem_size = consume_u16(buf);
		assert(elem_size == 1 || elem_size == 2 || elem_size == 4 || elem_size == 8);

		auto first_glyph = GlyphId(consume_u16(buf));
		auto num_glyphs = consume_u16(buf);

		if(target < first_glyph || (first_glyph + num_glyphs) <= target)
			return std::nullopt;

		auto tmp = buf.drop(static_cast<size_t>(target - first_glyph) * elem_size);
		return read_lookup(tmp, elem_size);
	}

	std::optional<uint64_t> searchLookupTable(zst::byte_span table, size_t num_font_glyphs, GlyphId target)
	{
		auto table_start = table;

		auto format = consume_u16(table);
		switch(format)
		{
			case 0: return search_lookup_f0(table, num_font_glyphs, target);
			case 2: return search_lookup_f2(table, target);
			case 4: return search_lookup_f4(table, table_start, target);
			case 6: return search_lookup_f6(table, target);
			case 8: return search_lookup_f8(table, target);
			case 10: return search_lookup_f10(table, target);
			default: //
				sap::warn("font/aat", "unknown LookupTable format {}", format);
				return std::nullopt;
		}
	}
}
