// path.cpp
// Copyright (c) 2023, yuki
// SPDX-License-Identifier: Apache-2.0

#include "pdf/page.h"
#include "pdf/path.h"
#include "pdf/object.h"
#include "pdf/resource.h"

namespace pdf
{
	Path::Path(Position2d display_position, Size2d display_size)
	    : m_display_position(display_position), m_display_size(display_size)
	{
	}

	void Path::addResources(const Page* page) const
	{
		auto rsrc = util::make<ExtGraphicsState>();
		page->addResource(rsrc);
	}

	void Path::addSegment(Segment segment)
	{
		m_segments.push_back(std::move(segment));
	}

	static void style_to_pdf_commands(auto& a, const Path::PaintStyle& style)
	{
		auto add_colour = [&](const sap::Colour& colour, bool stroking) {
			if(colour.isRGB())
			{
				auto rgb = colour.rgb();
				zpr::cprint(a, " {} {} {} {}", rgb.r, rgb.g, rgb.b, stroking ? "RG" : "rg");
			}
			else if(colour.isCMYK())
			{
				auto cmyk = colour.cmyk();
				zpr::cprint(a, " {} {} {} {} {}", cmyk.c, cmyk.m, cmyk.y, cmyk.k, stroking ? "K" : "k");
			}
			else
			{
				sap::internal_error("invalid colour type");
			}
		};

		zpr::cprint(a, " {} w", style.line_width.value());
		zpr::cprint(a, " {} J", static_cast<int>(style.cap_style));
		zpr::cprint(a, " {} j", static_cast<int>(style.join_style));
		zpr::cprint(a, " {} M", style.miter_limit);

		add_colour(style.stroke_colour, /* stroking: */ true);
		add_colour(style.fill_colour, /* stroking: */ false);
	}

	void Path::writePdfCommands(Stream* stream) const
	{
		auto str_buf = zst::buffer<char>();
		auto appender = [&str_buf](const char* c, size_t n) { str_buf.append(c, n); };

		zpr::cprint(appender,   //
		    "q\n"               //
		    "/ExtGState13 gs\n" //
		    "1 0 0 1 0 0 cm\n"  //
		);

		for(auto& seg : m_segments)
		{
			if(auto ps = std::get_if<PaintStyle>(&seg); ps)
			{
				style_to_pdf_commands(appender, *ps);
			}
			else if(auto m = std::get_if<MoveTo>(&seg); m)
			{
				zpr::cprint(appender, " {} {} m", m->pos.x(), m->pos.y());
			}
			else if(auto l = std::get_if<LineTo>(&seg); l)
			{
				zpr::cprint(appender, " {} {} l", l->pos.x(), l->pos.y());
			}
			else if(auto c = std::get_if<CubicBezier>(&seg); c)
			{
				zpr::cprint(appender, " {} {} {} {} {} {} c", c->cp1.x(), c->cp1.y(), c->cp2.x(), c->cp2.y(),
				    c->end.x(), c->end.y());
			}
			else if(auto v = std::get_if<CubicBezierIC1>(&seg); v)
			{
				zpr::cprint(appender, " {} {} {} {} v", v->cp2.x(), v->cp2.y(), v->end.x(), v->end.y());
			}
			else if(auto y = std::get_if<CubicBezierIC2>(&seg); y)
			{
				zpr::cprint(appender, " {} {} {} {} y", y->cp1.x(), y->cp1.y(), y->end.x(), y->end.y());
			}
			else if(auto re = std::get_if<Rectangle>(&seg); re)
			{
				zpr::cprint(appender, " {} {} {} {} re", re->start.x(), re->start.y(), re->size.x(), re->size.y());
			}
			else if(auto h = std::get_if<ClosePath>(&seg); h)
			{
				zpr::cprint(appender, " h");
			}
			else
			{
				sap::internal_error("???");
			}
		}

		// stroke that shit
		zpr::cprint(appender,
		    " S\n"
		    "Q\n");

		stream->append(str_buf.bytes());
	}
}
