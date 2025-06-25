// annotation.cpp
// Copyright (c) 2022, yuki
// SPDX-License-Identifier: Apache-2.0

#include "pdf/file.h"
#include "pdf/page.h"
#include "pdf/object.h"
#include "pdf/annotation.h"

namespace pdf
{
	Annotation::Annotation(pdf::Position2d pos, pdf::Size2d size) : m_position(pos), m_size(size)
	{
	}

	LinkAnnotation::LinkAnnotation(pdf::Position2d pos, pdf::Size2d size, Destination dest)
	    : Annotation(pos, size), m_destination(std::move(dest))
	{
	}

	Dictionary* LinkAnnotation::toDictionary(File* file) const
	{
		auto dest = Array::create(IndirectRef::create(file->getPage(m_destination.page)->dictionary()),
		    names::XYZ.ptr(),
		    Decimal::create(m_destination.position.x().value()), //
		    Decimal::create(m_destination.position.y().value()), //
		    Decimal::create(m_destination.zoom));

		auto dict = Dictionary::createIndirect(names::Annot,
		    {
		        { names::Subtype, names::Link.ptr() },
		        {
		            // our m_position is the upper left, but pdf expects the Rect to be lower-left / upper-right
		            // so we need to adjust this a bit. this is further complicated by the fact that here, we
		            // are working in PDF space -- Y-up.
		            names::Rect,
		            Array::create(                                               //
		                Decimal::create(m_position.x().value()),                 //
		                Decimal::create(m_position.y().value()),                 //
		                Decimal::create((m_position.x() + m_size.x()).value()),  //
		                Decimal::create((m_position.y() + m_size.y()).value())), //
		        },
		        // { names::BS, Dictionary::create(names::Border, { { names::S, names::S.ptr() } }) },
		        { names::F, Integer::create(1 << 2) }, // set the Print flag
		        { names::C, Array::create(Decimal::create(1.0), Decimal::create(0), Decimal::create(0)) },
		        {
		            names::A,
		            Dictionary::create(names::Action, { { names::S, names::GoTo.ptr() }, { names::D, dest } }),
		        },
		    });

		return dict;
	}
}
