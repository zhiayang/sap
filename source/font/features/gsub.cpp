// lookup.cpp
// Copyright (c) 2022, zhiayang
// SPDX-License-Identifier: Apache-2.0

#include "util.h"
#include "error.h"

#include "font/font.h"
#include "font/features.h"

namespace font::off::gsub
{
	std::optional<uint32_t> lookupSingleSubstitution(const LookupTable& lookup, uint32_t gid)
	{
		assert(lookup.type == LOOKUP_SINGLE);

		for(auto subtable : lookup.subtables)
		{
			auto subtable_start = subtable;
			auto format = consume_u16(subtable);

			if(format != 1 && format != 2)
			{
				sap::warn("font/gsub", "unknown subtable format '{}' in GSUB/Single", format);
				continue;
			}

			// both of them have coverage tables.
			auto cov_ofs = consume_u16(subtable);
			auto cov_idx = getGlyphCoverageIndex(subtable_start.drop(cov_ofs), gid);
			if(!cov_idx.has_value())
				continue;

			if(format == 1)
			{
				// fixed delta for all covered glyphs.
				auto delta = consume_u16(subtable);
				return gid + delta;
			}
			else
			{
				auto num_glyphs = consume_u16(subtable);
				assert(*cov_idx < num_glyphs);

				return peek_u16(subtable.drop(*cov_idx * sizeof(uint16_t)));
			}
		}

		return std::nullopt;
	}

	std::optional<std::vector<uint32_t>> lookupMultipleSubstitution(const LookupTable& lookup, uint32_t gid)
	{
		assert(lookup.type == LOOKUP_MULTIPLE);
		for(auto subtable : lookup.subtables)
		{
			auto subtable_start = subtable;
			auto format = consume_u16(subtable);

			if(format != 1)
			{
				sap::warn("font/gsub", "unknown subtable format '{}' in GSUB/Multiple", format);
				continue;
			}

			auto cov_ofs = consume_u16(subtable);
			auto cov_idx = getGlyphCoverageIndex(subtable_start.drop(cov_ofs), gid);
			if(!cov_idx.has_value())
				continue;

			auto num_sequences = consume_u16(subtable);
			assert(*cov_idx < num_sequences);

			auto seq_ofs = peek_u16(subtable.drop(*cov_idx * sizeof(uint16_t)));
			auto sequence = subtable_start.drop(seq_ofs);

			/*
				spec:

				The use of multiple substitution for deletion of an input glyph is prohibited.
				The glyphCount value should always be greater than 0.
			*/
			auto num_glyphs = consume_u16(sequence);
			assert(num_glyphs > 0);

			std::vector<uint32_t> subst {};
			for(uint16_t i = 0; i < num_glyphs; i++)
				subst.push_back(consume_u16(sequence));

			return subst;
		}

		return std::nullopt;
	}

	std::optional<std::pair<uint32_t, size_t>> lookupLigatureSubstitution(const LookupTable& lookup,
		zst::span<uint32_t> glyphs)
	{
		assert(glyphs.size() > 0);
		assert(lookup.type == LOOKUP_LIGATURE);

		for(auto subtable : lookup.subtables)
		{
			auto subtable_start = subtable;
			auto format = consume_u16(subtable);

			if(format != 1)
			{
				sap::warn("font/gsub", "unknown subtable format '{}' in GSUB/Ligature", format);
				continue;
			}

			auto cov_ofs = consume_u16(subtable);
			auto cov_idx = getGlyphCoverageIndex(subtable_start.drop(cov_ofs), glyphs[0]);
			if(!cov_idx.has_value())
				continue;

			auto num_sets = consume_u16(subtable);
			assert(*cov_idx < num_sets);

			auto set_ofs = peek_u16(subtable.drop(*cov_idx * sizeof(uint16_t)));

			auto ligature_set = subtable_start.drop(set_ofs);
			auto ligature_set_start = ligature_set;

			auto num_ligatures = consume_u16(ligature_set);
			for(uint16_t i = 0; i < num_ligatures; i++)
			{
				auto ligature = ligature_set_start.drop(consume_u16(ligature_set));

				uint32_t output_gid = consume_u16(ligature);
				size_t num_components = consume_u16(ligature);

				if(glyphs.size() < num_components)
					continue;

				bool matched = true;
				for(size_t k = 1; matched && k < num_components; k++)
				{
					if(glyphs[k] != consume_u16(ligature))
						matched = false;
				}

				if(matched)
					return std::pair(output_gid, num_components);
			}
		}

		return std::nullopt;
	}


