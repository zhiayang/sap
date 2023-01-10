// image.cpp
// Copyright (c) 2022, zhiayang
// SPDX-License-Identifier: Apache-2.0

#include "pdf/object.h"
#include "pdf/xobject.h"

namespace pdf
{
	XObject::XObject(File* doc, const Name& subtype)
	{
		m_contents = Stream::create();
	}

	Image::Image(File* doc, PdfScalar width, PdfScalar height, zst::byte_span image) : XObject(doc, names::Image)
	{
	}
}
