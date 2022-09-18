// lookup.cpp
// Copyright (c) 2022, zhiayang
// SPDX-License-Identifier: Apache-2.0

#include "util.h"
#include "error.h"

#include "font/font.h"
#include "font/features.h"

namespace font::off::gsub
{
	static SubstitutionMapping& combine_subst_mapping(SubstitutionMapping& to, SubstitutionMapping&& from)
	{
		to.extra_glyphs.insert(std::move_iterator(from.extra_glyphs.begin()), std::move_iterator(from.extra_glyphs.end()));
		to.contractions.insert(std::move_iterator(from.contractions.begin()), std::move_iterator(from.contractions.end()));
		to.replacements.insert(std::move_iterator(from.replacements.begin()), std::move_iterator(from.replacements.end()));
		return to;
	}

	static std::optional<GlyphReplacement> lookupForGlyphSequence(const GSubTable& gsub_table, const LookupTable& lookup,
		zst::span<GlyphId> glyphs, size_t position)
	{
		/*
		    cf. the comment in `lookupForGlyphSequence` in gpos

		    for GSUB i think it's a little more direct, because each "match" gives us the number of glyphs
		    to consume from the input sequence -- so we just skip that amount.
		*/

		size_t sub_begin = 0;
		size_t sub_end = 0;
		std::optional<GlyphReplacement> result {};

		for(size_t i = position; i < glyphs.size(); i++)
		{
			assert(lookup.type != gsub::LOOKUP_EXTENSION_SUBST);

			auto init_result = [&](size_t num, std::vector<GlyphId> subst) {
				if(result.has_value())
				{
					/*
					    we need to perform some copying. say we have glyphs[0..9]. If we substituted
					    glyphs[1..3], then we have sub_begin=1, sub_end=3.

					    if we are now at i=7 and we want to substitute glyphs[7..8], then we must
					    copy glyphs[4..6] to the output array as well.
					*/
					result->glyphs.insert(result->glyphs.end(), glyphs.begin() + sub_end, glyphs.begin() + i);

					sub_end = i + num;
				}
				else
				{
					sub_begin = i;
					sub_end = i + num;

					result = GlyphReplacement {};
				}

				result->glyphs.insert(result->glyphs.end(), subst.begin(), subst.end());
			};


			if(lookup.type == gsub::LOOKUP_SINGLE)
			{
				if(auto subst = lookupSingleSubstitution(lookup, glyphs[i]); subst.has_value())
				{
					init_result(1, { *subst });
					result->mapping.replacements.insert({ *subst, glyphs[i] });
				}
			}
			else if(lookup.type == gsub::LOOKUP_MULTIPLE)
			{
				if(auto subst = lookupMultipleSubstitution(lookup, glyphs[i]); subst.has_value())
				{
					init_result(1, *subst);
					result->mapping.extra_glyphs.insert(std::move_iterator(subst->begin()), std::move_iterator(subst->end()));
				}
			}
			else if(lookup.type == gsub::LOOKUP_LIGATURE)
			{
				if(auto subst = lookupLigatureSubstitution(lookup, glyphs.drop(i)); subst.has_value())
				{
					init_result(subst->second, { subst->first });

					auto foo = glyphs.drop(i).take(subst->second);
					result->mapping.contractions.insert({ subst->first, std::vector(foo.begin(), foo.end()) });

					i += subst->second - 1; // skip over the processed glyphs
				}
			}
			else if(lookup.type == LOOKUP_CONTEXTUAL || lookup.type == LOOKUP_CHAINING_CONTEXT)
			{
				std::optional<GlyphReplacement> subst {};
				if(lookup.type == LOOKUP_CONTEXTUAL)
					subst = lookupContextualSubstitution(gsub_table, lookup, glyphs.drop(i));
				else
					subst = lookupChainedContextSubstitution(gsub_table, lookup, glyphs, /* pos: */ i);

				if(subst.has_value())
				{
					// in this context, we should never have input_start != 0, since the lookups always
					// match starting from glyphs[0] (or glyphs[position]).
					assert(subst->input_start == 0);
					init_result(subst->input_consumed, std::move(subst->glyphs));

					// i can't think of a better way to combine them, so just insert wholesale...
					combine_subst_mapping(result->mapping, std::move(subst->mapping));

					// again, skip over the processed glyphs.
					i += subst->input_consumed - 1;
				}
			}
			else
			{
				sap::warn("font/gsub", "unsupported lookup type '{}'", lookup.type);
			}
		}

		if(result.has_value())
		{
			result->input_start = sub_begin;
			result->input_consumed = sub_end - sub_begin;
		}

		return result;
	}


