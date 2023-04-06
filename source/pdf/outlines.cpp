// outlines.cpp
// Copyright (c) 2022, zhiayang
// SPDX-License-Identifier: Apache-2.0

#include "pdf/pdf.h"
#include "pdf/destination.h"

namespace pdf
{
	OutlineItem::OutlineItem(std::string name, Destination dest, State state)
		: m_name(std::move(name)), m_destination(std::move(dest)), m_default_state(state)
	{
	}

	void OutlineItem::addChild(OutlineItem child)
	{
		m_children.push_back(std::move(child));
	}

	std::pair<Dictionary*, Dictionary*> OutlineItem::linkChildren( //
		const std::vector<OutlineItem>& children,
		Dictionary* parent,
		File* file)
	{
		assert(not children.empty());
		auto first_child = children.front().toDictionary(parent, file);
		auto last_child = children.size() == 1 ? first_child : children.back().toDictionary(parent, file);

		// now, string together the other children.
		Dictionary* current_child = first_child;
		for(size_t i = 1; i < children.size(); i++)
		{
			auto right_sibling = i + 1 == children.size() ? last_child : children[i].toDictionary(parent, file);
			current_child->add(names::Next, right_sibling);
			right_sibling->add(names::Prev, current_child);

			current_child = right_sibling;
		}

		return { first_child, last_child };
	}

	Dictionary* OutlineItem::toDictionary(Dictionary* parent, File* file) const
	{
		auto dict = Dictionary::createIndirect({});
		dict->add(names::Title, String::create(m_name));
		dict->add(names::Parent, parent);

		auto dest = Array::create(IndirectRef::create(file->getPage(m_destination.page)->dictionary()),
			names::XYZ.ptr(),
			Decimal::create(m_destination.position.x().value()), //
			Decimal::create(m_destination.position.y().value()), //
			Decimal::create(m_destination.zoom));

		dict->add(names::A,
			Dictionary::create(names::Action,
				{
					{ names::S, names::GoTo.ptr() },
					{ names::D, dest },
				}));

		if(m_children.size() > 0)
		{
			auto [first_child, last_child] = linkChildren(m_children, dict, file);
			dict->add(names::Count, Integer::create(-checked_cast<int64_t>(m_children.size())));
			dict->add(names::First, first_child);
			dict->add(names::Last, last_child);
		}

		return dict;
	}
}
