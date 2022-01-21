// common.cpp
// Copyright (c) 2021, zhiayang
// SPDX-License-Identifier: Apache-2.0

#include "util.h"
#include "error.h"
#include "font/font.h"
#include "font/features.h"

namespace font::off
{
	constexpr uint16_t NO_REQUIRED_FEATURE = 0xFFFF;
	constexpr auto DEFAULT = Tag("DFLT");

	template <typename TableKind>
	std::vector<uint16_t> getLookupTablesForFeatures(TableKind& table, const FeatureSet& features)
	{
		// step 1: get the script.
		Script* script = nullptr;
		if(auto it = table.scripts.find(features.script); it != table.scripts.end())
			script = &it->second;
		else if(auto it = table.scripts.find(DEFAULT); it != table.scripts.end())
			script = &it->second;
		else
			return {};  // found nothing

		assert(script != nullptr);

		// step 2: same thing for the language
		Language* lang = nullptr;
		if(auto it = script->languages.find(features.language); it != script->languages.end())
			lang = &it->second;
		else if(auto it = script->languages.find(DEFAULT); it != script->languages.end())
			lang = &it->second;
		else
			return {};  // nothing again

		assert(lang != nullptr);

		std::vector<uint16_t> lookups {};

		auto add_lookups_for_feature = [&](uint16_t idx, bool required) {
			assert(idx < table.features.size());

			auto& feature = table.features[idx];
			if(required || (features.enabled_features.find(feature.tag) != features.enabled_features.end()))
				lookups.insert(lookups.end(), feature.lookups.begin(), feature.lookups.end());
		};


		if(lang->required_feature.has_value())
			add_lookups_for_feature(*lang->required_feature, /* required: */ true);

		for(auto feat_idx : lang->features)
			add_lookups_for_feature(feat_idx, /* required: */ false);

		return lookups;
	}



	template std::vector<uint16_t> getLookupTablesForFeatures(GPosTable& table, const FeatureSet& features);

	// template
	// std::vector<size_t> getLookupTablesForFeatures(GSubTable& table, const FeatureSet& features);

	static Language parse_one_language(Tag tag, zst::byte_span buf)
	{
		Language lang {};
		lang.tag = tag;

		consume_u16(buf);   // reserved
		if(auto req = consume_u16(buf); req != NO_REQUIRED_FEATURE)
			lang.required_feature = req;

		auto num_features = consume_u16(buf);
		for(size_t i = 0; i < num_features; i++)
			lang.features.push_back(consume_u16(buf));

		return lang;
	}

	static Script parse_one_script(Tag tag, zst::byte_span buf)
	{
		auto table_start = buf;

		auto default_langsys = consume_u16(buf);

		Script script {};
		script.tag = tag;

		// if the default langsys is specified, then it is the langsys for the DFLT language
		if(default_langsys != 0)
			script.languages[DEFAULT] = parse_one_language(DEFAULT, table_start.drop(default_langsys));

		auto langsys_tables = parseTaggedList(buf);
		for(auto& langsys : langsys_tables)
			script.languages[langsys.tag] = parse_one_language(langsys.tag, langsys.data);

		return script;
	}


	std::map<Tag, Script> parseScriptAndLanguageTables(zst::byte_span buf)
	{
		std::map<Tag, Script> scripts {};
		auto script_tables = parseTaggedList(buf);

		for(auto& script : script_tables)
			scripts[script.tag] = parse_one_script(script.tag, script.data);

		return scripts;
	}

	std::vector<TaggedTable2> parseTaggedList(zst::byte_span buf)
	{
		std::vector<TaggedTable2> ret {};

		auto table_start = buf;
		auto num = consume_u16(buf);
		for(size_t i = 0; i < num; i++)
		{
			auto tag = Tag(consume_u32(buf));
			auto ofs = consume_u16(buf);

			ret.push_back({ tag, zst::byte_span(table_start.drop(ofs)) });
		}

		return ret;
	}



















	std::vector<TaggedTable> parseTaggedList(FontFile* font, zst::byte_span list)
	{
		auto table_start = list;

		std::vector<TaggedTable> ret;

		auto num = consume_u16(list);
		for(size_t i = 0; i < num; i++)
		{
			auto tag = Tag(consume_u32(list));
			auto ofs = consume_u16(list);

			ret.push_back({ tag, static_cast<size_t>(ofs + (table_start.data() - font->file_bytes)) });
		}

		return ret;
	}

	std::vector<LookupTable> parseLookupList(FontFile* font, zst::byte_span buf)
	{
		auto table_start = buf;
		auto num_lookups = consume_u16(buf);

		std::vector<LookupTable> table_list {};

		for(size_t i = 0; i < num_lookups; i++)
		{
			auto offset = consume_u16(buf);
			auto tbl_buf = table_start.drop(offset);

			LookupTable lookup_table {};
			lookup_table.data = tbl_buf;

			lookup_table.type = consume_u16(tbl_buf);
			lookup_table.flags = consume_u16(tbl_buf);

			auto num_subtables = consume_u16(tbl_buf);
			for(uint16_t i = 0; i < num_subtables; i++)
				lookup_table.subtable_offsets.push_back(consume_u16(tbl_buf));

			table_list.push_back(std::move(lookup_table));
		}

		return table_list;
	}
}
