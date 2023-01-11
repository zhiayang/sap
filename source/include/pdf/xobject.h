// xobject.h
// Copyright (c) 2022, zhiayang
// SPDX-License-Identifier: Apache-2.0

#pragma once

#include "pdf/units.h"
#include "pdf/resource.h"
#include "pdf/page_object.h"

namespace pdf
{
	struct File;
	struct Name;
	struct Stream;

	struct XObject : PageObject, Resource
	{
		XObject(const Name& subtype);

		virtual void addResources(const Page* page) const override;
		virtual Object* resourceObject() const override;
		virtual void serialise() const override;

		const std::string& getResourceName() const;
		Stream* stream() const { return m_stream; }

	protected:
		Stream* m_stream;
		mutable bool m_did_serialise = false;
	};


	struct Image : XObject
	{
		Image(sap::ImageBitmap image_data, Size2d display_size, Position2d display_position);
		virtual void writePdfCommands(Stream* stream) const override;

	private:
		sap::ImageBitmap m_image;
		Size2d m_display_size;
		Position2d m_display_position;

		Stream* m_alpha_channel = nullptr;
	};
}
