// charstring.cpp
// Copyright (c) 2022, zhiayang
// SPDX-License-Identifier: Apache-2.0

#include "error.h"
#include "font/cff.h"
#include "font/font.h"

namespace font::cff
{
	constexpr uint8_t CMD_HSTEM = 1;
	constexpr uint8_t CMD_VSTEM = 3;
	constexpr uint8_t CMD_VMOVETO = 4;
	constexpr uint8_t CMD_RLINETO = 5;
	constexpr uint8_t CMD_HLINETO = 6;
	constexpr uint8_t CMD_VLINETO = 7;
	constexpr uint8_t CMD_RRCURVETO = 8;
	constexpr uint8_t CMD_CALLSUBR = 10;
	constexpr uint8_t CMD_RETURN = 11;
	constexpr uint8_t CMD_ENDCHAR = 14;
	constexpr uint8_t CMD_HSTEMHM = 18;
	constexpr uint8_t CMD_HINTMASK = 19;
	constexpr uint8_t CMD_CNTRMASK = 20;
	constexpr uint8_t CMD_RMOVETO = 21;
	constexpr uint8_t CMD_HMOVETO = 22;
	constexpr uint8_t CMD_VSTEMHM = 23;
	constexpr uint8_t CMD_RCURVELINE = 24;
	constexpr uint8_t CMD_RLINECURVE = 25;
	constexpr uint8_t CMD_VVCURVETO = 26;
	constexpr uint8_t CMD_HHCURVETO = 27;
	constexpr uint8_t CMD_CALLGSUBR = 29;
	constexpr uint8_t CMD_VHCURVETO = 30;
	constexpr uint8_t CMD_HVCURVETO = 31;

	constexpr uint8_t CMD_ESCAPE = 12;
	constexpr uint8_t CMD_ESC_AND = 3;
	constexpr uint8_t CMD_ESC_OR = 4;
	constexpr uint8_t CMD_ESC_NOT = 5;
	constexpr uint8_t CMD_ESC_ABS = 9;
	constexpr uint8_t CMD_ESC_ADD = 10;
	constexpr uint8_t CMD_ESC_SUB = 11;
	constexpr uint8_t CMD_ESC_DIV = 12;
	constexpr uint8_t CMD_ESC_NEG = 14;
	constexpr uint8_t CMD_ESC_EQ = 15;
	constexpr uint8_t CMD_ESC_DROP = 18;
	constexpr uint8_t CMD_ESC_PUT = 20;
	constexpr uint8_t CMD_ESC_GET = 21;
	constexpr uint8_t CMD_ESC_IFELSE = 22;
	constexpr uint8_t CMD_ESC_RANDOM = 23;
	constexpr uint8_t CMD_ESC_MUL = 24;
	constexpr uint8_t CMD_ESC_SQRT = 26;
	constexpr uint8_t CMD_ESC_DUP = 27;
	constexpr uint8_t CMD_ESC_EXCH = 28;
	constexpr uint8_t CMD_ESC_INDEX = 29;
	constexpr uint8_t CMD_ESC_ROLL = 30;
	constexpr uint8_t CMD_ESC_HFLEX = 34;
	constexpr uint8_t CMD_ESC_FLEX = 35;
	constexpr uint8_t CMD_ESC_HFLEX1 = 36;
	constexpr uint8_t CMD_ESC_FLEX1 = 37;

	struct InterpState
	{
		std::vector<Operand> stack;

		int num_hstems = 0;
		int num_vstems = 0;
		bool in_header = false;

		inline void ensure(size_t n)
		{
			if(stack.size() < n)
				sap::error("font/cff", "stack underflow");
		}

		inline Operand pop()
		{
			ensure(1);

			auto ret = stack.back();
			stack.pop_back();
			return ret;
		}

		inline Operand peek()
		{
			ensure(1);
			return stack.back();
		}

		inline void push(Operand oper) { stack.push_back(std::move(oper)); }
	};

