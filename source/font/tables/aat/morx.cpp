// morx.cpp
// Copyright (c) 2022, yuki
// SPDX-License-Identifier: Apache-2.0

#include <span>


#include "font/aat.h"
#include "font/misc.h"
#include "font/font_file.h"

namespace font::aat
{
	static MorxFeature parse_morx_feature(zst::byte_span& feature)
	{
		auto type = consume_u16(feature);
		auto selector = consume_u16(feature);
		auto enable_flags = consume_u32(feature);
		auto disable_flags = consume_u32(feature);

		return MorxFeature {
			.feature = Feature { .type = type, .selector = selector },
			.enable_flags = enable_flags,
			.disable_flags = disable_flags,
		};
	}

	static MorxRearrangementSubtable parse_rearrangement_subtable(zst::byte_span buf, size_t num_glyphs)
	{
		return MorxRearrangementSubtable {
			.state_table = parseExtendedStateTable(buf, num_glyphs),
		};
	}

	static MorxContextualSubtable parse_contextual_subtable(zst::byte_span buf, size_t num_glyphs)
	{
		auto start = buf;
		auto state_table = parseExtendedStateTable(buf, num_glyphs);

		auto substitution_table = start.drop(consume_u32(buf));

		return MorxContextualSubtable {
			.state_table = std::move(state_table),
			.substitution_tables = substitution_table,
		};
	}

	static MorxLigatureSubtable parse_ligature_subtable(zst::byte_span buf, size_t num_glyphs)
	{
		auto start = buf;
		auto state_table = parseExtendedStateTable(buf, num_glyphs);

		auto ligature_actions = start.drop(consume_u32(buf));
		auto component_table = start.drop(consume_u32(buf));
		auto ligatures = start.drop(consume_u32(buf));

		return MorxLigatureSubtable {
			.state_table = std::move(state_table),
			.ligature_actions = ligature_actions.cast<uint32_t, std::endian::big>(),
			.component_table = component_table.cast<uint16_t, std::endian::big>(),
			.ligatures = ligatures.cast<uint16_t, std::endian::big>(),
		};
	}

	static MorxNonContextualSubtable parse_non_contextual_subtable(zst::byte_span buf, size_t num_glyphs)
	{
		return MorxNonContextualSubtable { .lookup = buf };
	}

	static MorxInsertionSubtable parse_insertion_subtable(zst::byte_span buf, size_t num_glyphs)
	{
		auto start = buf;
		auto state_table = parseExtendedStateTable(buf, num_glyphs);

		auto insertion_glyph_table = start.drop(consume_u32(buf));

		return MorxInsertionSubtable {
			.state_table = std::move(state_table),
			.insertion_glyph_table = insertion_glyph_table,
		};
	}

	static std::optional<MorxSubtable> parse_morx_subtable(zst::byte_span& buf, size_t num_glyphs)
	{
		auto table_start = buf;

		auto len = consume_u32(buf);
		auto cov = consume_u32(buf);
		auto flags = consume_u32(buf);

		auto type = (cov & 0xff);
		if(type == 3 || type > 5)
		{
			sap::warn("font/aat", "unsupported subtable type {}", type);
			buf.remove_prefix(len - 3 * sizeof(uint32_t));
			return std::nullopt;
		}

		MorxSubtableCommon com {
			.flags = flags,
			.process_logical_order = bool(cov & 0x10000000),
			.process_descending_order = bool(cov & 0x40000000),
			.only_vertical = bool(cov & 0x80000000),
			.both_horizontal_and_vertical = bool(cov & 0x20000000),
			.table_start = table_start,
		};

		MorxSubtable subtable {};

		auto subtable_data = buf.take(len - 3 * sizeof(uint32_t));
		if(type == 0)
			subtable = parse_rearrangement_subtable(subtable_data, num_glyphs);
		else if(type == 1)
			subtable = parse_contextual_subtable(subtable_data, num_glyphs);
		else if(type == 2)
			subtable = parse_ligature_subtable(subtable_data, num_glyphs);
		else if(type == 4)
			subtable = parse_non_contextual_subtable(subtable_data, num_glyphs);
		else if(type == 5)
			subtable = parse_insertion_subtable(subtable_data, num_glyphs);
		else
			assert(false && "unreachable!");


		buf.remove_prefix(len - 3 * sizeof(uint32_t));
		std::visit([&com](auto& v) { v.common = com; }, subtable);

		return subtable;
	}

