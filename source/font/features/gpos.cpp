// gpos.cpp
// Copyright (c) 2021, zhiayang
// Licensed under the Apache License Version 2.0.

#include <cassert>

#include "util.h"
#include "error.h"
#include "font/font.h"
#include "font/features.h"

namespace font
{
	static GlyphAdjustment parse_value_record(zst::byte_span& buf, uint16_t format)
	{
		GlyphAdjustment ret { };

		if(format & 0x01)   ret.x_placement = consume_u16(buf);
		if(format & 0x02)   ret.y_placement = consume_u16(buf);
		if(format & 0x04)   ret.x_advance = consume_u16(buf);
		if(format & 0x08)   ret.y_advance = consume_u16(buf);
		if(format & 0x10)   consume_u16(buf);   // X_PLACEMENT_DEVICE
		if(format & 0x20)   consume_u16(buf);   // Y_PLACEMENT_DEVICE
		if(format & 0x40)   consume_u16(buf);   // X_ADVANCE_DEVICE
		if(format & 0x80)   consume_u16(buf);   // Y_ADVANCE_DEVICE

		return ret;
	}

	constexpr static size_t get_value_record_size(uint16_t format)
	{
		size_t ret = 0;
		if(format & 0x01)   ret += sizeof(uint16_t);
		if(format & 0x02)   ret += sizeof(uint16_t);
		if(format & 0x04)   ret += sizeof(uint16_t);
		if(format & 0x08)   ret += sizeof(uint16_t);
		if(format & 0x10)   ret += sizeof(uint16_t);
		if(format & 0x20)   ret += sizeof(uint16_t);
		if(format & 0x40)   ret += sizeof(uint16_t);
		if(format & 0x80)   ret += sizeof(uint16_t);
		return ret;
	}

	// returns -1 if not found.
	static int32_t get_coverage_index(zst::byte_span cov_table, uint32_t glyphId)
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

			return -1;
		}
		else if(format == 2)
		{
			struct RangeRecord { uint16_t start; uint16_t end; uint16_t cov_idx; } __attribute__((packed));
			static_assert(sizeof(RangeRecord) == 3 * sizeof(uint16_t));

			auto count = consume_u16(cov_table);
			auto array = cov_table.take(count * sizeof(RangeRecord)).cast<RangeRecord>();

			// binary search the RangeRecords
			size_t low = 0;
			size_t high = 0;
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

			return -1;
		}
		else
		{
			sap::internal_error("invalid OTF coverage table format ({})", format);
		}
	}

	static GlyphAdjustment scale_adjustments(GlyphAdjustment adj, int units_per_em)
	{
		adj.x_advance = (adj.x_advance * 1000) / units_per_em;
		adj.y_advance = (adj.y_advance * 1000) / units_per_em;
		adj.x_placement = (adj.x_placement * 1000) / units_per_em;
		adj.y_placement = (adj.y_placement * 1000) / units_per_em;

		return adj;
	}











	std::optional<std::pair<GlyphAdjustment, GlyphAdjustment>> FontFile::getGlyphPairAdjustments(uint32_t g1, uint32_t g2) const
	{
		return { };
	}

	std::optional<GlyphAdjustment> FontFile::getGlyphAdjustment(uint32_t glyphId) const
	{
		// look for the single adjustment lookup
		if(!this->gpos_tables.lookup_tables[GPOS_LOOKUP_SINGLE].present)
			return { };

		auto ofs = this->gpos_tables.lookup_tables[GPOS_LOOKUP_SINGLE].file_offset;
		auto buf = zst::byte_span(this->file_bytes, this->file_size).drop(ofs);

		auto start = buf;

		// we already know the type, so just skip it.
		consume_u16(buf);
		auto flags = consume_u16(buf);
		auto num_subs = consume_u16(buf);

		for(size_t i = 0; i < num_subs; i++)
		{
			auto ofs = consume_u16(buf);
			auto subtable = start.drop(ofs);
			auto subtable_start = subtable;

			auto format = consume_u16(subtable);
			auto cov_ofs = consume_u16(subtable);
			auto value_fmt = consume_u16(subtable);

			if(format != 1 && format != 2)
				sap::internal_error("unknown format {}", format);

			if(auto coverage_idx = get_coverage_index(subtable_start.drop(cov_ofs), glyphId); coverage_idx != -1)
			{
				if(format == 1)
				{
					return scale_adjustments(parse_value_record(subtable, value_fmt), this->metrics.units_per_em);
				}
				else
				{
					auto num_records = consume_u16(subtable);
					assert(coverage_idx < num_records);

					subtable.remove_prefix(get_value_record_size(value_fmt) * coverage_idx);
					return scale_adjustments(parse_value_record(subtable, value_fmt), this->metrics.units_per_em);
				}
			}
		}

		return { };
	}

	static void parse_lookup_table(FontFile* font, zst::byte_span buf)
	{
		auto start = buf;

		auto type = consume_u16(buf);
		consume_u16(buf);   // ignore the flags
		consume_u16(buf);   // and the subtable_count

		if(type >= GPOS_LOOKUP_MAX)
		{
			zpr::println("warning: invalid GPOS lookup table with type {}", type);
			return;
		}

		LookupTable tbl { };
		tbl.present = true;
		tbl.type = type;
		tbl.file_offset = start.data() - font->file_bytes;

		font->gpos_tables.lookup_tables[type] = std::move(tbl);
	}

	void parseGPOS(FontFile* font, const Table& gpos_table)
	{
		auto buf = zst::byte_span(font->file_bytes, font->file_size);
		buf.remove_prefix(gpos_table.offset);

		auto table_start = buf;

		auto major = consume_u16(buf);
		auto minor = consume_u16(buf);

		auto script_list = parseTaggedList(font, table_start.drop(consume_u16(buf)));
		auto feature_list = parseTaggedList(font, table_start.drop(consume_u16(buf)));

		auto lookup_list_ofs = consume_u16(buf);

		for(auto& script : script_list)
			zpr::println("script {}", script.tag.str());

		for(auto& feat : feature_list)
			zpr::println("feature {}", feat.tag.str());

		auto lookup_tables = table_start.drop(lookup_list_ofs);
		auto num_lookup_tables = consume_u16(lookup_tables);
		for(size_t i = 0; i < num_lookup_tables; i++)
		{
			auto offset = consume_u16(lookup_tables);
			parse_lookup_table(font, table_start.drop(lookup_list_ofs + offset));
		}
	}
}
