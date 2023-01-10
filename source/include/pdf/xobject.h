// xobject.h
// Copyright (c) 2022, zhiayang
// SPDX-License-Identifier: Apache-2.0

#pragma once

#include "pdf/units.h"

namespace pdf
{
	struct File;
	struct Name;
	struct Stream;

	struct XObject
	{
		XObject(File* doc, const Name& subtype);

	private:
		Stream* m_contents;
	};


	struct Image : XObject
	{
		Image(File* doc, PdfScalar width, PdfScalar height, zst::byte_span image);

	private:
	};
}