	static MorxChain parse_morx_chain(zst::byte_span buf, bool has_subtable_coverage, size_t num_glyphs)
	{
		auto default_flags = consume_u32(buf);
		auto chain_len = consume_u32(buf);
		assert(chain_len == buf.size() + 8);

		auto num_feature_entries = consume_u32(buf);
		auto num_subtables = consume_u32(buf);

		MorxChain chain {};
		chain.default_flags = default_flags;

		for(auto i = 0u; i < num_feature_entries; i++)
			chain.features.push_back(parse_morx_feature(buf));

		for(auto i = 0u; i < num_subtables; i++)
		{
			if(auto s = parse_morx_subtable(buf, num_glyphs); s.has_value())
				chain.subtables.push_back(std::move(*s));
		}

		return chain;
	}
}

namespace font::aat
{
	inline constexpr uint16_t CLASS_CODE_END_OF_TEXT = 0;
	inline constexpr uint16_t CLASS_CODE_OUT_OF_BOUNDS = 1;
	inline constexpr uint16_t CLASS_CODE_DELETED_GLYPH = 2;
	inline constexpr uint16_t CLASS_CODE_END_OF_LINE = 3;

	static size_t get_entry_index_for_state(const StateTable& machine, size_t state, uint16_t glyph_class)
	{
		auto tmp = machine.state_array.drop(state * machine.state_row_size);
		if(machine.is_extended)
			return peek_u16(tmp.drop(glyph_class * sizeof(uint16_t)));
		else
			return peek_u8(tmp.drop(glyph_class * sizeof(uint8_t)));
	}

	template <size_t EntrySize, typename Action>
	static void run_state_machine(const StateTable& machine,
	    zst::span<GlyphId>& glyphs,
	    bool is_reverse,
	    Action&& action)
	{
		// this is the same for all the tables
		constexpr uint16_t FLAG_DONT_ADVANCE = 0x4000;

		size_t current_state = 0;
		size_t i = (is_reverse ? glyphs.size() - 1 : 0);

		while(i < glyphs.size())
		{
			uint16_t glyph_class = 0;
			if(auto it = machine.glyph_classes.find(glyphs[i]); it != machine.glyph_classes.end())
				glyph_class = it->second;
			else
				glyph_class = CLASS_CODE_OUT_OF_BOUNDS;

			auto entry_idx = get_entry_index_for_state(machine, current_state, glyph_class);
			auto entry_start = machine.entry_array.drop(entry_idx * EntrySize);

			auto next_state = consume_u16(entry_start);
			auto flags = consume_u16(entry_start);

			action(i, flags, entry_start);

			// dont-advance flag
			if(not(flags & FLAG_DONT_ADVANCE))
			{
				if(is_reverse)
				{
					if(i == 0)
						break;
					i -= 1;
				}
				else
				{
					if(i == glyphs.size() - 1)
						break;
					i += 1;
				}
			}

			// for ligatures, these are indices into the state table.
			current_state = next_state;
		}
	}




