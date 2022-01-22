// gpos.cpp
// Copyright (c) 2021, zhiayang
// SPDX-License-Identifier: Apache-2.0

#include <cassert>

#include "util.h"
#include "error.h"

#include "font/font.h"
#include "font/features.h"


namespace font::off
{
	// used in `lookup.cpp`
	static void combine_adjustments(GlyphAdjustment& a, const GlyphAdjustment& b)
	{
		a.horz_placement += b.horz_placement;
		a.vert_placement += b.vert_placement;
		a.horz_advance += b.horz_advance;
		a.vert_advance += b.vert_advance;
	}

	std::map<size_t, GlyphAdjustment> getPositioningAdjustmentsForGlyphSequence(FontFile* font,
		zst::span<uint32_t> glyphs, const FeatureSet& features)
	{
		auto gpos = font->gpos_table;

		/*
			OFF 1.9, page 217

			During text processing, a client applies a lookup to each glyph in the string before moving
			to the next lookup. A lookup is finished for a glyph after the client locates the target glyph
			or glyph context and performs a positioning action, if specified.
			-----------------------------

			TL;DR: for(lookups) { for(glyphs) { ... } }, and *NOT* the transposed.
		*/

		std::map<size_t, GlyphAdjustment> adjustments {};
		auto lookups = getLookupTablesForFeatures(font->gpos_table, features);

		for(auto& lookup_idx : lookups)
		{
			assert(lookup_idx < gpos.lookups.size());
			auto& lookup = gpos.lookups[lookup_idx];

			// in this case, we want to lookup the entire sequence, so start at position 0.
			auto new_adjs = gpos::lookupForGlyphSequence(gpos, lookup, glyphs, /* position: */ 0);
			for(auto& [ idx, adj ] : new_adjs)
				combine_adjustments(adjustments[idx], adj);
		}

		return adjustments;
	}

	std::map<size_t, GlyphAdjustment> gpos::lookupForGlyphSequence(const GPosTable& gpos_table, const LookupTable& lookup,
		zst::span<uint32_t> glyphs, size_t position)
	{
		/*
			OFF 1.9, page 217 (part 2)

			To move to the "next" glyph, the client will typically skip all the glyphs that participated
			in the lookup operation: glyphs that were positioned as well as any other glyphs that formed a
			context for the operation.

			There is just one exception: the "next" glyph in a sequence may be one of those that formed a
			context for the operation just performed. For example, in the case of pair positioning operations
			(i.e., kerning), if the ValueRecord for the second glyph is null, that glyph is treated as the
			"next" glyph in the sequence.
			-----------------------------

			TL;DR: i have no fucking clue. In which cases are there exceptions? In which cases not? idk.
			For now, we skip all glyphs that got adjusted.
		*/

		std::map<size_t, GlyphAdjustment> adjs {};
		for(size_t i = position; i < glyphs.size(); i++)
		{
			// this should have been eliminated during initial parsing already
			assert(lookup.type != gpos::LOOKUP_EXTENSION_POS);

			if(lookup.type == gpos::LOOKUP_SINGLE)
			{
				if(auto adj = gpos::lookupSingleAdjustment(lookup, glyphs[i]); adj.has_value())
					return { { i - position, *adj } };
			}
			else if((lookup.type == gpos::LOOKUP_PAIR) && (i + 1 < glyphs.size()))
			{
				auto [ a1, a2 ] = gpos::lookupPairAdjustment(lookup, glyphs[i], glyphs[i + 1]);
				if(a1.has_value())
					combine_adjustments(adjs[i - position], *a1);

				if(a2.has_value())
				{
					// if the second adjustment was not null, then we skip the second glyph
					combine_adjustments(adjs[i - position + 1], *a2);
					i += 1;
				}
			}
			else if(lookup.type == gpos::LOOKUP_CONTEXTUAL || lookup.type == gpos::LOOKUP_CHAINING_CONTEXT)
			{
				// this one requires both lookahead and lookbehind
				decltype (adjs) new_adjs {};

				if(lookup.type == gpos::LOOKUP_CONTEXTUAL)
					new_adjs = lookupContextualPositioning(gpos_table, lookup, glyphs.drop(i));
				else
					new_adjs = lookupChainedContextPositioning(gpos_table, lookup, glyphs, /* pos: */ i);

				// same deal with offsets and jumping as normal contextual lookups
				for(auto& [ idx, adj ] : new_adjs)
					combine_adjustments(adjs[i + idx - position], adj);

				if(new_adjs.size() > 0)
					i += new_adjs.rbegin()->first;
			}
		}

		return adjs;
	}
}




