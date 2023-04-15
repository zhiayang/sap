// linebreak.cpp
// Copyright (c) 2022, zhiayang
// SPDX-License-Identifier: Apache-2.0

#include <utf8proc/utf8proc.h>

#include "misc/dijkstra.h" // for dijkstra_shortest_path

#include "sap/style.h" // for Style
#include "sap/units.h" // for Length

#include "layout/base.h"      // for RectPageLayout, Cursor
#include "layout/line.h"      // for Line, breakLines
#include "layout/word.h"      // for Separator, Word
#include "layout/linebreak.h" //

namespace sap::layout::linebreak
{
	using InlineObjPtr = zst::SharedPtr<tree::InlineObject>;
	using InlineObjVec = std::vector<InlineObjPtr>;
	using VecIter = InlineObjVec::const_iterator;

	struct LineBreakNode
	{
		const InlineObjVec* m_contents;
		const Style* m_parent_style;

		// TODO: we should get the preferred line length from the layout and not fix it here
		Length m_preferred_line_length;
		VecIter m_broken_until;
		VecIter m_end;

		std::optional<BrokenLine> m_line;

		using Distance = double;

		static LineBreakNode make_end(VecIter end)
		{
			return LineBreakNode {
				.m_contents = nullptr,
				.m_parent_style = nullptr,
				.m_preferred_line_length = 0,
				.m_broken_until = end,
				.m_end = end,
				.m_line = std::nullopt,
			};
		}

		LineBreakNode make_neighbour(VecIter neighbour_broken_until, BrokenLine neighbour_line) const
		{
			return LineBreakNode {
				.m_contents = m_contents,
				.m_parent_style = m_parent_style,
				.m_preferred_line_length = m_preferred_line_length,
				.m_broken_until = neighbour_broken_until,
				.m_end = m_end,
				.m_line = std::move(neighbour_line),
			};
		}

		std::vector<std::pair<LineBreakNode, Distance>> neighbours()
		{
			auto neighbour_broken_until = m_broken_until;
			auto neighbour_line = BrokenLine(m_parent_style);
			std::vector<std::pair<LineBreakNode, Distance>> ret;

			while(true)
			{
				if(neighbour_broken_until == m_end)
				{
					// last line cost calculation has 0 cost
					Distance cost = 0;
					ret.emplace_back(this->make_neighbour(neighbour_broken_until, std::move(neighbour_line)), cost);

					return ret;
				}

				auto& wordorsep = *neighbour_broken_until++;
				neighbour_line.add(wordorsep.get());

				// perform micro-typography
				{
					auto first_it = neighbour_broken_until - checked_cast<ptrdiff_t>(neighbour_line.numParts());

					auto& first_item = *first_it;
					if(auto txt = first_item->castToText())
					{
						assert(not txt->contents().empty());
						auto ch0 = txt->contents()[0];

						if(ch0 == U'\'' || ch0 == U'"'
						    || util::is_one_of(utf8proc_category((int32_t) ch0),
						        UTF8PROC_CATEGORY_PS, //
						        UTF8PROC_CATEGORY_PF, //
						        UTF8PROC_CATEGORY_PI))
						{
							const auto adjust = 2.0_mm;
							neighbour_line.setLeftProtrusion(adjust);
						}
					}
				}









				auto neighbour_width = neighbour_line.width();

				if(neighbour_width >= m_preferred_line_length && ret.empty())
				{
					sap::warn("line breaker", "ovErFUll \\hBOx, badNesS 10001!100!");
					ret.emplace_back(this->make_neighbour(neighbour_broken_until, neighbour_line), 10000);
					return ret;
				}
				// don't allow shrinking more than 3%, otherwise it looks kinda bad
				else if(neighbour_width / m_preferred_line_length > 1.5)
				{
					return ret;
				}


				double cost_mult = 1;
				auto space_diff = (m_preferred_line_length - neighbour_width);
				if(space_diff < 0)
				{
					space_diff = space_diff.abs();
					cost_mult = 1.2;
				}

				if(auto sep = wordorsep->castToSeparator())
				{
					Distance cost = 0;
					// If there are no spaces we pretend there's half a space,
					// so the cost is twice as high as having 1 space

					// note: this is "extra mm per space character"
					if(sep->isHyphenationPoint() || sep->isExplicitBreakPoint())
					{
						auto tmp = std::max((double) neighbour_line.numSpaces() - 1, 0.5);
						auto avg_space_width = neighbour_line.totalSpaceWidth().mm() / tmp;

						cost += 0.25 * (1 + sep->hyphenationCost()) * (avg_space_width * avg_space_width);

						double extra_space_size = space_diff.mm() / tmp;
						cost += 3 * extra_space_size * extra_space_size;

						// TODO: adjust hyphenation here.
					}
					else if(sep->hasWhitespace())
					{
						auto tmp = std::max((double) neighbour_line.numSpaces(), 0.5);
						double extra_space_size = space_diff.mm() / tmp;
						cost += 2.99 * extra_space_size * extra_space_size;

						// see if we can adjust the right side of the line.
						// note that *neighbour_broken_until is now *TWO* guys
						// ahead of the last non-separator in the line.

						auto last_it = neighbour_broken_until - 2;
						auto& last_item = *last_it;

						if(auto txt = last_item->castToText())
						{
							assert(not txt->contents().empty());
							auto ch = txt->contents().back();

							// note: using this list
							// https://developer.mozilla.org/en-US/docs/Web/CSS/hanging-punctuation
							if(util::is_one_of(ch, U'\u002C', U'\u002E', U'\u060C', U'\u06D4', U'\u3001', U'\u3002',
							       U'\uFF0C', U'\uFF0E', U'\uFE50', U'\uFE51', U'\uFE52', U'\uFF61', U'\uFF64')
							    || util::is_one_of(ch, U'\'', U'\"')
							    || util::is_one_of(utf8proc_category((int32_t) ch), UTF8PROC_CATEGORY_PE,
							        UTF8PROC_CATEGORY_PF, UTF8PROC_CATEGORY_PI))
							{
								const auto adjust = 2.0_mm;
								neighbour_line.setRightProtrusion(adjust);
							}
						}
					}
					else
					{
						sap::internal_error("handle other sep");
					}

					ret.emplace_back(this->make_neighbour(neighbour_broken_until, neighbour_line), cost * cost_mult);
					neighbour_line.resetAdjustments();
				}
			}
		}

		size_t hash() const { return static_cast<size_t>(m_broken_until - m_contents->begin()); }
		bool operator==(const LineBreakNode& other) const { return m_broken_until == other.m_broken_until; }
	};



	std::vector<BrokenLine>
	breakLines(const Style* parent_style, const InlineObjVec& contents, Length preferred_line_length)
	{
		auto path = util::dijkstra_shortest_path(
		    LineBreakNode {
		        .m_contents = &contents,
		        .m_parent_style = parent_style,
		        .m_preferred_line_length = preferred_line_length,
		        .m_broken_until = contents.begin(),
		        .m_end = contents.end(),
		        .m_line = BrokenLine(parent_style),
		    },
		    LineBreakNode::make_end(contents.end()));

		std::vector<BrokenLine> ret;
		ret.reserve(path.size());

		for(auto& node : path)
			ret.push_back(std::move(*node.m_line));

		return ret;
	}
}