	std::optional<SubstitutedGlyphString> apply_table(const MorxLigatureSubtable& table,
	    zst::span<GlyphId> glyphs,
	    bool is_reverse,
	    size_t num_font_glyphs)
	{
		constexpr uint16_t PERFORM_ACTION = 0x2000;
		constexpr uint16_t SET_COMPONENT = 0x8000;

		constexpr size_t EntrySize = 3 * sizeof(uint16_t);

		bool did_substitute = false;
		SubstitutedGlyphString ret;

		util::hashset<size_t> deleted_glyphs {};
		util::hashmap<size_t, GlyphId> replacements {};

		std::vector<size_t> glyph_stack {};

		auto runner = [&](size_t i, uint16_t flags, zst::byte_span extra) {
			auto lig_action_idx = consume_u16(extra);

			if(flags & SET_COMPONENT)
				glyph_stack.push_back(i);

			// perform action
			if(flags & PERFORM_ACTION)
			{
				did_substitute = true;

				uint32_t accum_idx = 0;
				std::vector<size_t> constituent_glyph_idxs {};

				while(true)
				{
					auto idx = glyph_stack.back();

					auto cur_glyph = glyphs[idx];

					// TODO: is this the right thing to do?
					assert(not deleted_glyphs.contains(idx));

					if(auto it = replacements.find(idx); it != replacements.end())
						cur_glyph = it->second;

					glyph_stack.pop_back();

					uint32_t lig_action = table.ligature_actions[lig_action_idx];
					int32_t component_offset = static_cast<int32_t>(lig_action << 2) >> 2;
					auto component_table_idx = uint32_t(int32_t(cur_glyph) + component_offset);

					accum_idx += table.component_table[component_table_idx];

					bool is_last = (lig_action & 0x80000000);
					bool should_store = is_last || (lig_action & 0x40000000);
					if(should_store)
					{
						constituent_glyph_idxs.push_back(idx);

						auto output_lig = GlyphId(table.ligatures[accum_idx]);
						replacements[idx] = output_lig;

						std::vector<GlyphId> constituent_glyphs {};
						for(auto k : constituent_glyph_idxs)
							constituent_glyphs.push_back(glyphs[k]);

						ret.mapping.contractions[output_lig] = std::move(constituent_glyphs);

						constituent_glyph_idxs.clear();
						accum_idx = 0;
					}
					else
					{
						deleted_glyphs.insert(idx);
					}

					if(is_last)
						break;

					lig_action_idx += 1;
				}

				glyph_stack.insert(glyph_stack.end(), constituent_glyph_idxs.begin(), constituent_glyph_idxs.end());
			}
		};

		run_state_machine<EntrySize>(table.state_table, glyphs, is_reverse, runner);

		if(not did_substitute)
			return std::nullopt;

		ret.glyphs.reserve(glyphs.size());
		for(size_t i = 0; i < glyphs.size(); i++)
		{
			if(deleted_glyphs.contains(i))
				continue;
			else if(auto it = replacements.find(i); it != replacements.end())
				ret.glyphs.push_back(it->second);
			else
				ret.glyphs.push_back(glyphs[i]);
		}

		return ret;
	}