	std::optional<GlyphId> lookupSingleSubstitution(const LookupTable& lookup, GlyphId gid)
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
				return GlyphId { static_cast<uint32_t>(gid) + delta };
			}
			else
			{
				auto num_glyphs = consume_u16(subtable);
				assert(*cov_idx < num_glyphs);

				return GlyphId { peek_u16(subtable.drop(*cov_idx * sizeof(uint16_t))) };
			}
		}

		return std::nullopt;
	}

	std::optional<std::vector<GlyphId>> lookupMultipleSubstitution(const LookupTable& lookup, GlyphId gid)
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

			std::vector<GlyphId> subst {};
			for(uint16_t i = 0; i < num_glyphs; i++)
				subst.push_back(GlyphId { consume_u16(sequence) });

			return subst;
		}

		return std::nullopt;
	}

	std::optional<std::pair<GlyphId, size_t>> lookupLigatureSubstitution(const LookupTable& lookup, zst::span<GlyphId> glyphs)
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

				auto output_gid = GlyphId { consume_u16(ligature) };
				size_t num_components = consume_u16(ligature);

				if(glyphs.size() < num_components)
					continue;

				bool matched = true;
				for(size_t k = 1; matched && k < num_components; k++)
				{
					if(glyphs[k] != GlyphId { consume_u16(ligature) })
						matched = false;
				}

				if(matched)
					return std::pair(output_gid, num_components);
			}
		}

		return std::nullopt;
	}


	using SubstLookupRecord = ContextualLookupRecord;
	static GlyphReplacement apply_lookup_records(const GSubTable& gsub_table,
		const std::pair<std::vector<SubstLookupRecord>, size_t>& records, zst::span<GlyphId> glyphs, size_t position)
	{
		/*
		    so the idea is, instead of copying around the entire glyphstring like a fool,  when we
		    perform a lookup that substitutes stuff, we replace the input sequence part of this array,
		    without touching the lookbehind part. Then, we copy the lookahead part at the end. this saves...
		    a few copies, probably.
		*/
		auto num_input_glyphs = records.second;
		auto glyphstring = std::vector<GlyphId>(glyphs.begin(), glyphs.end());
		auto lookahead = glyphs.drop(position + num_input_glyphs);

		SubstitutionMapping sub_mapping {};

		for(auto [glyph_idx, lookup_idx] : records.first)
		{
			assert(lookup_idx < gsub_table.lookups.size());
			auto& nested_lookup = gsub_table.lookups[lookup_idx];

			auto span = zst::span<GlyphId>(glyphstring.data(), glyphstring.size());
			auto result = lookupForGlyphSequence(gsub_table, nested_lookup, span, /* pos: */ glyph_idx + position);
			if(result.has_value())
			{
				// erase out the replaced glyphs, leaving the untouched glyphs and the lookahead.
				glyphstring.erase(glyphstring.begin() + position + result->input_start,
					glyphstring.begin() + position + result->input_consumed);

				// copy over the new glyphs to the correct location
				glyphstring.insert(glyphstring.begin() + position + result->input_start,
					std::move_iterator(result->glyphs.begin()), std::move_iterator(result->glyphs.end()));

				combine_subst_mapping(sub_mapping, std::move(result->mapping));
			}
		}

		GlyphReplacement result {};
		result.input_start = 0;
		result.input_consumed = num_input_glyphs;
		result.mapping = std::move(sub_mapping);

		// finally, the glyphs we return should only include the input sequence.
		assert(glyphstring.size() > position + lookahead.size());
		result.glyphs = std::vector<GlyphId>(glyphstring.begin() + position, glyphstring.end() - lookahead.size());
		return result;
	}

	std::optional<GlyphReplacement> lookupContextualSubstitution(const GSubTable& gsub, const LookupTable& lookup,
		zst::span<GlyphId> glyphs)
	{
		assert(lookup.type == LOOKUP_CONTEXTUAL);
		for(auto subtable : lookup.subtables)
		{
			if(auto records = performContextualLookup(subtable, glyphs); records.has_value())
				return apply_lookup_records(gsub, *records, glyphs, /* pos: */ 0);
		}

		return std::nullopt;
	}

	std::optional<GlyphReplacement> lookupChainedContextSubstitution(const GSubTable& gsub, const LookupTable& lookup,
		zst::span<GlyphId> glyphs, size_t position)
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



namespace font::off
{
	SubstitutedGlyphString performSubstitutionsForGlyphSequence(FontFile* font, zst::span<GlyphId> input,
		const FeatureSet& features)
	{
		auto gsub_table = font->gsub_table;
		auto lookups = getLookupTablesForFeatures(font->gsub_table, features);

		SubstitutedGlyphString result {};

		result.glyphs = std::vector<GlyphId>(input.begin(), input.end());
		auto& glyphs = result.glyphs;

		auto span = [](auto& g) {
			return zst::span<GlyphId>(g.data(), g.size());
		};

		for(auto& lookup_idx : lookups)
		{
			assert(lookup_idx < gsub_table.lookups.size());
			auto& lookup = gsub_table.lookups[lookup_idx];

			// in this case, we want to lookup the entire sequence, so start at position 0.
			auto subst = gsub::lookupForGlyphSequence(gsub_table, lookup, span(glyphs), /* position: */ 0);
			if(subst.has_value())
			{
				glyphs.erase(glyphs.begin() + subst->input_start, glyphs.begin() + subst->input_start + subst->input_consumed);
				// copy over the new glyphs to the correct location
				glyphs.insert(glyphs.begin() + subst->input_start, std::move_iterator(subst->glyphs.begin()),
					std::move_iterator(subst->glyphs.end()));

				gsub::combine_subst_mapping(result.mapping, std::move(subst->mapping));
			}
		}

		return result;
	}
}
