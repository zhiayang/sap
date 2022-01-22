// contextual_lookup.cpp
// Copyright (c) 2022, zhiayang
// SPDX-License-Identifier: Apache-2.0

#include "error.h"

#include "font/font.h"
#include "font/features.h"

namespace font::off
{
	static std::vector<std::pair<uint16_t, uint16_t>> parse_records(size_t num_records, zst::byte_span table)
	{
		std::vector<std::pair<uint16_t, uint16_t>> ret {};
		for(size_t k = 0; k < num_records; k++)
			ret.emplace_back(consume_u16(table), consume_u16(table));

		return ret;
	}

	std::vector<std::pair<uint16_t, uint16_t>> performContextualLookup(zst::byte_span subtable, zst::span<uint32_t> glyphs)
	{
		auto subtable_start = subtable;
		auto format = consume_u16(subtable);

		if(format != 1 && format != 2 && format != 3)
		{
			sap::warn("font/off", "unknown subtable format '{}' in GPOS/GSUB Contextual", format);
			return {};
		}

		if(format == 3)
		{
			auto num_glyphs = consume_u16(subtable);
			auto num_records = consume_u16(subtable);

			// can't match this rule, not enough glyphs
			if(glyphs.size() < num_glyphs)
				return {};

			for(size_t i = 0; i < num_glyphs; i++)
			{
				// basically, the idea is that for glyph[i], it must appear in coverage[i].
				auto coverage = subtable_start.drop(consume_u16(subtable));
				if(!getGlyphCoverageIndex(coverage, glyphs[i]).has_value())
					return {};
			}

			return parse_records(num_records, subtable);
		}
		else
		{
			assert(format == 1 || format == 2);
			auto cov_ofs = consume_u16(subtable);

			// both formats 1 and 2 uses a coverage table to gate the first glyph. If the first
			// glyph in our sequence is not covered, skip this subtable.
			auto cov_idx = off::getGlyphCoverageIndex(subtable_start.drop(cov_ofs), glyphs[0]);
			if(!cov_idx.has_value())
				return {};

			auto try_match_rule = [&glyphs](zst::byte_span& rule, auto&& trf) -> std::pair<bool, uint16_t> {
				auto num_glyphs = consume_u16(rule);
				auto num_records = consume_u16(rule);

				// can't match this rule, not enough glyphs
				if(glyphs.size() < num_glyphs)
					return { false, 0 };

				for(auto k = 1; k < num_glyphs; k++)
				{
					if(trf(glyphs[k]) != consume_u16(rule))
						return { false, 0 };
				}

				return { true, num_records };
			};

			if(format == 1)
			{
				/*
					Format 1 defines lookups based on glyph ids. After slogging through the tables,
					the fundamental idea is to match the input glyphstring with an expected sequence,
					which is given in glyph ids.
				*/

				auto num_rulesets = consume_u16(subtable);
				assert(*cov_idx < num_rulesets);

				subtable.remove_prefix(*cov_idx * sizeof(uint16_t));
				auto ruleset_ofs = consume_u16(subtable);

				auto ruleset_table = subtable_start.drop(ruleset_ofs);
				auto ruleset_table_start = ruleset_table;

				auto num_rules = consume_u16(ruleset_table);
				for(size_t i = 0; i < num_rules; i++)
				{
					auto rule = ruleset_table_start.drop(consume_u16(ruleset_table));
					auto [ matched, num_records ] = try_match_rule(rule, [](auto x) { return x; });

					if(matched)
						return parse_records(num_records, rule);
				}
			}
			else
			{
				/*
					Format 2 defines lookups based on classes. The first glyph is still gated
					by the initial coverage table. At the final stage (Rule), glyphs in the
					input glyphstring are matched by *class id*, not glyph id.
				*/
				auto classdef_ofs = consume_u16(subtable);

				/*
					Here, we don't parse the entire table at once, but rather lookup the classdef table
					separately. We can perform a binary search on the table, so it's not too slow.

					Building a datastructure (ie. parsing the whole table) is probably not a good idea,
					since it won't (and can't, for now...) be cached between lookups.
				*/
				auto classdef_table = subtable_start.drop(classdef_ofs);
				auto num_class_sets = consume_u16(subtable);

				auto first_class_id = getGlyphClass(classdef_table, glyphs[0]);
				assert(first_class_id < num_class_sets);

				auto classset = subtable_start.drop(peek_u16(subtable.drop(first_class_id * sizeof(uint16_t))));
				auto classset_start = classset;

				auto num_rules = consume_u16(classset);
				for(size_t i = 0; i < num_rules; i++)
				{
					auto rule = classset_start.drop(consume_u16(classset));
					auto [ matched, num_records ] = try_match_rule(rule, [&classdef_table](auto gid) {
						return getGlyphClass(classdef_table, gid);
					});

					if(matched)
						return parse_records(num_records, rule);
				}
			}
		}

		return {};
	}


