// coverage.cpp
// Copyright (c) 2022, yuki
// SPDX-License-Identifier: Apache-2.0

#include "util.h"
#include "types.h"

#include "font/off.h"
#include "font/misc.h"
#include "font/features.h"

namespace font::off
{
	std::map<int, GlyphId> parseCoverageTable(zst::byte_span cov_table)
	{
		auto format = consume_u16(cov_table);
		std::map<int, GlyphId> coverage_map {};

		if(format == 1)
		{
			auto count = consume_u16(cov_table);

			// the first glyphid in the array has index 0, then 1, and so on.
			for(int i = 0; i < count; i++)
				coverage_map[i] = GlyphId { consume_u16(cov_table) };
		}
		else if(format == 2)
		{
			auto num_ranges = consume_u16(cov_table);
			for(size_t i = 0; i < num_ranges; i++)
			{
				auto first = consume_u16(cov_table);
				auto last = consume_u16(cov_table);
				auto start_cov = consume_u16(cov_table);

				for(auto k = first; k < last + 1; k++)
					coverage_map[start_cov + (k - first)] = GlyphId { k };
			}
		}
		else
		{
			sap::internal_error("invalid OTF coverage table format ({})", format);
		}

		return coverage_map;
	}

	std::optional<size_t> getGlyphCoverageIndex(zst::byte_span cov_table, GlyphId glyphId)
	{
		auto format = consume_u16(cov_table);

		auto gid16 = static_cast<uint16_t>(glyphId);
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

				if(val == gid16)
					return mid;
				else if(val < gid16)
					low = mid + 1;
				else
					high = mid;
			}

			return {};
		}
		else if(format == 2)
		{
			struct RangeRecord
			{
				uint16_t start;
				uint16_t end;
				uint16_t cov_idx;
			} __attribute__((packed));
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

				if(start <= gid16 && gid16 <= end)
					return static_cast<int32_t>(util::convertBEU16(val.cov_idx) + gid16 - start);
				else if(end < gid16)
					low = mid + 1;
				else
					high = mid;
			}

			return {};
		}
		else
		{
			sap::internal_error("invalid OTF coverage table format ({})", format);
		}
	}
}
