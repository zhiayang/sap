// common.cpp
// Copyright (c) 2021, zhiayang
// SPDX-License-Identifier: Apache-2.0

#include "font/tag.h" // for Tag
#include "font/misc.h"
#include "font/features.h"  // for LookupTable, TaggedTable, Feature, Script
#include "font/font_file.h" // for consume_u16, FontFile, consume_u32, Table

namespace font::off
{
	constexpr uint16_t NO_REQUIRED_FEATURE = 0xFFFF;
	constexpr auto DEFAULT = Tag("DFLT");

	template <typename TableKind>
	std::vector<uint16_t> getLookupTablesForFeatures(const TableKind& table, const FeatureSet& features)
	{
		// step 1: get the script.
		const Script* script = nullptr;
		if(auto it = table.scripts.find(features.script); it != table.scripts.end())
			script = &it->second;
		else if(auto it = table.scripts.find(DEFAULT); it != table.scripts.end())
			script = &it->second;
		else
			return {}; // found nothing

		assert(script != nullptr);

		// step 2: same thing for the language
		const Language* lang = nullptr;
		if(auto it = script->languages.find(features.language); it != script->languages.end())
			lang = &it->second;
		else if(auto it = script->languages.find(DEFAULT); it != script->languages.end())
			lang = &it->second;
		else
			return {}; // nothing again

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

	template std::vector<uint16_t> getLookupTablesForFeatures<GPosTable>(const GPosTable& table, const FeatureSet& features);
	template std::vector<uint16_t> getLookupTablesForFeatures<GSubTable>(const GSubTable& table, const FeatureSet& features);

	static Language parse_one_language(Tag tag, zst::byte_span buf)
	{
		Language lang {};
		lang.tag = tag;

		consume_u16(buf); // reserved
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

		// we have the 2 byte "default_langsys" to account for.
		auto langsys_tables = parseTaggedList(buf, /* starting_offset: */ sizeof(uint16_t));
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

	static Feature parse_one_feature(Tag tag, zst::byte_span buf)
	{
		Feature feature {};
		feature.tag = tag;

		auto table_start = buf;

		if(auto param_ofs = consume_u16(buf); param_ofs != 0)
			feature.parameters_table = table_start.drop(param_ofs);

		auto num_lookups = consume_u16(buf);
		for(size_t i = 0; i < num_lookups; i++)
			feature.lookups.push_back(consume_u16(buf));

		return feature;
	}

	std::vector<Feature> parseFeatureList(zst::byte_span buf)
	{
		std::vector<Feature> features {};
		auto feature_tables = parseTaggedList(buf);

		for(auto& feat : feature_tables)
			features.push_back(parse_one_feature(feat.tag, feat.data));

		return features;
	}


	std::vector<LookupTable> parseLookupList(zst::byte_span buf)
	{
		auto table_start = buf;
		auto num_lookups = consume_u16(buf);

		std::vector<LookupTable> table_list {};

		for(size_t i = 0; i < num_lookups; i++)
		{
			auto offset = consume_u16(buf);
			auto tbl_buf = table_start.drop(offset);
			auto lookup_start = tbl_buf;

			LookupTable lookup {};
			lookup.type = consume_u16(tbl_buf);
			lookup.flags = consume_u16(tbl_buf);

			// handle extension types here, so we don't need to go in and fuck around later
			if(lookup.type == gpos::LOOKUP_EXTENSION_POS || lookup.type == gsub::LOOKUP_EXTENSION_SUBST)
			{
				auto num_subtables = consume_u16(tbl_buf);
				for(uint16_t i = 0; i < num_subtables; i++)
				{
					auto proxy = lookup_start.drop(consume_u16(tbl_buf));
					auto proxy_start = proxy;

					auto format = consume_u16(proxy);
					if(format != 1)
					{
						sap::warn("font/otf", "unknown subtable format '{}' for GPOS/GSUB extension");
						continue;
					}

					// change the lookup type (all the subtables must have the same type anyway)
					lookup.type = consume_u16(proxy);
					lookup.subtables.push_back(proxy_start.drop(consume_u32(proxy)));
				}
			}
			else
			{
				auto num_subtables = consume_u16(tbl_buf);
				for(uint16_t i = 0; i < num_subtables; i++)
					lookup.subtables.push_back(lookup_start.drop(consume_u16(tbl_buf)));
			}

			lookup.mark_filtering_set = consume_u16(tbl_buf);

			table_list.push_back(std::move(lookup));
		}

		return table_list;
	}

	std::vector<TaggedTable> parseTaggedList(zst::byte_span buf, size_t starting_offset)
	{
		std::vector<TaggedTable> ret {};

		auto table_start = buf;
		auto num = consume_u16(buf);
		for(size_t i = 0; i < num; i++)
		{
			auto tag = Tag(consume_u32(buf));
			auto ofs = consume_u16(buf);

			ret.push_back({ tag, zst::byte_span(table_start.drop(ofs - starting_offset)) });
		}

		return ret;
	}


	template <typename TableType>
	static std::optional<TableType> parse_gpos_or_gsub_table(zst::byte_span buf)
	{
		auto table_start = buf;

		auto major = consume_u16(buf);
		auto minor = consume_u16(buf);

		if(major != 1 || (minor != 0 && minor != 1))
		{
			zpr::println("warning: unsupported GPOS/GSUB table version {}.{}, ignoring", major, minor);
			return std::nullopt;
		}

		auto script_list_ofs = consume_u16(buf);
		auto feature_list_ofs = consume_u16(buf);
		auto lookup_list_ofs = consume_u16(buf);

		TableType ret {};
		ret.scripts = parseScriptAndLanguageTables(table_start.drop(script_list_ofs));
		ret.features = parseFeatureList(table_start.drop(feature_list_ofs));
		ret.lookups = parseLookupList(table_start.drop(lookup_list_ofs));

		if(major == 1 && minor == 1)
		{
			if(auto feat_var_ofs = consume_u16(buf); feat_var_ofs != 0)
				ret.feature_variations_table = table_start.drop(feat_var_ofs);
		}

		return ret;
	}
}

namespace font
{
	void FontFile::parse_gsub_table(const Table& table)
	{
		m_gsub_table = off::parse_gpos_or_gsub_table<off::GSubTable>(this->bytes().drop(table.offset));
	}

	void FontFile::parse_gpos_table(const Table& table)
	{
		m_gpos_table = off::parse_gpos_or_gsub_table<off::GPosTable>(this->bytes().drop(table.offset));
	}
}
