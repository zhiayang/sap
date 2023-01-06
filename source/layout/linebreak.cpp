// linebreak.cpp
// Copyright (c) 2022, zhiayang
// SPDX-License-Identifier: Apache-2.0

#include <variant> // for holds_alternative, variant, visit

#include "dijkstra.h" // for dijkstra_shortest_path

#include "sap/style.h" // for Style
#include "sap/units.h" // for Length

#include "layout/base.h" // for RectPageLayout, Cursor
#include "layout/line.h" // for Line, breakLines
#include "layout/word.h" // for Separator, Word

namespace sap::layout
{
	using WordOrSep = std::variant<Word, Separator>;
	using WordVec = std::vector<WordOrSep>;
	using WordVecIter = WordVec::const_iterator;

	struct LineBreakNode
	{
		const WordVec* words;
		RectPageLayout* layout;
		const Style* parent_style;

		// TODO: we should get the preferred line length from the layout and not fix it here
		Length preferred_line_length;
		WordVecIter broken_until;
		WordVecIter end;
		Line line;

		using Distance = double;

		static LineBreakNode make_end(WordVecIter end)
		{
			return LineBreakNode {
				.words = nullptr,
				.layout = nullptr,
				.parent_style = nullptr,
				.preferred_line_length = 0,
				.broken_until = end,
				.end = end,
				.line = Line::invalid_line(),
			};
		}

		LineBreakNode make_neighbour(WordVecIter neighbour_broken_until, Line neighbour_line) const
		{
			return LineBreakNode {
				.words = words,
				.layout = layout,
				.parent_style = parent_style,
				.preferred_line_length = preferred_line_length,
				.broken_until = neighbour_broken_until,
				.end = end,
				.line = neighbour_line,
			};
		}

		std::vector<std::pair<LineBreakNode, Distance>> neighbours()
		{
			auto neighbour_broken_until = broken_until;
			auto neighbour_line = Line(layout, parent_style, line.lineCursor());
			std::vector<std::pair<LineBreakNode, Distance>> ret;

			while(true)
			{
				if(neighbour_broken_until == end)
				{
					// last line cost calculation has 0 cost
					Distance cost = 0;
					ret.emplace_back(this->make_neighbour(neighbour_broken_until, neighbour_line), cost);
					return ret;
				}


				// TODO: make it not ugly lol
				auto wordorsep = *neighbour_broken_until++;
				std::visit(
				    [&](auto w) {
					    neighbour_line.add(w);
				    },
				    wordorsep);

				// TODO: allow shrinking of spaces by allowing lines to go past the end of the preferred_line_length
				// by 10% of the space width * num_spaces
				if(neighbour_line.width() >= preferred_line_length)
				{
					if(ret.empty())
					{
						sap::warn("line breaker", "ovErFUll \\hBOx, badNesS 10001!100!");
						ret.emplace_back(this->make_neighbour(neighbour_broken_until, neighbour_line), 10000);
					}
					return ret;
				}

				if(auto sep = std::get_if<Separator>(&wordorsep); sep)
				{
					Distance cost = 0;

					if(sep->kind == tree::Separator::SPACE)
					{
						// If there are no spaces we pretend there's half a space,
						// so the cost is twice as high as having 1 space
						double extra_space_size = (preferred_line_length - line.width()).mm()
						                        / (neighbour_line.numSpaces() ? (double) neighbour_line.numSpaces() : 0.5);

						cost += extra_space_size * extra_space_size;
					}
					else if(sep->kind == tree::Separator::HYPHENATION_POINT)
					{
						cost += 0;
					}
					else
					{
						sap::internal_error("handle other sep");
					}

					ret.emplace_back(this->make_neighbour(neighbour_broken_until, neighbour_line), cost);
				}
			}
		}

		size_t hash() const { return static_cast<size_t>(broken_until - words->begin()); }
		bool operator==(const LineBreakNode& other) const { return broken_until == other.broken_until; }
	};

	std::vector<Line> breakLines(RectPageLayout* layout, Cursor cursor, const Style* parent_style, const WordVec& words,
	    Length preferred_line_length)
	{
		auto path = util::dijkstra_shortest_path(
		    LineBreakNode {
		        .words = &words,
		        .layout = layout,
		        .parent_style = parent_style,
		        .preferred_line_length = preferred_line_length,
		        .broken_until = words.begin(),
		        .end = words.end(),
		        .line = Line(layout, parent_style, cursor),
		    },
		    LineBreakNode::make_end(words.end()));

		std::vector<Line> ret;
		ret.reserve(path.size());

		for(auto& node : path)
			ret.push_back(std::move(node.line));

		return ret;
	}
}
