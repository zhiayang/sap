// cff_builder.cpp
// Copyright (c) 2022, zhiayang
// SPDX-License-Identifier: Apache-2.0

#include "util.h"
#include "error.h"
#include "font/cff.h"
#include "font/font.h"

namespace font::cff
{
	IndexTableBuilder& IndexTableBuilder::add(zst::byte_span data)
	{
		if(m_offsets.size() > 65535)
			sap::error("font/cff", "too many entries in CFF INDEX");

		m_offsets.push_back(m_data.size());
		m_data.append(data);

		return *this;
	}

	IndexTableBuilder& IndexTableBuilder::add(const IndexTable& table)
	{
		for(size_t i = 0; i < table.count; i++)
			this->add(table.get_item(i));

		return *this;
	}

	void IndexTableBuilder::writeInto(zst::byte_buffer& buf)
	{
		// don't wanna support 3 byte offsets
		int off_size = 0;
		if(m_data.size() < 255)
			off_size = 1;
		else if(m_data.size() < 65535)
			off_size = 2;
		else
			off_size = 4;

		auto write_offset = [&off_size, &buf](uint32_t offset) {
			if(off_size == 1)
				buf.append_bytes(static_cast<uint8_t>(offset));
			else if(off_size == 2)
				buf.append_bytes(util::convertBEU16(offset));
			else
				buf.append_bytes(util::convertBEU32(offset));
		};

		buf.append_bytes(util::convertBEU16(m_offsets.size()));
		if(m_offsets.empty())
			return;

		buf.append(static_cast<uint8_t>(off_size));

		// the offsets are biased by 1 (so the first entry has an offset of 1, not 0)
		for(auto ofs : m_offsets)
			write_offset(ofs + 1);

		// write the last one
		write_offset(m_data.size() + 1);
		buf.append(m_data.span());
	}




	// this is specialised for DICTs
	static void write_operand(const Operand& op, zst::byte_buffer& buf, bool force_4byte = false)
	{
		if(op.type == Operand::TYPE_INTEGER)
		{
			auto foo = op._integer;
			if(!force_4byte && -107 <= foo && foo <= 107)
			{
				buf.append(static_cast<uint8_t>(foo + 139));
			}
			else if(!force_4byte && 108 <= foo && foo <= 1131)
			{
				buf.append(static_cast<uint8_t>((foo / 256) + 247));
				buf.append(static_cast<uint8_t>((foo % 256) - 108));
			}
			else if(!force_4byte && -1131 <= foo && foo <= -108)
			{
				buf.append(static_cast<uint8_t>((-foo / 256) + 251));
				buf.append(static_cast<uint8_t>((-foo % 256) - 108));
			}
			else if(!force_4byte && -32768 <= foo && foo <= 32767)
			{
				buf.append(28);
				buf.append_bytes(util::convertBEU16(foo));
			}
			else
			{
				buf.append(29);
				buf.append_bytes(util::convertBEU32(foo));
			}
		}
		else if(op.type == Operand::TYPE_DECIMAL)
		{
			buf.append(30);

			uint8_t byte = 0;
			size_t nibbles = 0;

			auto add_nibble = [&](uint8_t nib) {
				byte = (byte << 4) | (nib & 0xF);
				if((++nibbles % 2) == 0)
					buf.append(byte), byte = 0;
			};

			// let's just do it the 3head way.
			auto tmp = zpr::sprint("{}", op._decimal);
			for(size_t i = 0; i < tmp.size(); i++)
			{
				auto c = tmp[i];

				if(c == '-')
				{
					add_nibble(0xE);
				}
				else if(c == '.')
				{
					add_nibble(0xA);
				}
				else if('0' <= c && c <= '9')
				{
					add_nibble(c - '0');
				}
				else if(c == 'e')
				{
					assert(i + 1 < tmp.size());
					if(tmp[i + 1] == '-')
						add_nibble(0xC);
					else
						add_nibble(0xB);
				}
			}

			add_nibble(0xF);
			if(nibbles % 2 != 0)
				add_nibble(0xF);
		}
		else
		{
			// note: TYPE_FIXED can't appear in DICT entries
			sap::error("font/cff", "invalid operand type '{}'", op.type);
		}
	}

