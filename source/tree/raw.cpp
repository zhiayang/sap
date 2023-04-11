// raw.cpp
// Copyright (c) 2022, zhiayang
// SPDX-License-Identifier: Apache-2.0

#include <limits>

#include "tree/raw.h"
#include "tree/paragraph.h"

#include "layout/line.h"
#include "layout/container.h"

#include "interp/interp.h"

namespace sap::tree
{
	RawBlock::RawBlock(zst::wstr_view str) : m_text(str.str())
	{
		size_t smallest_leading_spaces = std::numeric_limits<size_t>::max();
		std::vector<std::u32string> lines {};

		while(not str.empty())
		{
			// if the string is left with only spaces, stop.
			if(str.sv().find_first_not_of(U" \t") == std::string::npos)
				break;

			auto line = str.take_until(U'\n');
			while(line.ends_with(' ') || line.ends_with('\t') || line.ends_with('\n') || line.ends_with('\r'))
				line.remove_suffix(1);

			str.remove_prefix(line.size() + 1);

			// replace tabs with 4 spaces
			// TODO: parse and use the attributes
			std::u32string replaced;
			replaced.reserve(line.size());

			for(size_t i = 0; (i = line.find('\t')) != std::string::npos; line.remove_prefix(i + 1))
			{
				replaced.append(line.take(i).str());
				replaced.append(U"    ");
			}

			replaced.append(line.str());
			auto x = replaced.find_first_not_of(U' ');

			if(x != std::string::npos)
				smallest_leading_spaces = std::min(smallest_leading_spaces, x);

			lines.push_back(std::move(replaced));
		}

		// TODO: control this with an attribute
		// gobble leading whitespace.
		for(size_t i = 0; i < lines.size(); i++)
		{
			lines[i].erase(0, smallest_leading_spaces);
			m_lines.emplace_back(new Text(std::move(lines[i])));
		}
	}

	ErrorOr<void> RawBlock::evaluateScripts(interp::Interpreter* cs) const
	{
		// no scripts to evaluate
		return Ok();
	}

	ErrorOr<LayoutResult> RawBlock::create_layout_object_impl(interp::Interpreter* cs, //
		const Style* parent_style,
		Size2d available_space) const
	{
		// don't care about the space either
		(void) available_space;

		auto _ = cs->evaluator().pushBlockContext(this);
		auto style = m_style->useDefaultsFrom(parent_style)->useDefaultsFrom(cs->evaluator().currentStyle());

		std::vector<std::unique_ptr<layout::LayoutObject>> lines {};

		Size2d size { 0, 0 };

		bool is_first = true;
		for(auto& text_line : m_lines)
		{
			auto span = std::span(&text_line, 1);

			auto metrics = layout::computeLineMetrics(span, style);
			auto line = layout::Line::fromInlineObjects(cs, style, span, metrics, available_space,
				/* is_first: */ true, /* is_last: */ true);

			size.x() = std::max(size.x(), line->layoutSize().width);
			size.y() += line->layoutSize().total_height();

			is_first = false;
			lines.push_back(std::move(line));
		}

		auto vbox = std::make_unique<layout::Container>(style,
			LayoutSize {
				.width = size.x(),
				.ascent = 0,
				.descent = size.y(),
			},
			layout::Container::Direction::Vertical, //
			/* glue: */ false,                      //
			std::move(lines),                       //
			/* override obj spacing: */ Length(0));

		return Ok(LayoutResult::make(std::move(vbox)));
	}
}