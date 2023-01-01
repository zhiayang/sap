// kern.cpp
// Copyright (c) 2022, zhiayang
// SPDX-License-Identifier: Apache-2.0

#include "font/aat.h"
#include "font/misc.h"
#include "font/font_file.h"

namespace font::aat
{
	static void combine_adjustments(GlyphAdjustment& a, const GlyphAdjustment& b)
	{
		a.horz_placement += b.horz_placement;
		a.vert_placement += b.vert_placement;
		a.horz_advance += b.horz_advance;
		a.vert_advance += b.vert_advance;
	}

	static KernSubTable0 parse_kern_f0(KernSubTableCoverage coverage, zst::byte_span buf)
	{
		KernSubTable0 ret {};
		ret.coverage = std::move(coverage);

		auto& pairs = ret.pairs;

		auto num_pairs = consume_u16(buf);

		buf.remove_prefix(6); // skip the binary search stuff

		for(auto i = 0u; i < num_pairs; i++)
		{
			auto left = consume_u16(buf);
			auto right = consume_u16(buf);
			auto adjust = consume_i16(buf);

			pairs.push_back(KernSubTable0::Pair {
			    .left = GlyphId(left),
			    .right = GlyphId(right),
			    .shift = FontScalar(adjust),
			});
		}

		return ret;
	}

	static KernSubTable2 parse_kern_f2(KernSubTableCoverage coverage, zst::byte_span buf)
	{
		KernSubTable2 ret {};
		ret.coverage = std::move(coverage);

		auto save = buf;

		auto bytes_per_row = consume_u16(buf);
		(void) bytes_per_row;

		auto left_class_table_offset = consume_u16(buf);
		auto right_class_table_offset = consume_u16(buf);
		auto array_offset = consume_u16(buf);

		ret.lookup_array = save.drop(array_offset);
		auto left_class_table = save.drop(left_class_table_offset);
		auto right_class_table = save.drop(right_class_table_offset);

		auto parse_class_table = [](zst::byte_span table) {
			auto first_glyph = consume_u16(table);
			auto num_glyphs = consume_u16(table);

			std::unordered_map<GlyphId, uint16_t> ret {};
			for(auto i = 0u; i < num_glyphs; i++)
				ret[GlyphId(first_glyph + i)] = consume_u16(table);

			return ret;
		};

		ret.left_glyph_classes = parse_class_table(left_class_table);
		ret.right_glyph_classes = parse_class_table(right_class_table);

		return ret;
	}




	static KernTable parse_kern_version_0(zst::byte_span buf)
	{
		KernTable kern_table {};

		auto num_tables = consume_u16(buf);

		for(auto i = 0u; i < num_tables; i++)
		{
			auto ver = consume_u16(buf);
			auto total_len = consume_u16(buf);

			auto cov = consume_u16(buf);
			auto format = (cov & 0xff00) >> 8;

			auto subtable_len = total_len - 3 * sizeof(uint16_t);

			if(ver != 0)
			{
				sap::warn("otf/kern", "unsupported kern subtable version {}", ver);
				buf.remove_prefix(subtable_len);
				continue;
			}
			else if(format != 0 && format != 2)
			{
				sap::warn("otf/kern", "unsupported kern subtable format {}", format);
				buf.remove_prefix(subtable_len);
				continue;
			}

			KernSubTableCoverage coverage {
				.is_vertical = not(cov & (1 << 0)),
				.is_cross_stream = (bool) (cov & (1 << 2)),
				.is_variation = false,
				.is_override = (bool) (cov & (1 << 3)),
				.is_minimum = (bool) (cov & (1 << 1)),
			};

			if(format == 0)
				kern_table.subtables_f0.push_back(parse_kern_f0(std::move(coverage), buf.take(subtable_len)));
			else
				kern_table.subtables_f2.push_back(parse_kern_f2(std::move(coverage), buf.take(subtable_len)));

			buf.remove_prefix(subtable_len);
		}

		return kern_table;
	}


