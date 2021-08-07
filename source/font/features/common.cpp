// common.cpp
// Copyright (c) 2021, zhiayang
// Licensed under the Apache License Version 2.0.

#include "util.h"
#include "error.h"
#include "font/font.h"
#include "font/features.h"

namespace font
{
	std::vector<TaggedTable> parseTaggedList(FontFile* font, zst::byte_span list)
	{
		auto table_start = list;

		std::vector<TaggedTable> ret;

		auto num = consume_u16(list);
		for(size_t i = 0; i < num; i++)
		{
			auto tag = Tag(consume_u32(list));
			auto ofs = consume_u16(list);

			ret.push_back({ tag, static_cast<size_t>(ofs + (table_start.data() - font->file_bytes)) });
		}

		return ret;
	}

	LookupTable parseLookupTable(FontFile* font, zst::byte_span buf)
	{
		auto start = buf;

		auto type = consume_u16(buf);
		consume_u16(buf);   // ignore the flags
		consume_u16(buf);   // and the subtable_count

		LookupTable tbl { };
		tbl.type = type;
		tbl.file_offset = start.data() - font->file_bytes;

		return tbl;
	}

	std::map<int, uint32_t> parseCoverageTable(zst::byte_span cov_table)
	{
		auto format = consume_u16(cov_table);
		std::map<int, uint32_t> coverage_map;

		if(format == 1)
		{
			auto count = consume_u16(cov_table);

			// the first glyphid in the array has index 0, then 1, and so on.
			for(size_t i = 0; i < count; i++)
				coverage_map[i] = consume_u16(cov_table);
		}
		else if(format == 2)
		{
			auto num_ranges = consume_u16(cov_table);
			for(size_t i = 0; i < num_ranges; i++)
			{
				auto first = consume_u16(cov_table);
				auto last = consume_u16(cov_table);
				auto start_cov = consume_u16(cov_table);

				for(auto i = first; i < last + 1; i++)
					coverage_map[start_cov + (i - first)] = i;
			}
		}
		else
		{
			sap::internal_error("invalid OTF coverage table format ({})", format);
		}

		return coverage_map;
	}



	std::optional<int> getGlyphCoverageIndex(zst::byte_span cov_table, uint32_t glyphId)
	{
		auto format = consume_u16(cov_table);

		if(format == 1)
		{
			auto count = consume_u16(cov_table);
			auto array = cov_table.cast<uint16_t>();

			// binary search
			size_t low = 0;
			size_t high = count;

			while(low < high)
			{
				auto mid = (low + high) / 2u;
				auto val = util::convertBEU16(array[mid]);

				if(val == glyphId)
					return static_cast<int32_t>(mid);
				else if(val < glyphId)
					low = mid + 1;
				else
					high = mid;
			}

			return { };
		}
		else if(format == 2)
		{
			struct RangeRecord { uint16_t start; uint16_t end; uint16_t cov_idx; } __attribute__((packed));
			static_assert(sizeof(RangeRecord) == 3 * sizeof(uint16_t));

			auto count = consume_u16(cov_table);
			auto array = cov_table.take(count * sizeof(RangeRecord)).cast<RangeRecord>();

			// binary search the RangeRecords
			size_t low = 0;
			size_t high = count;
			while(low < high)
			{
				auto mid = (low + high) / 2u;
				auto val = array[mid];

				auto start = util::convertBEU16(val.start);
				auto end = util::convertBEU16(val.end);

				if(start <= glyphId && glyphId <= end)
					return static_cast<int32_t>(util::convertBEU16(val.cov_idx) + glyphId - start);
				else if(end < glyphId)
					low = mid + 1;
				else
					high = mid;
			}

			return { };
		}
		else
		{
			sap::internal_error("invalid OTF coverage table format ({})", format);
		}
	}
}
