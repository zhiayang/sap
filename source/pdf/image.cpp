// image.cpp
// Copyright (c) 2022, zhiayang
// SPDX-License-Identifier: Apache-2.0

#include "pdf/object.h"
#include "pdf/xobject.h"

namespace pdf
{
	Image::Image(sap::ImageBitmap image_data, pdf::Size2d display_size, pdf::Position2d display_position)
	    : XObject(names::Image)
	    , m_image(std::move(image_data))
	    , m_display_size(display_size)
	    , m_display_position(display_position)
	{
		auto dict = m_stream->dictionary();
		m_stream->setCompressed(true);

		dict->add(names::Subtype, names::Image.ptr());
		dict->add(names::Width, Integer::create(checked_cast<int64_t>(m_image.pixel_width)));
		dict->add(names::Height, Integer::create(checked_cast<int64_t>(m_image.pixel_height)));

		auto one = Decimal::create(1.0);
		auto zero = Decimal::create(0.0);

		dict->add(names::ColorSpace, names::DeviceRGB.ptr());
		dict->add(names::BitsPerComponent, Integer::create(8));
		dict->add(names::Decode, Array::create(zero, one, zero, one, zero, one));

		m_stream->append(m_image.rgb);

		if(m_image.haveAlpha())
		{
			// create the "soft mask"
			m_alpha_channel = Stream::create();
			m_alpha_channel->setCompressed(true);

			m_alpha_channel->append(m_image.alpha);

			auto tmp = m_alpha_channel->dictionary();

			tmp->add(names::Type, names::XObject.ptr());
			tmp->add(names::Subtype, names::Image.ptr());
			tmp->add(names::Width, Integer::create(checked_cast<int64_t>(m_image.pixel_width)));
			tmp->add(names::Height, Integer::create(checked_cast<int64_t>(m_image.pixel_height)));
			tmp->add(names::ColorSpace, names::DeviceGray.ptr());
			tmp->add(names::BitsPerComponent, Integer::create(8));
			tmp->add(names::Decode, Array::create(zero, one));

			dict->add(names::SMask, IndirectRef::create(m_alpha_channel));
		}
	}

	void Image::writePdfCommands(Stream* stream) const
	{
		auto buf = zst::buffer<char>();
		auto appender = [&buf](const char* c, size_t n) { buf.append(c, n); };

		zpr::cprint(appender,
		    "q\n"                           // save state
		    "1 0 0 1 {} {} cm\n"            // move to the position
		    "{} 0 0 {} 0 -{} cm\n"          // scale and move so we draw at the right place for y-down
		    "/DeviceRGB CS\n"               // set colour space to RGB (FIXME: support cmyk/greyscale images)
		    "/{} Do\n"                      // draw the image (xobject)
		    "Q\n",                          // restore state
		    m_display_position.x().value(), //
		    m_display_position.y().value(), //
		    m_display_size.x().value(),     //
		    m_display_size.y().value(),     //
		    m_display_size.y().value(),     //
		    this->resourceName());

		stream->append(buf.bytes().data(), buf.size());
	}
}
