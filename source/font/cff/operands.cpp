// operands.cpp
// Copyright (c) 2022, zhiayang
// SPDX-License-Identifier: Apache-2.0

#include <optional>

#include "error.h"
#include "font/cff.h"
#include "font/font.h"

namespace font::cff
{
	static std::optional<Operand> readNumber(zst::byte_span& buf, bool is_dict)
	{
		if(buf.empty())
			return {};

		auto b0 = buf[0];
		if(32 <= b0 && b0 <= 246) // 0x20 - 0xf6
		{
			buf.remove_prefix(1);
			return Operand().integer(b0 - 139);
		}
		else if(247 <= b0 && b0 <= 250) // 0xf7 - 0xfa
		{
			auto b1 = buf[1];
			buf.remove_prefix(2);
			return Operand().integer((b0 - 247) * 256 + b1 + 108);
		}
		else if(251 <= b0 && b0 <= 254) // 0xfb - 0xfe
		{
			auto b1 = buf[1];
			buf.remove_prefix(2);
			return Operand().integer(-(b0 - 251) * 256 - b1 - 108);
		}
		else if(b0 == 28) // 0x1c
		{
			// easy (big endian)
			buf.remove_prefix(1);
			return Operand().integer((int16_t) consume_u16(buf));
		}
		else if(is_dict && b0 == 29) // 0x1d
		{
			buf.remove_prefix(1);
			return Operand().integer((int32_t) consume_u32(buf));
		}
		else if(!is_dict && b0 == 255) // 0x1e
		{
			buf.remove_prefix(1);
			return Operand().fixed((int32_t) consume_u32(buf));
		}

		return {};
	}

	std::optional<Operand> readNumberFromCharString(zst::byte_span& buf)
	{
		return readNumber(buf, /* is_dict: */ false);
	}

	std::optional<Operand> readNumberFromDICT(zst::byte_span& buf)
	{
		if(auto ret = readNumber(buf, /* is_dict: */ true); ret.has_value())
		{
			return ret;
		}
		else if(!buf.empty() && buf[0] == 30)
		{
			// this is the real number representation. It's kinda scuffed, though. Basically,
			// it's BCD but for floating point.
			buf.remove_prefix(1);

			double value = 0;
			double decimal_place = 0;
			int exponent = 0;

			bool negative = false;
			bool exponentiating = false;
			bool negative_exponent = false;

			while(true)
			{
				if(buf.empty())
					sap::error("font/cff", "malformed DICT data (unterminated real number)");

				auto x = buf[0];
				buf.remove_prefix(1);

				auto process_nibble = [&](uint8_t nibble) -> bool {
					if(0 <= nibble && nibble <= 9)
					{
						if(exponentiating)
						{
							exponent = 10 * exponent + nibble;
						}
						else if(decimal_place != 0)
						{
							value += nibble * decimal_place;
							decimal_place /= 10.0;
						}
						else
						{
							value = 10 * value + nibble;
						}
					}
					else if(nibble == 10)
					{
						decimal_place = 0.1;
					}
					else if(nibble == 11)
					{
						exponentiating = true;
					}
					else if(nibble == 12)
					{
						exponentiating = true;
						negative_exponent = true;
					}
					else if(nibble == 14)
					{
						negative = true;
					}
					else if(nibble == 15)
					{
						return true;
					}

					return false;
				};


				if(process_nibble((x >> 4) & 0xF))
					break;

				if(process_nibble(x & 0xF))
					break;
			}

			while(exponent != 0)
			{
				if(negative_exponent)
					value /= 10.0;
				else
					value *= 10.0;
				exponent--;
			}

			return Operand().decimal(negative ? -value : value);
		}
		else
		{
			return {};
		}
	}


	std::vector<Operand> readOperandsFromDICT(zst::byte_span& buf)
	{
		std::vector<Operand> operands {};

		while(buf.size() > 0)
		{
			if(buf[0] < 28)
				break;

			auto op = readNumberFromDICT(buf);
			if(op.has_value())
				operands.push_back(std::move(*op));
			else
				break;
		}

		return operands;
	}

	std::vector<Operand> readOperandsFromCharString(zst::byte_span& buf)
	{
		std::vector<Operand> operands {};

		while(buf.size() > 0)
		{
			if(buf[0] < 28)
				break;

			auto op = readNumberFromCharString(buf);
			if(op.has_value())
				operands.push_back(std::move(*op));
			else
				break;
		}

		return operands;
	}
}