	std::optional<SubstitutedGlyphString> apply_table(const MorxRearrangementSubtable& table,
	    zst::span<GlyphId> input,
	    bool is_reverse,
	    size_t num_font_glyphs)
	{
		constexpr uint16_t MARK_FIRST = 0x8000;
		constexpr uint16_t MARK_LAST = 0x2000;
		constexpr uint16_t VERB_MASK = 0xF;

		constexpr size_t EntrySize = 2 * sizeof(uint16_t);

		SubstitutedGlyphString ret;
		ret.glyphs = std::vector(input.begin(), input.end());
		auto& glyphs = ret.glyphs;

		size_t first_idx = 0;
		size_t last_idx = 0;

		auto runner = [&](size_t idx, uint16_t flags, zst::byte_span extra) {
			if(flags & MARK_FIRST)
				first_idx = idx;

			if(flags & MARK_LAST)
				last_idx = idx;

			auto verb = flags & VERB_MASK;
			auto it_first = glyphs.begin() + ssize_t(first_idx);
			auto it_last = glyphs.begin() + ssize_t(last_idx) + 1;

			switch(verb)
			{
				case 0x0: break;
				case 0x1: { // Ax => xA
					auto A = glyphs[first_idx];
					std::shift_left(it_first, it_last, 1);
					glyphs[last_idx] = A;
					break;
				}

				case 0x2: { // xD => Dx
					auto D = glyphs[last_idx];
					std::shift_right(it_first, it_last, 1);
					glyphs[first_idx] = D;
					break;
				}

				case 0x3: { // AxD => DxA
					std::swap(glyphs[first_idx], glyphs[last_idx]);
					break;
				}

				case 0x4: { // ABx => xAB
					auto A = glyphs[first_idx];
					auto B = glyphs[first_idx + 1];
					std::shift_left(it_first, it_last, 2);
					glyphs[last_idx - 1] = A;
					glyphs[last_idx - 0] = B;
					break;
				}

				case 0x5: { // ABx => xBA
					auto A = glyphs[first_idx];
					auto B = glyphs[first_idx + 1];
					std::shift_left(it_first, it_last, 2);
					glyphs[last_idx - 1] = B;
					glyphs[last_idx - 0] = A;
					break;
				}

				case 0x6: { // xCD => CDx
					auto C = glyphs[last_idx - 1];
					auto D = glyphs[last_idx - 0];
					std::shift_right(it_first, it_last, 2);
					glyphs[first_idx + 0] = C;
					glyphs[first_idx + 1] = D;
					break;
				}

				case 0x7: { // xCD => DCx
					auto C = glyphs[last_idx - 1];
					auto D = glyphs[last_idx - 0];
					std::shift_right(it_first, it_last, 2);
					glyphs[first_idx + 0] = D;
					glyphs[first_idx + 1] = C;
					break;
				}

				case 0x8: { // AxCD => CDxA
					auto A = glyphs[first_idx];
					auto C = glyphs[last_idx - 1];
					auto D = glyphs[last_idx - 0];
					std::shift_right(it_first, it_last, 1);
					glyphs[first_idx + 0] = C;
					glyphs[first_idx + 1] = D;
					glyphs[last_idx] = A;
					break;
				}

				case 0x9: { // AxCD => DCxA
					auto A = glyphs[first_idx];
					auto C = glyphs[last_idx - 1];
					auto D = glyphs[last_idx - 0];
					std::shift_right(it_first, it_last, 1);
					glyphs[first_idx + 0] = D;
					glyphs[first_idx + 1] = C;
					glyphs[last_idx] = A;
					break;
				}

				case 0xA: { // ABxD => DxAB
					auto A = glyphs[first_idx + 0];
					auto B = glyphs[first_idx + 1];
					auto D = glyphs[last_idx];
					std::shift_left(it_first, it_last, 1);
					glyphs[first_idx + 0] = D;
					glyphs[last_idx - 1] = A;
					glyphs[last_idx - 0] = B;
					break;
				}

				case 0xB: { // ABxD => DxBA
					auto A = glyphs[first_idx + 0];
					auto B = glyphs[first_idx + 1];
					auto D = glyphs[last_idx];
					std::shift_left(it_first, it_last, 1);
					glyphs[first_idx + 0] = D;
					glyphs[last_idx - 1] = B;
					glyphs[last_idx - 0] = A;
					break;
				}

				case 0xC: { // ABxCD => CDxAB
					auto A = glyphs[first_idx + 0];
					auto B = glyphs[first_idx + 1];
					auto C = glyphs[last_idx - 1];
					auto D = glyphs[last_idx - 0];
					glyphs[first_idx + 0] = C;
					glyphs[first_idx + 1] = D;
					glyphs[last_idx - 1] = A;
					glyphs[last_idx - 0] = B;
					break;
				}

				case 0xD: { // ABxCD => CDxBA
					auto A = glyphs[first_idx + 0];
					auto B = glyphs[first_idx + 1];
					auto C = glyphs[last_idx - 1];
					auto D = glyphs[last_idx - 0];
					glyphs[first_idx + 0] = C;
					glyphs[first_idx + 1] = D;
					glyphs[last_idx - 1] = B;
					glyphs[last_idx - 0] = A;
					break;
				}

				case 0xE: { // ABxCD => DCxAB
					auto A = glyphs[first_idx + 0];
					auto B = glyphs[first_idx + 1];
					auto C = glyphs[last_idx - 1];
					auto D = glyphs[last_idx - 0];
					glyphs[first_idx + 0] = D;
					glyphs[first_idx + 1] = C;
					glyphs[last_idx - 1] = A;
					glyphs[last_idx - 0] = B;
					break;
				}

				case 0xF: { // ABxCD => DCxBA
					auto A = glyphs[first_idx + 0];
					auto B = glyphs[first_idx + 1];
					auto C = glyphs[last_idx - 1];
					auto D = glyphs[last_idx - 0];
					glyphs[first_idx + 0] = D;
					glyphs[first_idx + 1] = C;
					glyphs[last_idx - 1] = B;
					glyphs[last_idx - 0] = A;
					break;
				}
			}
		};

		run_state_machine<EntrySize>(table.state_table, input, is_reverse, runner);

		return ret;
	}

