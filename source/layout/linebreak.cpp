// linebreak.cpp
// Copyright (c) 2022, yuki / zhiayang
// SPDX-License-Identifier: Apache-2.0

#include <utf8proc/utf8proc.h>

#include "misc/dijkstra.h" // for dijkstra_shortest_path

#include "sap/style.h" // for Style
#include "sap/units.h" // for Length

#include "interp/interp.h"

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
		interp::Interpreter* m_interp;
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
				.m_interp = nullptr,
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
				.m_interp = m_interp,
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
			auto neighbour_line = BrokenLine(m_parent_style ? *m_parent_style : Style::empty());
			std::vector<std::pair<LineBreakNode, Distance>> ret;

			// adjust left-side protrusion once per line
			Length left_protrusion = 0;
			{
				auto first_it = neighbour_broken_until - checked_cast<ptrdiff_t>(neighbour_line.numParts());

				auto& first_item = *first_it;
				if(auto txt = first_item->castToText())
				{
					assert(not txt->contents().empty());
					auto sv = zst::wstr_view(txt->contents());

					auto sty = m_parent_style->extendWith(txt->style());
					if(auto p = m_interp->getMicrotypeProtrusionFor(sv[0], sty))
					{
						// protrusion is defined as proportion of the glyph width.
						// so, we must calculate that.
						auto glyph_width = sty.font()->getWordSize(sv.take(1), sty.font_size().into()).x();
						left_protrusion = (p->left * glyph_width).into();
					}
				}
			}


			while(true)
			{
				neighbour_line.setLeftProtrusion(left_protrusion);

				if(neighbour_broken_until == m_end)
				{
					// completely arbitrary. *BUT* the key goal is to have exponentially
					// increasing costs for having a very very short last line.
					auto ratio = neighbour_line.width() / m_preferred_line_length;

					Distance cost = 2.0 / std::pow(ratio + 1, 1.3);
					ret.emplace_back(this->make_neighbour(neighbour_broken_until, std::move(neighbour_line)), cost);

					return ret;
				}

				auto& wordorsep = *neighbour_broken_until++;
				neighbour_line.add(wordorsep.get());

				auto neighbour_width = neighbour_line.width();

				if(neighbour_width >= m_preferred_line_length && ret.empty())
				{
					sap::warn("line breaker", "ovErFUll \\hBOx, badNesS 10001!100!");
					ret.emplace_back(this->make_neighbour(neighbour_broken_until, neighbour_line), 10000);
					return ret;
				}
				// don't allow shrinking more than 10%, otherwise it looks kinda bad
				else if(neighbour_width / m_preferred_line_length > 1.03)
				{
					return ret;
				}


				double cost_mult = 1;
				auto space_diff = (m_preferred_line_length - neighbour_width);
				if(space_diff < 0)
				{
					space_diff = space_diff.abs();
					cost_mult = 1.05;
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

						double extra_space_size = space_diff.mm() / tmp;
						cost += 3 * extra_space_size * extra_space_size;
						cost += 0.2 * (1 + sep->hyphenationCost()) * (avg_space_width * avg_space_width);

						// add a large cost for doing shit like e-ducational
						auto last_it = neighbour_broken_until - 2;
						auto& last_item = *last_it;
						if(auto txt = last_item->castToText())
						{
							assert(not txt->contents().empty());
							auto frag = neighbour_line.lastWordFragment();
							cost += pow(2, 1.0 / static_cast<double>(frag.size()));
						}

						if(sep->isHyphenationPoint())
						{
							assert(not sep->endOfLine().empty());

							auto sty = m_parent_style->extendWith(sep->style());
							if(auto p = m_interp->getMicrotypeProtrusionFor(sep->endOfLine()[0], sty))
							{
								auto w = sty.font()->getWordSize(sep->endOfLine(), sty.font_size().into()).x();
								neighbour_line.setRightProtrusion((p->right * w).into());
							}
						}
						else
						{
							assert(sep->isExplicitBreakPoint());
							if(auto txt = last_item->castToText())
							{
								assert(not txt->contents().empty());
								auto frag = neighbour_line.lastWordFragment();
								auto sty = m_parent_style->extendWith(txt->style());

								if(auto p = m_interp->getMicrotypeProtrusionFor(frag.back(), sty))
								{
									auto w = sty.font()->getWordSize(frag.take_last(1), sty.font_size().into()).x();
									neighbour_line.setRightProtrusion((p->right * w).into());
								}
							}
						}
					}
					else if(sep->hasWhitespace())
					{
						auto tmp = std::max((double) neighbour_line.numSpaces(), 0.5);
						double extra_space_size = space_diff.mm() / tmp;
						cost += std::pow(extra_space_size, 3);

						// see if we can adjust the right side of the line.
						// note that *neighbour_broken_until is now *TWO* guys
						// ahead of the last non-separator in the line.

						auto last_it = neighbour_broken_until - 2;
						auto& last_item = *last_it;

						if(auto txt = last_item->castToText())
						{
							assert(not txt->contents().empty());
							auto sv = zst::wstr_view(txt->contents());

							auto sty = m_parent_style->extendWith(txt->style());
							if(auto p = m_interp->getMicrotypeProtrusionFor(sv.back(), sty))
							{
								auto w = sty.font()->getWordSize(sv.take_last(1), sty.font_size().into()).x();
								neighbour_line.setRightProtrusion((p->right * w).into());
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



	std::vector<BrokenLine> breakLines(interp::Interpreter* cs,
	    const Style& parent_style,
	    const InlineObjVec& contents,
	    Length preferred_line_length)
	{
		auto path = util::dijkstra_shortest_path(
		    LineBreakNode {
		        .m_interp = cs,
		        .m_contents = &contents,
		        .m_parent_style = &parent_style,
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
