// object.cpp
// Copyright (c) 2021, zhiayang
// Licensed under the Apache License Version 2.0.

#include "pdf/misc.h"
#include "pdf/object.h"
#include "pdf/writer.h"
#include "pdf/document.h"

namespace pdf
{
	struct IndirHelper
	{
		IndirHelper(Writer* w, const Object* obj) : w(w), indirect(obj->is_indirect)
		{
			if(indirect)
			{
				obj->byte_offset = w->position();
				w->writeln("{} {} obj", obj->id, obj->gen);
			}
		}

		~IndirHelper()
		{
			if(indirect)
				w->writeln("endobj"), w->writeln();
		}

		IndirHelper(IndirHelper&&) = delete;
		IndirHelper(const IndirHelper&) = delete;

		Writer* w = 0;
		bool indirect = false;
	};


	void Object::write(Writer* w) const
	{
		if(this->is_indirect)
			w->write("{} {} R", this->id, this->gen);
		else
			this->writeFull(w);
	}

	void Object::makeIndirect(Document* doc)
	{
		// TODO: should this be an error?
		if(this->is_indirect)
			return;

		this->is_indirect = true;
		this->id = doc->getNewObjectId();
		this->gen = 0;
		doc->addObject(this);
	}

	void Boolean::writeFull(Writer* w) const
	{
		IndirHelper helper(w, this);
		w->write(this->value ? "true" : "false");
	}

	void Integer::writeFull(Writer* w) const
	{
		IndirHelper helper(w, this);
		w->write("{}", this->value);
	}

	void Decimal::writeFull(Writer* w) const
	{
		IndirHelper helper(w, this);
		w->write("{}", this->value);
	}

	void String::writeFull(Writer* w) const
	{
		IndirHelper helper(w, this);

		// for now, always write strings as their hexadecimal encoding...
		w->write("<");
		for(uint8_t c : this->value)
			w->write("{02x}", c);

		w->write(">");
	}

	void Name::writeFull(Writer* w) const
	{
		IndirHelper helper(w, this);

		auto encode_name = [](zst::str_view sv) -> std::string {
			std::string ret;
			ret.reserve(sv.size());

			for(uint8_t c : sv)
			{
				if(c < '!' || c > '~' || c == '#')
					ret += zpr::sprint("#{02x}", c);

				else
					ret.push_back(c);
			}
			return ret;
		};

		w->write("/{}", encode_name(this->name));
	}

	void Array::writeFull(Writer* w) const
	{
		IndirHelper helper(w, this);
		w->write("[ ");
		for(auto obj : this->values)
		{
			obj->write(w);
			w->write(" ");
		}
		w->write("]");
	}

	void Dictionary::writeFull(Writer* w) const
	{
		IndirHelper helper(w, this);

		if(this->values.empty())
		{
			w->write("<< >>");
			return;
		}

		w->writeln("<<");
		w->nesting++;
		bool first = true;
		for(auto& [ name, value ] : this->values)
		{
			w->write("{}", zpr::w(w->nesting * 2)(""));
			name.write(w);
			w->write(" ");
			value->write(w);
			w->writeln();

			first = false;
		}
		w->nesting--;
		w->writeln("{}>>", zpr::w(w->nesting * 2)(""));
	}

	void Stream::writeFull(Writer* w) const
	{
		if(!this->is_indirect)
			pdf::error("cannot write non-materialised stream (not bound to a document)");

		IndirHelper helper(w, this);

		this->dict->writeFull(w);
		w->writeln("stream");

		w->writeBytes(this->bytes.data(), this->bytes.size());

		w->writeln();
		w->writeln("endstream");
	}





	void IndirectRef::writeFull(Writer* w) const
	{
		IndirHelper helper(w, this);
		w->write("{} {} R", this->id, this->generation);
	}

	void Stream::append(zst::byte_span xs)
	{
		this->bytes.append(xs);
		this->dict->addOrReplace(names::Length, Integer::create(this->bytes.size()));
	}

	void Stream::append(const uint8_t* arr, size_t num)
	{
		this->bytes.append(arr, num);
		this->dict->addOrReplace(names::Length, Integer::create(this->bytes.size()));
	}

	void Dictionary::add(const Name& n, Object* obj)
	{
		if(auto it = this->values.find(n); it != this->values.end())
			pdf::error("key '{}' already exists in dictionary", n.name);

		this->values.emplace(n, obj);
	}

	void Dictionary::addOrReplace(const Name& n, Object* obj)
	{
		this->values.insert_or_assign(n, obj);
	}

	Object* Dictionary::valueForKey(const Name& name) const
	{
		if(auto it = this->values.find(name); it != this->values.end())
			return it->second;

		else
			return nullptr;
	}

	void Stream::attach(Document* document)
	{
		if(this->is_indirect)
			pdf::error("stream has already been attached to a document");

		this->makeIndirect(document);
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
		return createObject<Boolean>(value);
	}

	Integer* Integer::create(int64_t value)
	{
		return createObject<Integer>(value);
	}

	Decimal* Decimal::create(double value)
	{
		return createObject<Decimal>(value);
	}

	String* String::create(zst::str_view value)
	{
		return createObject<String>(value);
	}

	Name* Name::create(zst::str_view name)
	{
		return createObject<Name>(name);
	}

	Array* Array::create(std::vector<Object*> objs)
	{
		return createObject<Array>(std::move(objs));
	}

	Array* Array::createIndirect(Document* doc, std::vector<Object*> objs)
	{
		return createIndirectObject<Array>(doc, std::move(objs));
	}

	Dictionary* Dictionary::create(std::map<Name, Object*> values)
	{
		return createObject<Dictionary>(std::move(values));
	}

	Dictionary* Dictionary::createIndirect(Document* doc, std::map<Name, Object*> values)
	{
		return createIndirectObject<Dictionary>(doc, std::move(values));
	}

	Dictionary* Dictionary::create(const Name& type, std::map<Name, Object*> values)
	{
		auto ret = createObject<Dictionary>(std::move(values));
		ret->add(names::Type, const_cast<Name*>(&type));
		return ret;
	}

	Dictionary* Dictionary::createIndirect(Document* doc, const Name& type, std::map<Name, Object*> values)
	{
		auto ret = createIndirectObject<Dictionary>(doc, std::move(values));
		ret->add(names::Type, const_cast<Name*>(&type));
		return ret;
	}

	IndirectRef* IndirectRef::create(Object* ref)
	{
		if(!ref->is_indirect)
			pdf::error("cannot make indirect reference to non-indirect object");

		return createObject<IndirectRef>(ref->id, ref->gen);
	}

	IndirectRef* IndirectRef::create(int64_t id, int64_t gen)
	{
		return createObject<IndirectRef>(id, gen);
	}




	Stream* Stream::create(Document* doc, zst::byte_buffer bytes)
	{
		auto dict = Dictionary::create({
			{ names::Length, Integer::create(bytes.size()) }
		});

		return createIndirectObject<Stream>(doc, dict, std::move(bytes));
	}

	Stream* Stream::create(Document* doc, Dictionary* dict, zst::byte_buffer bytes)
	{
		dict->addOrReplace(names::Length, Integer::create(bytes.size()));
		return createIndirectObject<Stream>(doc, dict, std::move(bytes));
	}

	Stream* Stream::createDetached(Document* doc, Dictionary* dict, zst::byte_buffer bytes)
	{
		dict->addOrReplace(names::Length, Integer::create(bytes.size()));
		return createObject<Stream>(dict, std::move(bytes));
	}

	Object::~Object()
	{
	}
}