	static KernTable parse_kern_version_1(zst::byte_span buf)
	{
		KernTable kern_table {};

		// drop the other version part
		buf.remove_prefix(2);

		auto num_tables = consume_u32(buf);
		for(auto i = 0u; i < num_tables; i++)
		{
			auto total_len = consume_u32(buf);

			auto cov = consume_u16(buf);
			auto format = (cov & 0xff00) >> 8;

			auto tuple_idx = consume_u16(buf);
			(void) tuple_idx;

			auto subtable_len = total_len - 3 * sizeof(uint16_t);

			if(format > 4)
			{
				sap::warn("otf/kern", "unsupported kern subtable format {}", format);
				buf.remove_prefix(subtable_len);
				continue;
			}

			KernSubTableCoverage coverage {
				.is_vertical = bool(cov & 0x8000),
				.is_cross_stream = bool(cov & 0x4000),
				.is_variation = bool(cov & 0x2000),
				.is_override = false,
				.is_minimum = false,
			};

			if(format == 0)
				kern_table.subtables_f0.push_back(parse_kern_f0(std::move(coverage), buf.take(subtable_len)));
			else
				kern_table.subtables_f2.push_back(parse_kern_f2(std::move(coverage), buf.take(subtable_len)));

			buf.remove_prefix(subtable_len);
		}

		return kern_table;
	}

	// todo: only adjust the right glyph
	static std::optional<GlyphAdjustment> lookup_subtables(const KernTable& table, GlyphId left, GlyphId right)
	{
		bool found = false;
		GlyphAdjustment ret {};

		auto add_adjustment = [&ret, &found](FontScalar adj, const KernSubTableCoverage& coverage) {
			found = true;

			if(coverage.is_override)
			{
				if(coverage.is_cross_stream)
					ret.vert_advance = 0;
				else
					ret.horz_advance = 0;
			}

			if(coverage.is_cross_stream)
				combine_adjustments(ret, { .vert_advance = adj });
			else
				combine_adjustments(ret, { .horz_advance = adj });

			if(coverage.is_minimum)
			{
				if(coverage.is_cross_stream)
					ret.vert_advance = std::max(ret.vert_advance, adj);
				else
					ret.horz_advance = std::max(ret.horz_advance, adj);
			}
		};

		if(static_cast<uint32_t>(left) < 0xFFFF && static_cast<uint32_t>(right) < 0xFFFF)
		{
			auto search_u32 = ((static_cast<uint32_t>(left) & 0xffff) << 16) | (static_cast<uint32_t>(right) & 0xffff);

			for(auto& sub : table.subtables_f0)
			{
				if(sub.coverage.is_vertical)
					continue;

				// now, binary search the second set.
				size_t low = 0;
				size_t high = sub.pairs.size();

				while(low < high)
				{
					auto mid = (low + high) / 2u;

					auto tmp = ((static_cast<uint32_t>(sub.pairs[mid].left) & 0xffff) << 16)
					         | (static_cast<uint32_t>(sub.pairs[mid].right) & 0xffff);

					if(tmp == search_u32)
					{
						add_adjustment(sub.pairs[mid].shift, sub.coverage);
						break;
					}
					else if(tmp < search_u32)
					{
						low = mid + 1;
					}
					else
					{
						high = mid;
					}
				}
			}
		}

		if(static_cast<uint32_t>(left) < 0xFFFF && static_cast<uint32_t>(right) < 0xFFFF)
		{
			for(auto& sub : table.subtables_f2)
			{
				uint16_t left_class = 0;
				uint16_t right_class = 0;

				if(auto it = sub.left_glyph_classes.find(left); it != sub.left_glyph_classes.end())
					left_class = it->second;
				else
					continue;

				if(auto it = sub.right_glyph_classes.find(right); it != sub.right_glyph_classes.end())
					right_class = it->second;
				else
					continue;

				auto shift = FontScalar(peek_i16(sub.lookup_array.drop(left_class + right_class)));
				add_adjustment(shift, sub.coverage);
			}
		}

		if(found)
			return ret;
		else
			return std::nullopt;
	}

	std::map<size_t, GlyphAdjustment> getPositioningAdjustmentsForGlyphSequence(const KernTable& table, zst::span<GlyphId> glyphs)
	{
		if(glyphs.size() < 2)
			return {};

		std::map<size_t, GlyphAdjustment> adjustments {};

		// kern table only has pairs, so this is easy.
		for(size_t i = 0; i < glyphs.size() - 1; i++)
		{
			auto maybe_adj = lookup_subtables(table, glyphs[i], glyphs[i + 1]);
			if(maybe_adj.has_value())
				adjustments[i] = std::move(*maybe_adj);
		}

		return adjustments;
	}
}

namespace font
{
	void FontFile::parse_kern_table(const Table& table)
	{
		auto buf = this->bytes().drop(table.offset);
		auto version = consume_u16(buf);

		if(version == 0)
			m_kern_table = aat::parse_kern_version_0(buf);
		else if(version == 1)
			m_kern_table = aat::parse_kern_version_1(buf);
		else
			sap::error("otf", "invalid 'kern' version '{}'", version);
	}
}
