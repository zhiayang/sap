// image.cpp
// Copyright (c) 2022, zhiayang
// SPDX-License-Identifier: Apache-2.0

#include "pdf/object.h"
#include "pdf/xobject.h"

namespace pdf
{
	Image::Image(Data image_data, PdfScalar display_width, PdfScalar display_height)
	    : XObject(names::Image)
	    , m_image_data(std::move(image_data))
	    , m_width(display_width)
	    , m_height(display_height)
	{
		auto dict = m_stream->dictionary();

		dict->add(names::Subtype, names::Image.ptr());
		dict->add(names::Width, Integer::create(checked_cast<int64_t>(m_image_data.width)));
		dict->add(names::Height, Integer::create(checked_cast<int64_t>(m_image_data.height)));

		auto one = Decimal::create(1.0);
		auto zero = Decimal::create(0.0);

		dict->add(names::ColorSpace, names::DeviceRGB.ptr());
		dict->add(names::BitsPerComponent, Integer::create(8));
		dict->add(names::Decode, Array::create(zero, one, zero, one, zero, one));

		m_stream->append(m_image_data.bytes.data(), m_image_data.bytes.size());
	}

	void Image::writePdfCommands(Stream* stream) const
	{
		auto buf = zst::buffer<char>();
		auto appender = [&buf](const char* c, size_t n) {
			buf.append(c, n);
		};

		zpr::cprint(appender,
		    "q\n"                // save state
		    "{} 0 0 {} 0 0 cm\n" // scale by width x height
		    "/{} Do\n"           // draw the image (xobject)
		    "Q\n",               // restore state
		    m_width.value(), m_height.value(), m_resource_name);

		stream->append(buf.bytes().data(), buf.size());
	}
}