namespace font::off::gpos
{
	static OptionalGA parse_value_record(zst::byte_span& buf, uint16_t format)
	{
		if(format == 0)
			return std::nullopt;

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


	OptionalGA lookupSingleAdjustment(const LookupTable& lookup, uint32_t gid)
	{
		assert(lookup.type == LOOKUP_SINGLE);

		for(auto subtable : lookup.subtables)
		{
			auto subtable_start = subtable;

			auto format = consume_u16(subtable);
			auto cov_ofs = consume_u16(subtable);
			auto value_fmt = consume_u16(subtable);

			if(format != 1 && format != 2)
			{
				sap::warn("font/gpos", "unknown subtable format '{}' in GPOS/Single", format);
				continue;
			}

			if(auto cov_idx = off::getGlyphCoverageIndex(subtable_start.drop(cov_ofs), gid); cov_idx.has_value())
			{
				if(format == 1)
				{
					return parse_value_record(subtable, value_fmt);
				}
				else
				{
					auto num_records = consume_u16(subtable);
					assert(cov_idx < num_records);

					subtable.remove_prefix(get_value_record_size(value_fmt) * (*cov_idx));
					return parse_value_record(subtable, value_fmt);
				}
			}
		}

		return std::nullopt;
	}

	std::pair<OptionalGA, OptionalGA> lookupPairAdjustment(const LookupTable& lookup, uint32_t gid1, uint32_t gid2)
	{
		assert(lookup.type == LOOKUP_PAIR);
		for(auto subtable : lookup.subtables)
		{
			auto subtable_start = subtable;

			auto format = consume_u16(subtable);
			auto cov_ofs = consume_u16(subtable);
			auto value_fmt1 = consume_u16(subtable);
			auto value_fmt2 = consume_u16(subtable);

			if(format != 1 && format != 2)
			{
				sap::warn("font/gpos", "unknown subtable format '{}' in GPOS/Pair", format);
				continue;
			}

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

		return { std::nullopt, std::nullopt };
	}

	using PosLookupRecord = ContextualLookupRecord;
	static std::map<size_t, GlyphAdjustment> apply_lookup_records(const GPosTable& gpos,
		const std::pair<std::vector<PosLookupRecord>, size_t>& records, zst::span<uint32_t> glyphs, size_t position)
	{
		// ok, we matched this one.
		std::map<size_t, GlyphAdjustment> adjustments {};
		for(auto [ glyph_idx, lookup_idx ] : records.first)
		{
			assert(lookup_idx < gpos.lookups.size());
			auto& nested_lookup = gpos.lookups[lookup_idx];

			auto new_adjs = lookupForGlyphSequence(gpos, nested_lookup, glyphs, /* pos: */ position + glyph_idx);
			for(auto& [ idx, adj ] : new_adjs)
				combine_adjustments(adjustments[idx], adj);
		}

		return adjustments;
	}

	std::map<size_t, GlyphAdjustment> lookupContextualPositioning(const GPosTable& gpos, const LookupTable& lookup,
		zst::span<uint32_t> glyphs)
	{
		assert(lookup.type == LOOKUP_CONTEXTUAL);
		for(auto subtable : lookup.subtables)
		{
			if(auto records = performContextualLookup(subtable, glyphs); records.has_value())
				return apply_lookup_records(gpos, *records, glyphs, /* pos: */ 0);
		}

		return {};
	}

	std::map<size_t, GlyphAdjustment> lookupChainedContextPositioning(const GPosTable& gpos, const LookupTable& lookup,
		zst::span<uint32_t> glyphs, size_t position)
	{
		assert(position < glyphs.size());
		assert(lookup.type == LOOKUP_CHAINING_CONTEXT);

		for(auto subtable : lookup.subtables)
		{
			if(auto records = performChainedContextLookup(subtable, glyphs, position); records.has_value())
				return apply_lookup_records(gpos, *records, glyphs, /* pos: */ position);
		}

		return {};
	}
}
