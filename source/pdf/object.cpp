// object.cpp
// Copyright (c) 2021, zhiayang
// SPDX-License-Identifier: Apache-2.0

#include "util.h" // for checked_cast

#include "pdf/file.h"   // for File
#include "pdf/misc.h"   // for IndirHelper, error
#include "pdf/object.h" // for Name, Dictionary, Object, Array, IndirectRef
#include "pdf/writer.h" // for Writer


namespace pdf
{
	static constexpr bool PRETTY_PRINT = false;

	size_t getNewResourceId()
	{
		static size_t s_next_resource_id = 0;
		return ++s_next_resource_id;
	}

	IndirHelper::IndirHelper(Writer* w, const Object* obj) : w(w), indirect(obj->isIndirect())
	{
		if(indirect)
		{
			obj->m_byte_offset = w->position();
			w->writeln("{} {} obj", obj->id(), obj->gen());
		}
	}

	IndirHelper::~IndirHelper()
	{
		if(indirect)
			w->writeln(), w->writeln("endobj"), w->writeln();
	}

	void Object::write(Writer* w) const
	{
		if(m_is_indirect)
			w->write("{} {} R", m_id, m_gen);
		else
			this->writeFull(w);
	}

	void Boolean::writeFull(Writer* w) const
	{
		auto helper = IndirHelper(w, this);
		w->write(m_value ? "true" : "false");
	}

	void Integer::writeFull(Writer* w) const
	{
		auto helper = IndirHelper(w, this);
		w->write("{}", m_value);
	}

	void Decimal::writeFull(Writer* w) const
	{
		auto helper = IndirHelper(w, this);
		w->write("{}", m_value);
	}

	void String::writeFull(Writer* w) const
	{
		auto helper = IndirHelper(w, this);

		// for now, always write strings as their hexadecimal encoding...
		w->write("<");
		for(char c : m_value)
			w->write("{02x}", static_cast<uint8_t>(c));

		w->write(">");
	}

	void Name::writeFull(Writer* w) const
	{
		auto helper = IndirHelper(w, this);

		auto encode_name = [](zst::str_view sv) -> std::string {
			std::string ret;
			ret.reserve(sv.size());

			for(char c : sv)
			{
				if(c < '!' || c > '~' || c == '#')
					ret += zpr::sprint("#{02x}", static_cast<uint8_t>(c));

				else
					ret.push_back(c);
			}
			return ret;
		};

		w->write("/{}", encode_name(m_name));
	}

	void Array::writeFull(Writer* w) const
	{
		auto helper = IndirHelper(w, this);
		w->write("[ ");
		for(auto obj : m_values)
		{
			obj->write(w);
			w->write(" ");
		}
		w->write("]");
	}

	void Dictionary::writeFull(Writer* w) const
	{
		auto helper = IndirHelper(w, this);

		if(m_values.empty())
		{
			w->write("<< >>");
			return;
		}

		w->writeln("<<");
		w->nesting++;
		for(auto& [name, value] : m_values)
		{
			if(PRETTY_PRINT)
				w->write("{}", zpr::w(w->nesting * 2)(""));

			name.write(w);
			w->write(" ");
			value->write(w);
			w->writeln();
		}
		w->nesting--;

		if(PRETTY_PRINT)
			w->write("{}", zpr::w(w->nesting * 2)(""));

		w->write(">>");
	}



	void IndirectRef::writeFull(Writer* w) const
	{
		auto helper = IndirHelper(w, this);
		w->write("{} {} R", m_object->id(), m_object->gen());
	}

	void Dictionary::add(const Name& n, Object* obj)
	{
		if(auto it = m_values.find(n); it != m_values.end())
			pdf::error("key '{}' already exists in dictionary", n.name());

		m_values.emplace(n, obj);
	}

	void Dictionary::addOrReplace(const Name& n, Object* obj)
	{
		m_values.insert_or_assign(n, obj);
	}

	void Dictionary::remove(const Name& n)
	{
		m_values.erase(n);
	}

	Object* Dictionary::valueForKey(const Name& name) const
	{
		if(auto it = m_values.find(name); it != m_values.end())
			return it->second;

		else
			return nullptr;
	}

	void Array::append(Object* obj)
	{
		m_values.push_back(obj);
	}

	void Array::clear()
	{
		m_values.clear();
	}


















	void Null::writeFull(Writer* w) const
	{
		w->write("null");
	}

	Null* Null::get()
	{
		static Null* singleton = 0;
		if(!singleton)
			singleton = util::make<Null>();

		return singleton;
	}


	Boolean* Boolean::create(bool value)
	{
		return Object::create<Boolean>(value);
	}

	Integer* Integer::create(int64_t value)
	{
		return Object::create<Integer>(value);
	}

	Decimal* Decimal::create(double value)
	{
		return Object::create<Decimal>(value);
	}

	String* String::create(zst::str_view value)
	{
		return Object::create<String>(value);
	}

	Name* Name::create(zst::str_view name)
	{
		return Object::create<Name>(name);
	}

	Array* Array::create(std::vector<Object*> objs)
	{
		return Object::create<Array>(std::move(objs));
	}

	Array* Array::createIndirect(std::vector<Object*> objs)
	{
		return Object::createIndirect<Array>(std::move(objs));
	}

	Dictionary* Dictionary::create(std::map<Name, Object*> values)
	{
		return Object::create<Dictionary>(std::move(values));
	}

	Dictionary* Dictionary::createIndirect(std::map<Name, Object*> values)
	{
		return Object::createIndirect<Dictionary>(std::move(values));
	}

	Dictionary* Dictionary::create(const Name& type, std::map<Name, Object*> values)
	{
		auto ret = Object::create<Dictionary>(std::move(values));
		ret->add(names::Type, const_cast<Name*>(&type));
		return ret;
	}

	Dictionary* Dictionary::createIndirect(const Name& type, std::map<Name, Object*> values)
	{
		auto ret = Object::createIndirect<Dictionary>(std::move(values));
		ret->add(names::Type, const_cast<Name*>(&type));
		return ret;
	}

	IndirectRef* IndirectRef::create(Object* ref)
	{
		if(not ref->isIndirect())
			pdf::error("cannot make indirect reference to non-indirect object");

		return Object::create<IndirectRef>(ref);
	}

	Object::~Object()
	{
	}
}
