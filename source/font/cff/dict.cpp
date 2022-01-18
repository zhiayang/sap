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
				dict[key] = std::move(values);
		};

		add_default(DictKey::FontBBox, {
			Operand().integer(0), Operand().integer(0), Operand().integer(0), Operand().integer(0)
		});

		add_default(DictKey::charset, { Operand().integer(0) });
		add_default(DictKey::Encoding, { Operand().integer(0) });

		add_default(DictKey::isFixedPitch, { Operand().integer(0) });
		add_default(DictKey::ItalicAngle, { Operand().integer(0) });
		add_default(DictKey::UnderlinePosition, { Operand().integer(-100) });
		add_default(DictKey::UnderlineThickness, { Operand().integer(50) });
		add_default(DictKey::PaintType, { Operand().integer(0) });
		add_default(DictKey::CharstringType, { Operand().integer(2) });

		add_default(DictKey::FontMatrix, {
			Operand().decimal(0.001), Operand().decimal(0), Operand().decimal(0),
			Operand().decimal(0.001), Operand().decimal(0), Operand().decimal(0)
		});

		add_default(DictKey::StrokeWidth, { Operand().integer(0) });

		add_default(DictKey::CIDFontVersion, { Operand().integer(0) });
		add_default(DictKey::CIDFontRevision, { Operand().integer(0) });
		add_default(DictKey::CIDFontType, { Operand().integer(0) });
		add_default(DictKey::CIDCount, { Operand().integer(8720) });
	}

	uint16_t Dictionary::string_id(DictKey key) const
	{
		if(auto it = this->values.find(key); it != this->values.end() && it->second.size() > 0)
			return it->second[0].string_id();
		else
			return 0;
	}

	int32_t Dictionary::integer(DictKey key) const
	{
		if(auto it = this->values.find(key); it != this->values.end() && it->second.size() > 0)
			return it->second[0].integer();
		else
			return 0;
	}

	double Dictionary::decimal(DictKey key) const
	{
		if(auto it = this->values.find(key); it != this->values.end() && it->second.size() > 0)
			return it->second[0].decimal();
		else
			return 0;
	}

	double Dictionary::fixed(DictKey key) const
	{
		if(auto it = this->values.find(key); it != this->values.end() && it->second.size() > 0)
			return it->second[0].fixed();
		else
			return 0;
	}
}