	std::optional<SubstitutedGlyphString> apply_table(const MorxContextualSubtable& table,
	    zst::span<GlyphId> glyphs,
	    bool is_reverse,
	    size_t num_font_glyphs)
	{
		constexpr uint16_t SET_MARK = 0x8000;
		constexpr size_t EntrySize = 4 * sizeof(uint16_t);

		bool did_substitute = false;

		util::hashmap<size_t, GlyphId> replacements {};
		size_t marked_glyph_idx = 0;

		auto runner = [&](size_t idx, uint16_t flags, zst::byte_span extra) {
			auto mark_idx = consume_u16(extra);
			auto curr_idx = consume_u16(extra);

			auto subst_offsets = table.substitution_tables.cast<uint32_t, std::endian::big>();

			if(mark_idx != 0xffff)
			{
				did_substitute = true;
				auto offset = subst_offsets[mark_idx];
				auto lookup_table = table.substitution_tables.drop(offset);

				if(auto rep = searchLookupTable(lookup_table, num_font_glyphs, glyphs[marked_glyph_idx]);
				    rep.has_value())
					replacements[marked_glyph_idx] = GlyphId(*rep);
			}

			if(curr_idx != 0xffff)
			{
				did_substitute = true;
				auto offset = subst_offsets[curr_idx];
				auto lookup_table = table.substitution_tables.drop(offset);

				if(auto rep = searchLookupTable(lookup_table, num_font_glyphs, glyphs[idx]); rep.has_value())
					replacements[idx] = GlyphId(*rep);
			}

			if(flags & SET_MARK)
				marked_glyph_idx = idx;
		};

		run_state_machine<EntrySize>(table.state_table, glyphs, is_reverse, runner);

		if(not did_substitute)
			return std::nullopt;

		SubstitutedGlyphString ret;
		ret.glyphs.reserve(glyphs.size());
		for(size_t i = 0; i < glyphs.size(); i++)
		{
			if(auto it = replacements.find(i); it != replacements.end())
				ret.glyphs.push_back(it->second);
			else
				ret.glyphs.push_back(glyphs[i]);
		}

		return ret;
	}

	std::optional<SubstitutedGlyphString> apply_table(const MorxNonContextualSubtable& table,
	    zst::span<GlyphId> glyphs,
	    bool is_reverse,
	    size_t num_font_glyphs)
	{
		bool did_substitute = false;

		util::hashmap<size_t, GlyphId> replacements {};
		for(size_t i = 0; i < glyphs.size(); i++)
		{
			if(auto ret = searchLookupTable(table.lookup, num_font_glyphs, glyphs[i]); ret.has_value())
				replacements[i] = GlyphId(*ret);
		}

		if(not did_substitute)
			return std::nullopt;

		SubstitutedGlyphString ret;
		ret.glyphs.reserve(glyphs.size());
		for(size_t i = 0; i < glyphs.size(); i++)
		{
			if(auto it = replacements.find(i); it != replacements.end())
				ret.glyphs.push_back(it->second);
			else
				ret.glyphs.push_back(glyphs[i]);
		}

		return ret;
	}

	std::optional<SubstitutedGlyphString> apply_table(const MorxInsertionSubtable& table,
	    zst::span<GlyphId> glyphs,
	    bool is_reverse,
	    size_t num_font_glyphs)
	{
		constexpr uint16_t SET_MARK = 0x8000;
		constexpr uint16_t CURRENT_INSERT_BEFORE = 0x800;
		constexpr uint16_t MARKED_INSERT_BEFORE = 0x400;
		constexpr uint16_t CURRENT_INSERT_COUNT_MASK = 0x3E0;
		constexpr uint16_t MARKED_INSERT_COUNT_MASK = 0x1F;

		constexpr size_t EntrySize = 4 * sizeof(uint16_t);

		SubstitutedGlyphString ret {};
		ret.glyphs = std::vector(glyphs.begin(), glyphs.end());

		size_t marked_glyph_idx = 0;

		auto update_span = [&ret]() { return zst::span<GlyphId>(ret.glyphs.data(), ret.glyphs.size()); };

		glyphs = update_span();

		auto insert_glyphs = [&](size_t at, size_t count, zst::byte_span array, size_t start_idx, bool insert_before) {
			auto tmp = array.cast<uint16_t, std::endian::big>().drop(start_idx).take(count);

			std::vector<GlyphId> to_insert {};
			for(size_t i = 0; i < count; i++)
				to_insert.push_back(GlyphId(tmp[i]));

			ret.glyphs.insert(ret.glyphs.begin() + ssize_t(at) + (insert_before ? 0 : 1), to_insert.begin(),
			    to_insert.end());
			ret.mapping.extra_glyphs.insert(to_insert.begin(), to_insert.end());

			glyphs = update_span();
		};

		auto runner = [&](size_t idx, uint16_t flags, zst::byte_span extra) {
			if(flags & SET_MARK)
				marked_glyph_idx = idx;

			auto curr_insert_idx = consume_u16(extra);
			auto mark_insert_idx = consume_u16(extra);

			if(mark_insert_idx != 0xffff)
			{
				auto insert_count = static_cast<size_t>((flags & MARKED_INSERT_COUNT_MASK) >> 0);
				insert_glyphs(marked_glyph_idx, insert_count, table.insertion_glyph_table, mark_insert_idx,
				    flags & MARKED_INSERT_BEFORE);
			}

			if(curr_insert_idx != 0xffff)
			{
				auto insert_count = static_cast<size_t>((flags & CURRENT_INSERT_COUNT_MASK) >> 5);
				insert_glyphs(marked_glyph_idx, insert_count, table.insertion_glyph_table, curr_insert_idx,
				    flags & CURRENT_INSERT_BEFORE);
			}
		};

		run_state_machine<EntrySize>(table.state_table, glyphs, is_reverse, runner);

		return ret;
	}






