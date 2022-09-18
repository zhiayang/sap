// dict.cpp
// Copyright (c) 2022, zhiayang
// SPDX-License-Identifier: Apache-2.0

#include "error.h"
#include "font/cff.h"

namespace font::cff
{
	static void ensure_bytes(zst::byte_span buf, size_t num)
	{
		if(buf.size() < num)
			sap::error("font/cff", "malformed DICT data: unexpected end of data");
	}

	Dictionary readDictionary(zst::byte_span dict)
	{
		Dictionary dictionary {};
		std::vector<Operand> operands {};

		while(dict.size() > 0)
		{
			auto foo = dict[0];
			if(foo < 28)
			{
				uint16_t oper = foo;
				dict.remove_prefix(1);
				if(foo == 12)
				{
					ensure_bytes(dict, 1);
					oper = 0x0C00 | dict[0];
					dict.remove_prefix(1);
				}

				dictionary.values[static_cast<DictKey>(oper)] = std::move(operands);
				operands.clear();
			}
			else
			{
				operands = readOperandsFromDICT(dict);
			}
		}

		return dictionary;
	}


	void populateDefaultValuesForTopDict(Dictionary& dict)
	{
		auto add_default = [&dict](DictKey key, std::vector<Operand> values) {
			if(dict.values.find(key) == dict.values.end())
				dict.values[key] = std::move(values);
		};

		constexpr DictKey keys_with_defaults[] = { DictKey::FontBBox, DictKey::charset, DictKey::Encoding, DictKey::isFixedPitch,
			DictKey::ItalicAngle, DictKey::UnderlinePosition, DictKey::UnderlineThickness, DictKey::PaintType,
			DictKey::CharstringType, DictKey::FontMatrix, DictKey::StrokeWidth, DictKey::CIDFontVersion, DictKey::CIDFontRevision,
			DictKey::CIDFontType, DictKey::CIDCount };

		for(auto key : keys_with_defaults)
			add_default(key, *getDefaultValueForDictKey(key));
	}

	std::optional<std::vector<Operand>> getDefaultValueForDictKey(DictKey key)
	{
#define OP_INT(x) Operand().integer(x)
#define OP_DEC(x) Operand().decimal(x)

		if(key == DictKey::FontBBox)
			return std::vector<Operand> { OP_INT(0), OP_INT(0), OP_INT(0), OP_INT(0) };
		else if(key == DictKey::charset)
			return std::vector<Operand> { OP_INT(0) };
		else if(key == DictKey::Encoding)
			return std::vector<Operand> { OP_INT(0) };
		else if(key == DictKey::isFixedPitch)
			return std::vector<Operand> { OP_INT(0) };
		else if(key == DictKey::ItalicAngle)
			return std::vector<Operand> { OP_INT(0) };
		else if(key == DictKey::UnderlinePosition)
			return std::vector<Operand> { OP_INT(-100) };
		else if(key == DictKey::UnderlineThickness)
			return std::vector<Operand> { OP_INT(50) };
		else if(key == DictKey::PaintType)
			return std::vector<Operand> { OP_INT(0) };
		else if(key == DictKey::CharstringType)
			return std::vector<Operand> { OP_INT(2) };
		else if(key == DictKey::StrokeWidth)
			return std::vector<Operand> { OP_INT(0) };
		else if(key == DictKey::CIDFontVersion)
			return std::vector<Operand> { OP_INT(0) };
		else if(key == DictKey::CIDFontRevision)
			return std::vector<Operand> { OP_INT(0) };
		else if(key == DictKey::CIDFontType)
			return std::vector<Operand> { OP_INT(0) };
		else if(key == DictKey::CIDCount)
			return std::vector<Operand> { OP_INT(8720) };
		else if(key == DictKey::FontMatrix)
			return std::vector<Operand> { OP_DEC(0.001), OP_DEC(0), OP_DEC(0), OP_DEC(0.001), OP_DEC(0), OP_DEC(0) };
		else
			return std::nullopt;

#undef OP_INT
#undef OP_DEC
	}




	std::vector<Operand> Dictionary::get(DictKey key) const
	{
		if(auto it = this->values.find(key); it != this->values.end())
			return it->second;

		else if(auto def = getDefaultValueForDictKey(key); def.has_value())
			return *def;

		else
			return {};
	}

	uint16_t Dictionary::string_id(DictKey key) const
	{
		if(auto it = this->values.find(key); it != this->values.end() && it->second.size() > 0)
			return it->second[0].string_id();

		else if(auto def = getDefaultValueForDictKey(key); def.has_value())
			return (*def)[0].string_id();

		else
			return 0;
	}

	int32_t Dictionary::integer(DictKey key) const
	{
		if(auto it = this->values.find(key); it != this->values.end() && it->second.size() > 0)
			return it->second[0].integer();

		else if(auto def = getDefaultValueForDictKey(key); def.has_value())
			return (*def)[0].integer();

		else
			return 0;
	}

	double Dictionary::decimal(DictKey key) const
	{
		if(auto it = this->values.find(key); it != this->values.end() && it->second.size() > 0)
			return it->second[0].decimal();

		else if(auto def = getDefaultValueForDictKey(key); def.has_value())
			return (*def)[0].decimal();

		else
			return 0;
	}

	double Dictionary::fixed(DictKey key) const
	{
		if(auto it = this->values.find(key); it != this->values.end() && it->second.size() > 0)
			return it->second[0].fixed();

		else if(auto def = getDefaultValueForDictKey(key); def.has_value())
			return (*def)[0].fixed();

		else
			return 0;
	}
}