	std::optional<SubstitutionResult> lookupForGlyphSequence(const GSubTable& gsub_table, const LookupTable& lookup,
		zst::span<uint32_t> glyphs, size_t position)
	{


		return std::nullopt;
	}



	using SubstLookupRecord = ContextualLookupRecord;
	static SubstitutionResult apply_lookup_records(const GSubTable& gsub_table,
		const std::pair<std::vector<SubstLookupRecord>, size_t>& records,
		zst::span<uint32_t> glyphs, size_t position)
	{
		/*
			so the idea is, instead of copying around the entire glyphstring like a fool,  when we
			perform a lookup that substitutes stuff, we replace the input sequence part of this array,
			without touching the lookbehind part. Then, we copy the lookahead part at the end. this saves...
			a few copies, probably.
		*/
		auto num_input_glyphs = records.second;
		auto glyphstring = std::vector<uint32_t>(glyphs.begin(), glyphs.end());
		auto lookahead = glyphs.drop(position + num_input_glyphs);

		for(auto [ glyph_idx, lookup_idx ] : records.first)
		{
			assert(lookup_idx < gsub_table.lookups.size());
			auto& nested_lookup = gsub_table.lookups[lookup_idx];

			auto span = zst::span<uint32_t>(glyphstring.data(), glyphstring.size());
			auto result = lookupForGlyphSequence(gsub_table, nested_lookup, span, /* pos: */ glyph_idx + position);
			if(result.has_value())
			{
				// clear the input sequence + lookahead part
				glyphstring.erase(glyphstring.begin() + position, glyphstring.end());

				// copy over the new input sequence
				glyphstring.insert(glyphstring.end(),
					std::move_iterator(result->glyphs.begin()),
					std::move_iterator(result->glyphs.end())
				);

				// and finally, the lookahead
				glyphstring.insert(glyphstring.end(), lookahead.begin(), lookahead.end());
			}
		}

		SubstitutionResult result {};
		result.input_consumed = num_input_glyphs;

		// finally, the glyphs we return should only include the input sequence.
		assert(glyphstring.size() > position + lookahead.size());
		result.glyphs = std::vector<uint32_t>(glyphstring.begin() + position, glyphstring.end() - lookahead.size());

		return result;
	}

	std::optional<SubstitutionResult> lookupContextualSubstitution(const GSubTable& gsub, const LookupTable& lookup,
		zst::span<uint32_t> glyphs)
	{
		assert(lookup.type == LOOKUP_CONTEXTUAL);
		for(auto subtable : lookup.subtables)
		{
			if(auto records = performContextualLookup(subtable, glyphs); records.has_value())
				return apply_lookup_records(gsub, *records, glyphs, /* pos: */ 0);
		}

		return std::nullopt;
	}

	std::optional<SubstitutionResult> lookupChainedContextSubstitution(const GSubTable& gsub, const LookupTable& lookup,
		zst::span<uint32_t> glyphs, size_t position)
	{
		assert(position < glyphs.size());
		assert(lookup.type == LOOKUP_CHAINING_CONTEXT);

		for(auto subtable : lookup.subtables)
		{
			if(auto records = performChainedContextLookup(subtable, glyphs, position); records.has_value())
				return apply_lookup_records(gsub, *records, glyphs, /* pos: */ position);
		}

		return std::nullopt;
	}
}
