// assign_ids.cpp
// Copyright (c) 2022, zhiayang
// SPDX-License-Identifier: Apache-2.0

#include "pdf/file.h"
#include "pdf/object.h"

namespace pdf
{
	void Object::collectIndirectObjectsAndAssignIds(File* doc)
	{
		if(m_assigned_id)
			return;

		m_assigned_id = true;
		this->assign_children_ids(doc);

		if(dynamic_cast<const IndirectRef*>(this) != nullptr)
		{
			// do nothing
		}
		else if(m_is_indirect)
		{
			m_id = doc->getNewObjectId();
			m_gen = 0;

			doc->addObject(this);
		}
	}

	void Object::writeIndirectObjects(Writer* w) const
	{
		if(m_written)
			return;

		m_written = true;
		this->write_indirect_children(w);

		bool is_indirect_ref = (dynamic_cast<const IndirectRef*>(this) != nullptr);
		if(not m_is_indirect && not is_indirect_ref)
			return;

		if(not m_assigned_id)
			sap::internal_error("pdf: did not assign id to indirect object?");

		if(not is_indirect_ref)
			this->writeFull(w);
	}

	void Array::assign_children_ids(File* doc)
	{
		for(auto obj : m_values)
			obj->collectIndirectObjectsAndAssignIds(doc);
	}

	void Dictionary::assign_children_ids(File* doc)
	{
		for(auto& [_, obj] : m_values)
			obj->collectIndirectObjectsAndAssignIds(doc);
	}

	void Stream::assign_children_ids(File* doc)
	{
		m_dict->collectIndirectObjectsAndAssignIds(doc);
	}

	void IndirectRef::assign_children_ids(File* doc)
	{
		m_object->collectIndirectObjectsAndAssignIds(doc);
	}



	void Array::write_indirect_children(Writer* w) const
	{
		for(auto obj : m_values)
			obj->writeIndirectObjects(w);
	}

	void Dictionary::write_indirect_children(Writer* w) const
	{
		for(auto& [_, obj] : m_values)
			obj->writeIndirectObjects(w);
	}

	void Stream::write_indirect_children(Writer* w) const
	{
		m_dict->writeIndirectObjects(w);
	}

	void IndirectRef::write_indirect_children(Writer* w) const
	{
		m_object->writeIndirectObjects(w);
	}





	// none of these do anything, since they have no children.
	void Boolean::write_indirect_children(Writer* w) const
	{
	}

	void Integer::write_indirect_children(Writer* w) const
	{
	}

	void Decimal::write_indirect_children(Writer* w) const
	{
	}

	void String::write_indirect_children(Writer* w) const
	{
	}

	void Name::write_indirect_children(Writer* w) const
	{
	}

	void Null::write_indirect_children(Writer* w) const
	{
	}

	void Boolean::assign_children_ids(File* doc)
	{
	}

	void Integer::assign_children_ids(File* doc)
	{
	}

	void Decimal::assign_children_ids(File* doc)
	{
	}

	void String::assign_children_ids(File* doc)
	{
	}

	void Name::assign_children_ids(File* doc)
	{
	}

	void Null::assign_children_ids(File* doc)
	{
	}
}