	std::vector<std::pair<uint16_t, uint16_t>> performChainedContextLookup(zst::byte_span subtable,
		zst::span<uint32_t> glyphs, size_t position)
	{
		auto subtable_start = subtable;
		auto format = consume_u16(subtable);

		if(format != 1 && format != 2 && format != 3)
		{
			sap::warn("font/off", "unknown subtable format '{}' in GPOS/GSUB ChainingContext", format);
			return {};
		}

		if(format == 3)
		{
			auto num_lookbehind = consume_u16(subtable);
			auto num_glyphs     = peek_u16(subtable.drop(num_lookbehind * sizeof(uint16_t)));

			// note: here, the input sequence includes glyph[position] itself, so there is no -1 to cancel
			auto num_lookahead  = peek_u16(subtable.drop((num_lookbehind + num_glyphs + 1) * sizeof(uint16_t)));

			if(position < num_lookbehind)
				return {};
			else if(position + num_glyphs > glyphs.size())
				return {};
			else if(position + num_glyphs + num_lookahead >= glyphs.size())
				return {};

			for(size_t k = 0; k < num_lookbehind; k++)
			{
				auto coverage = subtable_start.drop(consume_u16(subtable));
				if(!getGlyphCoverageIndex(coverage, glyphs[position - k - 1]).has_value())
					return {};
			}

			consume_u16(subtable);  // num_glyphs, which we already read
			for(size_t k = 1; k < num_glyphs; k++)
			{
				auto coverage = subtable_start.drop(consume_u16(subtable));
				if(!getGlyphCoverageIndex(coverage, glyphs[position + k]).has_value())
					return {};
			}

			consume_u16(subtable);  // num_lookahead, which we already read
			for(size_t k = 0; k < num_lookahead; k++)
			{
				auto coverage = subtable_start.drop(consume_u16(subtable));
				if(!getGlyphCoverageIndex(coverage, glyphs[position + num_glyphs + k]).has_value())
					return {};
			}

			// match success
			auto num_records = consume_u16(subtable);
			return parse_records(num_records, subtable);
		}
		else
		{
			assert(format == 1 || format == 2);
			auto cov_ofs = consume_u16(subtable);

			// both formats 1 and 2 uses a coverage table to gate the first glyph. If the first
			// glyph in our sequence is not covered, skip this subtable.
			auto cov_idx = off::getGlyphCoverageIndex(subtable_start.drop(cov_ofs), glyphs[position]);
			if(!cov_idx.has_value())
				return {};

			auto try_match_rule = [&glyphs, position](zst::byte_span& rule, auto&& lookbehind_trf,
				auto&& gid_trf, auto&& lookahead_trf) -> bool
			{
				auto num_lookbehind = consume_u16(rule);
				auto num_glyphs = peek_u16(rule.drop(num_lookbehind * sizeof(uint16_t)));

				// note: I know the input sequence only has num_glyphs - 1 uint16s, but we also
				// account for the `num_glyphs` u16 itself, so the -1 and +1 cancel out.
				auto num_lookahead = peek_u16(rule.drop((num_lookbehind + num_glyphs) * sizeof(uint16_t)));

				// if we don't have enough to backtrack, bail
				if(position < num_lookbehind)
					return false;
				else if(position + num_glyphs > glyphs.size())
					return false;
				else if(position + num_glyphs + num_lookahead >= glyphs.size())
					return false;

				for(size_t k = 0; k < num_lookbehind; k++)
				{
					if(lookbehind_trf(glyphs[position - k - 1]) != consume_u16(rule))
						return false;
				}

				consume_u16(rule);  // num_glyphs, which we already read
				for(size_t k = 1; k < num_glyphs; k++)
				{
					if(gid_trf(glyphs[position + k]) != consume_u16(rule))
						return false;
				}

				consume_u16(rule);  // num_lookahead, which we already read
				for(size_t k = 0; k < num_lookahead; k++)
				{
					if(lookahead_trf(glyphs[position + num_glyphs + k]) != consume_u16(rule))
						return false;
				}

				return true;
			};


			// note that this is very similar (structurally) to normal contextual lookups;
			// see if we can deduplicate this code in the future.
			if(format == 1)
			{
				auto num_rulesets = consume_u16(subtable);
				assert(*cov_idx < num_rulesets);

				subtable.remove_prefix(*cov_idx * sizeof(uint16_t));
				auto ruleset_ofs = consume_u16(subtable);

				auto ruleset_table = subtable_start.drop(ruleset_ofs);
				auto ruleset_table_start = ruleset_table;

				auto identity_trf = [](uint32_t x) {
					return x;
				};

				auto num_rules = consume_u16(ruleset_table);
				for(size_t i = 0; i < num_rules; i++)
				{
					auto rule = ruleset_table_start.drop(consume_u16(ruleset_table));
					if(try_match_rule(rule, identity_trf, identity_trf, identity_trf))
					{
						auto num_records = consume_u16(rule);
						return parse_records(num_records, rule);
					}
				}
			}
			else
			{
				auto lookbehind_classdefs = subtable_start.drop(consume_u16(subtable));
				auto input_classdefs      = subtable_start.drop(consume_u16(subtable));
				auto lookahead_classdefs  = subtable_start.drop(consume_u16(subtable));

				auto num_class_sets = consume_u16(subtable);

				auto first_class_id = getGlyphClass(input_classdefs, glyphs[position]);
				assert(first_class_id < num_class_sets);

				auto classset = subtable_start.drop(peek_u16(subtable.drop(first_class_id * sizeof(uint16_t))));
				auto classset_start = classset;

				auto num_rules = consume_u16(classset);
				for(size_t i = 0; i < num_rules; i++)
				{
					auto rule = classset_start.drop(consume_u16(classset));
					auto lookbehind_trf = [&lookbehind_classdefs](uint32_t gid) {
						return getGlyphClass(lookbehind_classdefs, gid);
					};

					auto input_trf = [&input_classdefs](uint32_t gid) {
						return getGlyphClass(input_classdefs, gid);
					};

					auto lookahead_trf = [&lookahead_classdefs](uint32_t gid) {
						return getGlyphClass(lookahead_classdefs, gid);
					};

					if(try_match_rule(rule, lookbehind_trf, input_trf, lookahead_trf))
					{
						auto num_records = consume_u16(rule);
						return parse_records(num_records, rule);
					}
				}
			}
		}

		return {};
	}
}