	static size_t get_operand_size(const Operand& op, bool force_4byte = false)
	{
		if(op.type == Operand::TYPE_INTEGER)
		{
			auto foo = op._integer;
			if(!force_4byte && -107 <= foo && foo <= 107)
				return 1;
			else if(!force_4byte && 108 <= foo && foo <= 1131)
				return 2;
			else if(!force_4byte && -1131 <= foo && foo <= -108)
				return 2;
			else if(!force_4byte && -32768 <= foo && foo <= 32767)
				return 3;
			else
				return 5;
		}
		else if(op.type == Operand::TYPE_DECIMAL)
		{
			// let's just do it the 3head way.
			auto tmp = zpr::sprint("{}", op._decimal).size();
			return 2 + (tmp / 2);
		}
		return 0;
	}


	DictBuilder::DictBuilder()
	{
	}
	DictBuilder::DictBuilder(const Dictionary& from_dict)
	{
		m_values.insert(from_dict.values.begin(), from_dict.values.end());
	}

	DictBuilder& DictBuilder::set(DictKey key, std::vector<Operand> values)
	{
		m_values[key] = std::move(values);
		return *this;
	}

	DictBuilder& DictBuilder::setInteger(DictKey key, int32_t value)
	{
		m_values[key] = { Operand().integer(value) };
		return *this;
	}

	DictBuilder& DictBuilder::setStringId(DictKey key, uint16_t value)
	{
		m_values[key] = { Operand().string_id(value) };
		return *this;
	}

	DictBuilder& DictBuilder::setIntegerPair(DictKey key, int32_t a, int32_t b)
	{
		m_values[key] = { Operand().integer(a), Operand().integer(b) };
		return *this;
	}

	DictBuilder& DictBuilder::erase(DictKey key)
	{
		m_values.erase(key);
		return *this;
	}

	zst::byte_buffer DictBuilder::serialise()
	{
		zst::byte_buffer buf {};
		this->writeInto(buf);

		return buf;
	}

	static constexpr DictKey g_AbsoluteOffsetKeys[] = { DictKey::charset, DictKey::Encoding, DictKey::CharStrings,
		DictKey::Private, DictKey::FDArray, DictKey::FDSelect, DictKey::Subrs };

	static bool is_absolute_offset_key(DictKey key)
	{
		for(auto k : g_AbsoluteOffsetKeys)
		{
			if(k == key)
				return true;
		}
		return false;
	}

	size_t DictBuilder::computeSize()
	{
		size_t total_size = 0;
		for(auto& [key, values] : m_values)
		{
			if(auto def = getDefaultValueForDictKey(key); def.has_value() && def == values)
				continue;

			if(static_cast<uint16_t>(key) >= 0x0C00)
				total_size += 2;
			else
				total_size += 1;

			for(auto& val : values)
				total_size += get_operand_size(val, /* force_4byte: */ is_absolute_offset_key(key));
		}
		return total_size;
	}

	void DictBuilder::writeInto(zst::byte_buffer& buf)
	{
		auto write_key_value = [&](DictKey key) {
			if(auto it = m_values.find(key); it != m_values.end())
			{
				if(auto def = getDefaultValueForDictKey(key); def.has_value() && def == it->second)
					return;

				for(auto& value : it->second)
					write_operand(value, buf, /* force_4byte: */ is_absolute_offset_key(key));

				auto kv = static_cast<uint16_t>(key);
				if(kv >= 0x0C00)
					buf.append(12), kv &= 0xFF;

				buf.append(kv);
			}
		};

		// write the ROS or SyntheticBase keys first, if they exist. (write_key_value does nothing
		// if the key does not exist, so this is fine)
		write_key_value(DictKey::ROS);
		write_key_value(DictKey::SyntheticBase);

		// then, for all keys (skipping ROS or SyntheticBase), just write them.
		for(auto& [key, _] : m_values)
		{
			if(key != DictKey::ROS && key != DictKey::SyntheticBase)
				write_key_value(key);
		}
	}
}
