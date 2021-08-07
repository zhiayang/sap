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

		if(format & 0x01)   ret.horz_placement = consume_u16(buf);
		if(format & 0x02)   ret.vert_placement = consume_u16(buf);
		if(format & 0x04)   ret.horz_advance = consume_u16(buf);
		if(format & 0x08)   ret.vert_advance = consume_u16(buf);
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

	static GlyphAdjustment scale_adjustments(GlyphAdjustment adj, int units_per_em)
	{
		adj.horz_advance = (adj.horz_advance * 1000) / units_per_em;
		adj.vert_advance = (adj.vert_advance * 1000) / units_per_em;
		adj.horz_placement = (adj.horz_placement * 1000) / units_per_em;
		adj.vert_placement = (adj.vert_placement * 1000) / units_per_em;

		return adj;
	}











	std::optional<std::pair<GlyphAdjustment, GlyphAdjustment>> FontFile::getGlyphPairAdjustments(uint32_t gid1, uint32_t gid2) const
	{
		// look for the single adjustment lookup
		if(!this->gpos_tables.lookup_tables[GPOS_LOOKUP_PAIR].present)
			return { };

		auto ofs = this->gpos_tables.lookup_tables[GPOS_LOOKUP_PAIR].file_offset;
		auto buf = zst::byte_span(this->file_bytes, this->file_size).drop(ofs);

		auto table_start = buf;

		// we already know the type, so just skip it.
		consume_u16(buf);
		auto flags = consume_u16(buf);
		auto num_subs = consume_u16(buf);

		(void) flags;

		for(size_t i = 0; i < num_subs; i++)
		{
			auto ofs = consume_u16(buf);
			auto subtable = table_start.drop(ofs);
			auto subtable_start = subtable;

			auto format = consume_u16(subtable);
			auto cov_ofs = consume_u16(subtable);
			auto value_fmt1 = consume_u16(subtable);
			auto value_fmt2 = consume_u16(subtable);

			if(format != 1 && format != 2)
				sap::internal_error("unknown format {}", format);

			// the coverage table only lists the first glyph id.
			if(auto coverage_idx = getGlyphCoverageIndex(subtable_start.drop(cov_ofs), gid1); coverage_idx.has_value())
			{
				if(format == 1)
				{
					auto num_pair_sets = consume_u16(subtable);
					assert(*coverage_idx < num_pair_sets);

				#if 0
					// this thing doesn't use offsets, so we can only iterate.
					for(size_t i = 0; i < static_cast<size_t>(coverage_idx); i++)
					{
						auto ofs = consume_u16(subtable);
						auto num_pairs = consume_u16(subtable);
						subtable.remove_prefix(num_pairs * PairRecordSize);
					}
				#endif

					const auto PairRecordSize = sizeof(uint16_t)
						+ get_value_record_size(value_fmt1)
						+ get_value_record_size(value_fmt2);

					auto pairset_offset = peek_u16(subtable.drop(*coverage_idx * sizeof(uint16_t)));
					auto pairset_table = subtable_start.drop(pairset_offset);

					auto num_pairs = consume_u16(pairset_table);

					// now, binary search the second set.
					size_t low = 0;
					size_t high = num_pairs;

					while(low < high)
					{
						auto mid = (low + high) / 2u;
						auto glyph = peek_u16(pairset_table.drop(mid * PairRecordSize));

						if(glyph == gid2)
						{
							auto tmp = pairset_table.drop(mid * PairRecordSize);
							auto a1 = scale_adjustments(parse_value_record(tmp, value_fmt1), this->metrics.units_per_em);
							auto a2 = scale_adjustments(parse_value_record(tmp, value_fmt2), this->metrics.units_per_em);
							return std::make_pair(a1, a2);
						}
						else if(glyph < gid2)
						{
							low = mid + 1;
						}
						else
						{
							high = mid;
						}
					}
				}
				else
				{
					auto cls_ofs1 = consume_u16(subtable);
					auto cls_ofs2 = consume_u16(subtable);

					auto num_cls1 = consume_u16(subtable);
					auto num_cls2 = consume_u16(subtable);

					auto g1_class = getGlyphClass(subtable_start.drop(cls_ofs1), gid1);
					auto g2_class = getGlyphClass(subtable_start.drop(cls_ofs2), gid2);

					// note that num_cls1/2 include class 0
					if(g1_class < num_cls1 && g2_class < num_cls2)
					{
						const auto RecordSize = get_value_record_size(value_fmt1) + get_value_record_size(value_fmt2);

						// skip all the way to the correct Class2Record
						auto cls2_start = subtable.drop(g1_class * num_cls2 * RecordSize);
						auto pair_start = cls2_start.drop(g2_class * RecordSize);

						auto a1 = scale_adjustments(parse_value_record(pair_start, value_fmt1), this->metrics.units_per_em);
						auto a2 = scale_adjustments(parse_value_record(pair_start, value_fmt2), this->metrics.units_per_em);

						return std::make_pair(a1, a2);
					}
				}
			}
		}

		return { };
	}

	std::optional<GlyphAdjustment> FontFile::getGlyphAdjustment(uint32_t glyphId) const
	{
		// look for the single adjustment lookup
		if(!this->gpos_tables.lookup_tables[GPOS_LOOKUP_SINGLE].present)
			return { };

		auto ofs = this->gpos_tables.lookup_tables[GPOS_LOOKUP_SINGLE].file_offset;
		auto buf = zst::byte_span(this->file_bytes, this->file_size).drop(ofs);

		auto table_start = buf;

		// we already know the type, so just skip it.
		consume_u16(buf);
		auto flags = consume_u16(buf);
		auto num_subs = consume_u16(buf);
		(void) flags;

		for(size_t i = 0; i < num_subs; i++)
		{
			auto ofs = consume_u16(buf);
			auto subtable = table_start.drop(ofs);
			auto subtable_start = subtable;

			auto format = consume_u16(subtable);
			auto cov_ofs = consume_u16(subtable);
			auto value_fmt = consume_u16(subtable);

			if(format != 1 && format != 2)
				sap::internal_error("unknown format {}", format);

			if(auto coverage_idx = getGlyphCoverageIndex(subtable_start.drop(cov_ofs), glyphId); coverage_idx.has_value())
			{
				if(format == 1)
				{
					return scale_adjustments(parse_value_record(subtable, value_fmt), this->metrics.units_per_em);
				}
				else
				{
					auto num_records = consume_u16(subtable);
					assert(coverage_idx < num_records);

					subtable.remove_prefix(get_value_record_size(value_fmt) * *coverage_idx);
					return scale_adjustments(parse_value_record(subtable, value_fmt), this->metrics.units_per_em);
				}
			}
		}

		return { };
	}

	void parseGPos(FontFile* font, const Table& gpos_table)
	{
		auto buf = zst::byte_span(font->file_bytes, font->file_size);
		buf.remove_prefix(gpos_table.offset);

		auto table_start = buf;

		auto major = consume_u16(buf);
		auto minor = consume_u16(buf);

		if(major != 1 || (minor != 0 && minor != 1))
		{
			zpr::println("warning: unsupported GPOS table version {}.{}, ignoring", major, minor);
			return;
		}

		auto script_list = parseTaggedList(font, table_start.drop(consume_u16(buf)));
		auto feature_list = parseTaggedList(font, table_start.drop(consume_u16(buf)));

		auto lookup_list_ofs = consume_u16(buf);

		auto lookup_tables = table_start.drop(lookup_list_ofs);
		auto num_lookup_tables = consume_u16(lookup_tables);
		for(size_t i = 0; i < num_lookup_tables; i++)
		{
			auto offset = consume_u16(lookup_tables);
			auto tbl = parseLookupTable(font, table_start.drop(lookup_list_ofs + offset));

			if(tbl.type >= GPOS_LOOKUP_MAX)
				zpr::println("warning: invalid GPOS lookup table with type {}", tbl.type);
			else
				font->gpos_tables.lookup_tables[tbl.type] = std::move(tbl);
		}
	}
}
