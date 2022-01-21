// gpos.cpp
// Copyright (c) 2021, zhiayang
// SPDX-License-Identifier: Apache-2.0

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










	std::optional<std::pair<GlyphAdjustment, GlyphAdjustment>> FontFile::getGlyphPairAdjustments(uint32_t gid1, uint32_t gid2) const
	{
		for(auto& lookup : this->gpos_tables.lookup_tables)
		{
			if(lookup.type != GPOS_LOOKUP_PAIR)
				continue;

			for(auto ofs : lookup.subtable_offsets)
			{
				auto subtable = lookup.data.drop(ofs);
				auto subtable_start = subtable;

				auto format = consume_u16(subtable);
				auto cov_ofs = consume_u16(subtable);
				auto value_fmt1 = consume_u16(subtable);
				auto value_fmt2 = consume_u16(subtable);

				if(format != 1 && format != 2)
					sap::internal_error("unknown format {}", format);

				// the coverage table only lists the first glyph id.
				if(auto cov_idx = off::getGlyphCoverageIndex(subtable_start.drop(cov_ofs), gid1); cov_idx.has_value())
				{
					if(format == 1)
					{
						auto num_pair_sets = consume_u16(subtable);
						assert(*cov_idx < num_pair_sets);

						const auto PairRecordSize = sizeof(uint16_t)
							+ get_value_record_size(value_fmt1)
							+ get_value_record_size(value_fmt2);

						auto pairset_offset = peek_u16(subtable.drop(*cov_idx * sizeof(uint16_t)));
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
								// we need to drop the glyph id itself to get to the actual data
								auto tmp = pairset_table.drop(mid * PairRecordSize).drop(sizeof(uint16_t));

								auto a1 = parse_value_record(tmp, value_fmt1);
								auto a2 = parse_value_record(tmp, value_fmt2);
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

						auto g1_class = off::getGlyphClass(subtable_start.drop(cls_ofs1), gid1);
						auto g2_class = off::getGlyphClass(subtable_start.drop(cls_ofs2), gid2);

						// note that num_cls1/2 include class 0
						if(g1_class < num_cls1 && g2_class < num_cls2)
						{
							const auto RecordSize = get_value_record_size(value_fmt1) + get_value_record_size(value_fmt2);

							// skip all the way to the correct Class2Record
							auto cls2_start = subtable.drop(g1_class * num_cls2 * RecordSize);
							auto pair_start = cls2_start.drop(g2_class * RecordSize);

							auto a1 = parse_value_record(pair_start, value_fmt1);
							auto a2 = parse_value_record(pair_start, value_fmt2);

							return std::make_pair(a1, a2);
						}
					}
				}
			}
		}

		return { };
	}

	std::optional<GlyphAdjustment> FontFile::getGlyphAdjustment(uint32_t glyphId) const
	{
		for(auto& lookup : this->gpos_tables.lookup_tables)
		{
			if(lookup.type != GPOS_LOOKUP_SINGLE)
				continue;

			for(auto ofs : lookup.subtable_offsets)
			{
				auto subtable = lookup.data.drop(ofs);
				auto subtable_start = subtable;

				auto format = consume_u16(subtable);
				auto cov_ofs = consume_u16(subtable);
				auto value_fmt = consume_u16(subtable);

				if(format != 1 && format != 2)
					sap::internal_error("unknown format {}", format);

				if(auto cov_idx = off::getGlyphCoverageIndex(subtable_start.drop(cov_ofs), glyphId); cov_idx.has_value())
				{
					if(format == 1)
					{
						return parse_value_record(subtable, value_fmt);
					}
					else
					{
						auto num_records = consume_u16(subtable);
						assert(cov_idx < num_records);

						subtable.remove_prefix(get_value_record_size(value_fmt) * *cov_idx);
						return parse_value_record(subtable, value_fmt);
					}
				}
			}
		}

		return { };
	}


	std::map<std::pair<uint32_t, uint32_t>, KerningPair> FontFile::getAllKerningPairs() const
	{
		std::map<std::pair<uint32_t, uint32_t>, KerningPair> kerning_pairs;

		for(auto& lookup : this->gpos_tables.lookup_tables)
		{
			if(lookup.type != GPOS_LOOKUP_PAIR)
				continue;

			for(auto ofs : lookup.subtable_offsets)
			{
				auto subtable = lookup.data.drop(ofs);
				auto subtable_start = subtable;

				auto format = consume_u16(subtable);
				auto cov_ofs = consume_u16(subtable);
				auto value_fmt1 = consume_u16(subtable);
				auto value_fmt2 = consume_u16(subtable);

				if(format != 1 && format != 2)
					sap::internal_error("unknown format {}", format);

				auto cov_table = off::parseCoverageTable(subtable_start.drop(cov_ofs));

				if(format == 1)
				{
					// there should be one PairSet for every glyph in the coverage table
					auto num_pair_sets = consume_u16(subtable);
					assert(cov_table.size() == num_pair_sets);

					for(size_t i = 0; i < num_pair_sets; i++)
					{
						auto pairset_table = subtable_start.drop(consume_u16(subtable));
						auto num_pairs = consume_u16(pairset_table);

						auto glyph_1 = cov_table[i];

						for(size_t k = 0; k < num_pairs; k++)
						{
							auto glyph_2 = consume_u16(pairset_table);
							auto a1 = parse_value_record(pairset_table, value_fmt1);
							auto a2 = parse_value_record(pairset_table, value_fmt2);

							kerning_pairs[{ glyph_1, glyph_2 }] = { a1, a2 };
						}
					}
				}
				else if(format == 2)
				{
					auto cls_ofs1 = consume_u16(subtable);
					auto cls_ofs2 = consume_u16(subtable);

					auto num_cls1 = consume_u16(subtable);
					auto num_cls2 = consume_u16(subtable);

					auto classes_1 = off::parseClassDefTable(subtable_start.drop(cls_ofs1));
					auto classes_2 = off::parseClassDefTable(subtable_start.drop(cls_ofs2));

					for(size_t c1 = 0; c1 < num_cls1; c1++)
					{
						for(size_t c2 = 0; c2 < num_cls2; c2++)
						{
							auto a1 = parse_value_record(subtable, value_fmt1);
							auto a2 = parse_value_record(subtable, value_fmt2);

							// for all the glyphs in each class, apply the adjustment.
							for(auto g1 : classes_1[c1])
								for(auto g2 : classes_2[c2])
									kerning_pairs[{ g1, g2 }] = { a1, a2 };
						}
					}
				}
				else
				{
					assert(false);
				}
			}
		}

		return kerning_pairs;
	}
}


namespace font::off
{
	std::map<size_t, GlyphAdjustment> getPositioningAdjustmentsForGlyphSequence(FontFile* font,
		zst::span<uint32_t> glyphs, const FeatureSet& features)
	{
		return {};
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

#if 0
		zpr::println("scripts:");
		for(auto& scr : script_list)
			zpr::println("  {}", scr.tag.str());

		zpr::println("features: ({})", feature_list.size());
		for(auto& feat : feature_list)
		{
			zpr::println("  feat: {}, ofs = {}", feat.tag.str(), feat.file_offset);
			auto data = zst::byte_span(font->file_bytes, font->file_size).drop(feat.file_offset);

			consume_u16(data);
			auto num_lookups = consume_u16(data);
			for(size_t i = 0; i < num_lookups; i++)
				zpr::println("    [{}]", consume_u16(data));
		}
#endif

		auto lookup_list_ofs = consume_u16(buf);
		auto lookup_tables = table_start.drop(lookup_list_ofs);

		font->gpos_tables.lookup_tables = parseLookupList(font, lookup_tables);
	}
}
