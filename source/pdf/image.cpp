// image.cpp
// Copyright (c) 2022, zhiayang
// SPDX-License-Identifier: Apache-2.0

#include "pdf/object.h"
#include "pdf/xobject.h"

namespace pdf
{
	Image::Image(PdfScalar width, PdfScalar height, zst::byte_span image) : XObject(names::Image)
	{
	}
}