	static SubstitutionMapping& combine_subst_mapping(SubstitutionMapping& to, SubstitutionMapping&& from)
	{
		to.extra_glyphs.insert(std::move_iterator(from.extra_glyphs.begin()),
		    std::move_iterator(from.extra_glyphs.end()));
		to.contractions.insert(std::move_iterator(from.contractions.begin()),
		    std::move_iterator(from.contractions.end()));
		to.replacements.insert(std::move_iterator(from.replacements.begin()),
		    std::move_iterator(from.replacements.end()));
		return to;
	}


	std::optional<SubstitutedGlyphString> performSubstitutionsForGlyphSequence(const MorxTable& morx,
	    zst::span<GlyphId> glyphs,
	    const FeatureSet& features)
	{
		std::optional<SubstitutedGlyphString> ret {};

		util::hashset<aat::Feature> enabled_features {};
		util::hashset<aat::Feature> disabled_features {};

		for(auto& f : features.enabled_features)
		{
			auto tmp = convertOFFFeatureToAAT(f);
			enabled_features.insert(tmp.begin(), tmp.end());
		}

		for(auto& f : features.disabled_features)
		{
			auto tmp = convertOFFFeatureToAAT(f);
			disabled_features.insert(tmp.begin(), tmp.end());
		}

		// TODO: handle feature selection
		for(auto& chain : morx.chains)
		{
			uint32_t flags = chain.default_flags;
			for(auto& feat : chain.features)
			{
				if(disabled_features.contains(feat.feature))
				{
					flags &= ~feat.enable_flags;
				}
				else if(enabled_features.contains(feat.feature))
				{
					flags |= feat.enable_flags;
					flags &= feat.disable_flags;
				}
			}

			auto get_common = [](const auto& subtable) -> const MorxSubtableCommon& {
				return std::visit([](const auto& x) -> const auto& { return x.common; }, subtable);
			};

			size_t tmp = 0;
			for(auto& subtable : chain.subtables)
			{
				auto& common = get_common(subtable);
				if(not(common.flags & flags))
					continue;

				// assume we are only dealing with horizontal text
				if(common.only_vertical && not common.both_horizontal_and_vertical)
					continue;

				// assume we do not do right-to-left text; logical order is always the same
				// as layout order in this case.
				bool reverse_glyphs = common.process_descending_order;

				if(not ret.has_value())
				{
					ret = SubstitutedGlyphString();
					ret->glyphs = std::vector(glyphs.begin(), glyphs.end());
				}

				auto visitor = [&](auto& t) {
					auto gs = zst::span<GlyphId>(ret->glyphs.data(), ret->glyphs.size());
					return apply_table(t, gs, reverse_glyphs, morx.num_font_glyphs);
				};

				if(auto result = std::visit(visitor, subtable); result.has_value())
				{
					ret->glyphs = std::move(result->glyphs);
					combine_subst_mapping(ret->mapping, std::move(result->mapping));
				}

				tmp++;
			}
		}

		return ret;
	}
}

namespace font
{
	void FontFile::parse_morx_table(const Table& morx)
	{
		auto buf = this->bytes().drop(morx.offset);
		auto version = consume_u16(buf);
		(void) consume_u16(buf); // unused

		if(version < 2 || version > 3)
		{
			sap::warn("font/aat", "invalid morx table version {}", version);
			return;
		}

		aat::MorxTable morx_table {};
		morx_table.num_font_glyphs = m_num_glyphs;

		auto num_chains = consume_u32(buf);
		for(auto i = 0u; i < num_chains; i++)
		{
			auto chain_start = buf;

			// do a little peeking to figure out how long the chain is
			(void) consume_u32(buf);
			auto chain_len = consume_u32(buf);

			morx_table.chains.push_back(aat::parse_morx_chain(chain_start.take(chain_len),
			    /* have_subtable_coverage: */ version >= 3, m_num_glyphs));

			buf.remove_prefix(chain_len - 4);
		}

		m_morx_table = std::move(morx_table);
	}
}
