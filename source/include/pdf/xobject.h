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
		Image(PdfScalar width, PdfScalar height, zst::byte_span image);
		virtual void writePdfCommands(Stream* stream) const override;

	private:
	};
}
