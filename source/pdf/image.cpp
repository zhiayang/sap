// image.cpp
// Copyright (c) 2022, zhiayang
// SPDX-License-Identifier: Apache-2.0

#include "pdf/object.h"
#include "pdf/xobject.h"

namespace pdf
{
	Image::Image(PdfScalar width, PdfScalar height, zst::byte_span image) : XObject(names::Image)
	{
		auto dict = m_stream->dictionary();
		dict->add(names::Width, Decimal::create(width.value()));
		dict->add(names::Height, Decimal::create(height.value()));
	}

	void Image::writePdfCommands(Stream* stream) const
	{
		auto str_buf = zst::buffer<char>();
		auto appender = [&str_buf](const char* c, size_t n) {
			str_buf.append(c, n);
		};

		// return zpr::sprint("/{} Do\n", m_resource_name);
	}
}
