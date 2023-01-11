// xobject.h
// Copyright (c) 2022, zhiayang
// SPDX-License-Identifier: Apache-2.0

#pragma once

#include "pdf/units.h"
#include "pdf/page_object.h"

namespace pdf
{
	struct File;
	struct Name;
	struct Stream;

	struct XObject : PageObject
	{
		XObject(const Name& subtype);

		virtual void addResources(const Page* page) const override;

		void serialise() const;
		const std::string& getResourceName() const;
		Stream* stream() const { return m_stream; }

	protected:
		Stream* m_stream;
		std::string m_resource_name;
		mutable bool m_did_serialise = false;
	};


	struct Image : XObject
	{
		struct Data
		{
			zst::byte_span bytes;
			size_t width;
			size_t height;
			size_t bits_per_pixel;
		};

		Image(Data image_data, PdfScalar display_width, PdfScalar display_height);
		virtual void writePdfCommands(Stream* stream) const override;

	private:
		Data m_image_data;
		PdfScalar m_width;
		PdfScalar m_height;
	};
}
