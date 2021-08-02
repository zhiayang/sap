// object.h
// Copyright (c) 2021, zhiayang
// Licensed under the Apache License Version 2.0.

#pragma once

#include <cstdint>
#include <cstddef>

#include <map>
#include <string>
#include <vector>
#include <utility>
#include <type_traits>

#include "pool.h"

namespace pdf
{
	struct Writer;

	/*
		PDF objects, which include: booleans, integers,
		real numbers, arrays, dictionaries, and streams.

		dictionaries and streams are always referred to indirectly by
		default, except when written with writeFull(). Otherwise,
		the "simple types" (including arrays) are written out in full
		at their point of use, and don't get an id/generation.

		names are never indirect, they are always written properly.
	*/

	struct Stream;
	struct Document;
	struct Dictionary;

	struct Object
	{
		virtual ~Object();

		// write with the default behaviour
		void write(Writer* w) const;

		// write the full definition (without the indirect definition), even for
		// dictionaries and streams.
		virtual void writeFull(Writer* w) const = 0;

		size_t id = 0;
		size_t gen = 0;
		bool is_indirect = false;

		mutable size_t byte_offset = 0;
	};

	struct Boolean : Object
	{
		explicit Boolean(bool value) : value(value) { }
		virtual void writeFull(Writer* w) const override;

		static Boolean* create(bool value);

		bool value = false;
	};

	struct Integer : Object
	{
		explicit Integer(int64_t value) : value(value) { }
		virtual void writeFull(Writer* w) const override;

		static Integer* create(int64_t value);

		int64_t value = 0;
	};

	struct Decimal : Object
	{
		explicit Decimal(double value) : value(value) { }
		virtual void writeFull(Writer* w) const override;

		static Decimal* create(double value);

		double value = 0;
	};

	struct String : Object
	{
		// explicit String(std::string value) : value(std::move(value)) { }
		explicit String(zst::str_view value) : value(value.str()) { }
		virtual void writeFull(Writer* w) const override;

		static String* create(zst::str_view value);


		std::string value { };
	};

	struct Name : Object
	{
		explicit Name(zst::str_view name) : name(name.str()) { }
		virtual void writeFull(Writer* w) const override;

		static Name* create(zst::str_view name);

		std::string name { };
	};

	struct Array : Object
	{
		explicit Array(std::vector<Object*> values) : values(std::move(values)) { }

		virtual void writeFull(Writer* w) const override;

		static Array* create(std::vector<Object*> objs);
		static Array* createIndirect(Document* doc, std::vector<Object*> objs);

		template <typename... Objs>
		static Array* create(Objs&&... objs)
		{
			return Array::create(std::vector<Object*> { objs... });
		}

		template <typename... Objs>
		static Array* createIndirect(Document* doc, Objs&&... objs)
		{
			return Array::createIndirect(doc, std::vector<Object*> { objs... });
		}



		std::vector<Object*> values;
	};

	struct Dictionary : Object
	{
		Dictionary() { }
		explicit Dictionary(std::map<Name, Object*> values) : values(std::move(values)) { }

		void add(const Name& n, Object* obj);
		void addOrReplace(const Name& n, Object* obj);

		virtual void writeFull(Writer* w) const override;

		static Dictionary* create(std::map<Name, Object*> values);
		static Dictionary* create(const Name& type, std::map<Name, Object*> values);
		static Dictionary* createIndirect(Document* doc, std::map<Name, Object*> values);
		static Dictionary* createIndirect(Document* doc, const Name& type, std::map<Name, Object*> values);

		Object* valueForKey(const Name& name) const;


		std::map<Name, Object*> values;
	};

	// owns the memory.
	struct Stream : Object
	{
		explicit Stream(Dictionary* dict, std::vector<uint8_t> bytes) : dict(dict), bytes(std::move(bytes)) { }

		virtual void writeFull(Writer* w) const override;

		void append(const std::vector<uint8_t>& xs);
		void append(uint8_t* arr, size_t num);

		static Stream* create(Document* doc, std::vector<uint8_t> bytes);
		static Stream* create(Document* doc, Dictionary* dict, std::vector<uint8_t> bytes);

		Dictionary* dict = 0;
		std::vector<uint8_t> bytes;
	};

	struct IndirectRef : Object
	{
		IndirectRef(int64_t id, int64_t gen) : id(id), generation(gen) { }

		virtual void writeFull(Writer* w) const override;

		static IndirectRef* create(Object* ref);
		static IndirectRef* create(int64_t id, int64_t gen);

		int64_t id = 0;
		int64_t generation = 0;
	};









	static constexpr bool operator < (const Name& a, const Name& b)
	{
		return a.name < b.name;
	}


	// list of names
	namespace names
	{
		static const auto Type      = Name("Type");
		static const auto Catalog   = Name("Catalog");
		static const auto Size      = Name("Size");
		static const auto Count     = Name("Count");
		static const auto Kids      = Name("Kids");
		static const auto Root      = Name("Root");
		static const auto Parent    = Name("Parent");
		static const auto Page      = Name("Page");
		static const auto Length    = Name("Length");
		static const auto Pages     = Name("Pages");
		static const auto Resources = Name("Resources");
		static const auto MediaBox  = Name("MediaBox");
	}
}