	static bool run_charstring(zst::byte_span instrs, std::vector<Subroutine>& global_subrs, std::vector<Subroutine>& local_subrs,
		InterpState& interp)
	{
		auto calculate_bias = [](const std::vector<Subroutine>& subrs) {
			if(subrs.size() < 1240)
				return 107;
			else if(subrs.size() < 33900)
				return 1131;
			else
				return 32768;
		};

		auto global_bias = calculate_bias(global_subrs);
		auto local_bias = calculate_bias(local_subrs);

		while(instrs.size() > 0)
		{
			auto x = instrs[0];
			if(x == 28 || (32 <= x && x <= 255))
			{
				interp.push(*readNumberFromCharString(instrs));
				continue;
			}

			bool clear_stack = true;
			instrs.remove_prefix(1);
			switch(x)
			{
				case CMD_HSTEM:
				case CMD_HSTEMHM:
					interp.num_hstems += interp.stack.size() / 2;
					break;

				case CMD_VSTEM:
				case CMD_VSTEMHM:
					interp.num_vstems += interp.stack.size() / 2;
					break;

				case CMD_HINTMASK:
				case CMD_CNTRMASK: {
					// for the first one, there is an implicit vstem, so add another hint:
					interp.num_vstems += (interp.stack.size() / 2);

					// calculate the required mask length.
					auto num_hints = interp.num_hstems + interp.num_vstems;

					auto mask_bytes = (num_hints + 7) / 8;
					interp.in_header = false;

					// skip the mask bytes (`x` was already dropped)
					instrs.remove_prefix(mask_bytes);
					break;
				}

				case CMD_RETURN:
					return false;

				case CMD_ENDCHAR:
					return true;

				case CMD_CALLSUBR:
				case CMD_CALLGSUBR: {

					interp.ensure(1);

					zst::byte_span subr_cs {};
					auto subr_num = interp.pop().integer();
					if(x == CMD_CALLSUBR)
					{
						subr_num += local_bias;
						if(subr_num >= (int32_t) local_subrs.size())
							sap::error("font/cff", "local subr {} out of bounds (max {})", subr_num, local_subrs.size());

						subr_cs = local_subrs[subr_num].charstring;
						local_subrs[subr_num].used = true;
					}
					else
					{
						subr_num += global_bias;
						if(subr_num >= (int32_t) global_subrs.size())
							sap::error("font/cff", "global subr {} out of bounds (max {})", subr_num, global_subrs.size());

						subr_cs = global_subrs[subr_num].charstring;
						global_subrs[subr_num].used = true;
					}

					auto finish = run_charstring(subr_cs, global_subrs, local_subrs, interp);
					if(finish)
						return true;

					clear_stack = false;
					break;
				}

				case CMD_RMOVETO:
					interp.ensure(2);
					interp.in_header = false;
					break;
				case CMD_HMOVETO:
					interp.ensure(1);
					interp.in_header = false;
					break;
				case CMD_VMOVETO:
					interp.ensure(1);
					interp.in_header = false;
					break;
				case CMD_RLINETO:
					interp.ensure(2);
					break;
				case CMD_HLINETO:
					interp.ensure(1);
					break;
				case CMD_VLINETO:
					interp.ensure(1);
					break;
				case CMD_VHCURVETO:
					interp.ensure(4);
					break;
				case CMD_HVCURVETO:
					interp.ensure(4);
					break;
				case CMD_RRCURVETO:
					interp.ensure(6);
					break;
				case CMD_RCURVELINE:
					interp.ensure(8);
					break;
				case CMD_RLINECURVE:
					interp.ensure(8);
					break;
				case CMD_VVCURVETO:
					interp.ensure(4);
					break;
				case CMD_HHCURVETO:
					interp.ensure(4);
					break;

				case CMD_ESCAPE: {
					auto x = instrs[0];
					instrs.remove_prefix(1);

					switch(x)
					{
						case CMD_ESC_HFLEX:
							interp.ensure(7);
							break;
						case CMD_ESC_HFLEX1:
							interp.ensure(9);
							break;

						case CMD_ESC_FLEX:
							interp.ensure(13);
							break;
						case CMD_ESC_FLEX1:
							interp.ensure(11);
							break;


						case CMD_ESC_ADD:
							interp.ensure(2);
							interp.pop();
							clear_stack = false;
							break;
						case CMD_ESC_SUB:
							interp.ensure(2);
							interp.pop();
							clear_stack = false;
							break;
						case CMD_ESC_DIV:
							interp.ensure(2);
							interp.pop();
							clear_stack = false;
							break;
						case CMD_ESC_MUL:
							interp.ensure(2);
							interp.pop();
							clear_stack = false;
							break;
						case CMD_ESC_NEG:
							interp.ensure(1);
							clear_stack = false;
							break;
						case CMD_ESC_ABS:
							interp.ensure(1);
							clear_stack = false;
							break;
						case CMD_ESC_SQRT:
							interp.ensure(1);
							clear_stack = false;
							break;
						case CMD_ESC_RANDOM:
							interp.push(Operand().integer(0));
							clear_stack = false;
							break;

						case CMD_ESC_INDEX: {
							interp.ensure(2);
							auto i = interp.pop().integer();
							if(i <= 0)
								interp.push(interp.peek());
							else
								interp.push(interp.stack[interp.stack.size() - i - 1]);

							clear_stack = false;
							break;
						}

						case CMD_ESC_ROLL: {
							// TODO: implement `roll`
							interp.ensure(2);
							clear_stack = false;
							break;
						}

						case CMD_ESC_EXCH: {
							interp.ensure(2);
							std::swap(interp.stack[interp.stack.size() - 1], interp.stack[interp.stack.size() - 2]);
							clear_stack = false;
							break;
						}

						case CMD_ESC_DROP:
							interp.ensure(1);
							interp.pop();
							clear_stack = false;
							break;
						case CMD_ESC_DUP:
							interp.ensure(1);
							interp.push(interp.peek());
							clear_stack = false;
							break;

						case CMD_ESC_PUT:
							interp.ensure(2);
							interp.pop();
							interp.pop();
							clear_stack = false;
							break;

						case CMD_ESC_GET:
							interp.ensure(1);
							clear_stack = false;
							break;

						case CMD_ESC_AND:
							interp.ensure(2);
							interp.pop();
							clear_stack = false;
							break;
						case CMD_ESC_OR:
							interp.ensure(2);
							interp.pop();
							clear_stack = false;
							break;
						case CMD_ESC_NOT:
							interp.ensure(1);
							clear_stack = false;
							break;
						case CMD_ESC_EQ:
							interp.ensure(2);
							interp.pop();
							clear_stack = false;
							break;
						case CMD_ESC_IFELSE:
							interp.ensure(4);
							interp.pop();
							interp.pop();
							interp.pop();
							clear_stack = false;

						default:
							sap::error("font/cff", "invalid opcode '0c {x}'", x);
							break;
					}
					break;
				}

				default:
					sap::error("font/cff", "invalid opcode '{x}'", x);
					break;
			}

			if(clear_stack)
				interp.stack.clear();
		}

		// finish by default
		return true;
	}

	void interpretCharStringAndMarkSubrs(zst::byte_span instrs, std::vector<Subroutine>& global_subrs,
		std::vector<Subroutine>& local_subrs)
	{
		InterpState interp {};
		run_charstring(instrs, global_subrs, local_subrs, interp);
	}
}
