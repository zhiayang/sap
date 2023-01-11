// object.h
// Copyright (c) 2021, zhiayang
// SPDX-License-Identifier: Apache-2.0

#pragma once

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
	struct File;
	struct Dictionary;

	size_t getNewResourceId();

	struct Object
	{
		virtual ~Object();

		// write with the default behaviour
		void write(Writer* w) const;

		void collectIndirectObjectsAndAssignIds(File* document);
		void writeIndirectObjects(Writer* w) const;

		// write the full definition (without the indirect definition), even for
		// dictionaries and streams.
		virtual void writeFull(Writer* w) const = 0;

		bool isIndirect() const { return m_is_indirect; }
		size_t byteOffset() const { return m_byte_offset; }

		size_t id() const { return m_id; }
		size_t gen() const { return m_gen; }

		template <typename T, typename... Args, typename = std::enable_if_t<std::is_base_of_v<Object, T>>>
		static T* createIndirect(Args&&... args)
		{
			auto obj = util::make<T>(static_cast<Args&&>(args)...);
			obj->m_is_indirect = true;
			return obj;
		}

		template <typename T, typename... Args, typename = std::enable_if_t<std::is_base_of_v<Object, T>>>
		static inline T* create(Args&&... args)
		{
			return util::make<T>(static_cast<Args&&>(args)...);
		}

	protected:
		virtual void assign_children_ids(File* document) = 0;
		virtual void write_indirect_children(Writer* w) const = 0;

		size_t m_id = 0;
		size_t m_gen = 0;

		bool m_assigned_id = false;
		mutable bool m_written = false;

		bool m_is_indirect = false;
		mutable size_t m_byte_offset = 0;

		friend struct IndirHelper;
	};

	struct Boolean : Object
	{
		explicit Boolean(bool value) : m_value(value) { }
		bool value() const { return m_value; }

		virtual void writeFull(Writer* w) const override;
		virtual void assign_children_ids(File* document) override;
		virtual void write_indirect_children(Writer* w) const override;

		static Boolean* create(bool value);

	private:
		bool m_value = false;
	};

	struct Integer : Object
	{
		explicit Integer(int64_t value) : m_value(value) { }
		int64_t value() const { return m_value; }

		virtual void writeFull(Writer* w) const override;
		virtual void assign_children_ids(File* document) override;
		virtual void write_indirect_children(Writer* w) const override;

		static Integer* create(int64_t value);

	private:
		int64_t m_value = 0;
	};

	struct Decimal : Object
	{
		explicit Decimal(double value) : m_value(value) { }
		double value() const { return m_value; }

		virtual void writeFull(Writer* w) const override;
		virtual void assign_children_ids(File* document) override;
		virtual void write_indirect_children(Writer* w) const override;

		static Decimal* create(double value);

	private:
		double m_value = 0;
	};

	struct String : Object
	{
		explicit String(zst::str_view value) : m_value(value.str()) { }
		const std::string& value() const { return m_value; }

		virtual void writeFull(Writer* w) const override;
		virtual void assign_children_ids(File* document) override;
		virtual void write_indirect_children(Writer* w) const override;

		static String* create(zst::str_view value);

	private:
		std::string m_value {};
	};

	struct Name : Object
	{
		explicit Name(zst::str_view name) : m_name(name.str()) { }

		// special because our builtin names are values and not pointers
		Name* ptr() const { return const_cast<Name*>(this); }
		const std::string& name() const { return m_name; }

		virtual void writeFull(Writer* w) const override;
		virtual void assign_children_ids(File* document) override;
		virtual void write_indirect_children(Writer* w) const override;

		static Name* create(zst::str_view name);


	private:
		std::string m_name {};
	};

	struct Array : Object
	{
		explicit Array(std::vector<Object*> values) : m_values(std::move(values)) { }

		const std::vector<Object*>& values() const { return m_values; }
		std::vector<Object*>& values() { return m_values; }

		void clear();
		void append(Object* obj);

		virtual void writeFull(Writer* w) const override;
		virtual void assign_children_ids(File* document) override;
		virtual void write_indirect_children(Writer* w) const override;


		static Array* create(std::vector<Object*> objs);
		static Array* createIndirect(std::vector<Object*> objs);

		template <typename... Objs>
		static Array* create(Objs&&... objs)
		{
			return Array::create(std::vector<Object*> { objs... });
		}

		template <typename... Objs>
		static Array* createIndirect(Objs&&... objs)
		{
			return Array::createIndirect(std::vector<Object*> { objs... });
		}

	private:
		std::vector<Object*> m_values;
	};

	struct Dictionary : Object
	{
		Dictionary() { }
		explicit Dictionary(std::map<Name, Object*> values) : m_values(std::move(values)) { }

		void add(const Name& n, Object* obj);
		void addOrReplace(const Name& n, Object* obj);
		void remove(const Name& n);
		bool empty() const { return m_values.empty(); }

		virtual void writeFull(Writer* w) const override;
		virtual void assign_children_ids(File* document) override;
		virtual void write_indirect_children(Writer* w) const override;

		static Dictionary* create(std::map<Name, Object*> values);
		static Dictionary* create(const Name& type, std::map<Name, Object*> values);
		static Dictionary* createIndirect(std::map<Name, Object*> values);
		static Dictionary* createIndirect(const Name& type, std::map<Name, Object*> values);

		Object* valueForKey(const Name& name) const;

	private:
		std::map<Name, Object*> m_values;
	};

	// owns the memory.
	struct Stream : Object
	{
		explicit Stream(Dictionary* dict, zst::byte_buffer bytes) : m_bytes(std::move(bytes)), m_dict(dict) { }
		~Stream();

		Dictionary* dictionary() { return m_dict; }
		const Dictionary* dictionary() const { return m_dict; }

		bool isCompressed() const { return m_compressed; }
		void setCompressed(bool compressed);

		void append(zst::str_view xs);
		void append(zst::byte_span xs);
		void append(const uint8_t* arr, size_t num);

		void clear();
		void setContents(zst::byte_span bytes);

		template <typename T>
		void append_bytes(const T& value)
		{
			this->append(reinterpret_cast<const uint8_t*>(&value), sizeof(T));
		}

		virtual void writeFull(Writer* w) const override;
		virtual void assign_children_ids(File* document) override;
		virtual void write_indirect_children(Writer* w) const override;

		static Stream* create();
		static Stream* create(zst::byte_buffer bytes);
		static Stream* create(Dictionary* dict, zst::byte_buffer bytes);

		// TODO: FOR DEBUGGING
		void write_to_file(void* f) const;

	private:
		zst::byte_buffer m_bytes;
		bool m_compressed = false;
		Dictionary* m_dict = nullptr;
	};

	struct IndirectRef : Object
	{
		IndirectRef(Object* obj) : m_object(obj) { }

		virtual void writeFull(Writer* w) const override;
		virtual void assign_children_ids(File* document) override;
		virtual void write_indirect_children(Writer* w) const override;

		static IndirectRef* create(Object* ref);

	private:
		Object* m_object;
	};

	struct Null : Object
	{
		Null() { }

		virtual void writeFull(Writer* w) const override;
		virtual void assign_children_ids(File* document) override;
		virtual void write_indirect_children(Writer* w) const override;

		static Null* get();
	};


	inline bool operator<(const Name& a, const Name& b)
	{
		return a.name() < b.name();
	}

	// list of names
	namespace names
	{
		// TODO: this shit needs to be interned without a shitty usage api like this
		static const auto Ascent = pdf::Name("Ascent");
		static const auto BaseFont = pdf::Name("BaseFont");
		static const auto BitsPerComponent = pdf::Name("BitsPerComponent");
		static const auto CIDFontType0 = pdf::Name("CIDFontType0");
		static const auto CIDFontType0C = pdf::Name("CIDFontType0C");
		static const auto CIDFontType2 = pdf::Name("CIDFontType2");
		static const auto CIDSet = pdf::Name("CIDSet");
		static const auto CIDSystemInfo = pdf::Name("CIDSystemInfo");
		static const auto CIDToGIDMap = pdf::Name("CIDToGIDMap");
		static const auto CapHeight = pdf::Name("CapHeight");
		static const auto Catalog = pdf::Name("Catalog");
		static const auto ColorSpace = pdf::Name("ColorSpace");
		static const auto Contents = pdf::Name("Contents");
		static const auto Count = pdf::Name("Count");
		static const auto DeviceCMYK = pdf::Name("DeviceCMYK");
		static const auto DeviceGray = pdf::Name("DeviceGray");
		static const auto DeviceRGB = pdf::Name("DeviceRGB");
		static const auto DW = pdf::Name("DW");
		static const auto Decode = pdf::Name("Decode");
		static const auto DescendantFonts = pdf::Name("DescendantFonts");
		static const auto Descent = pdf::Name("Descent");
		static const auto Encoding = pdf::Name("Encoding");
		static const auto Filter = pdf::Name("Filter");
		static const auto FirstChar = pdf::Name("FirstChar");
		static const auto Flags = pdf::Name("Flags");
		static const auto FlateDecode = pdf::Name("FlateDecode");
		static const auto Font = pdf::Name("Font");
		static const auto FontBBox = pdf::Name("FontBBox");
		static const auto FontDescriptor = pdf::Name("FontDescriptor");
		static const auto FontFile2 = pdf::Name("FontFile2");
		static const auto FontFile3 = pdf::Name("FontFile3");
		static const auto FontName = pdf::Name("FontName");
		static const auto Height = pdf::Name("Height");
		static const auto Identity = pdf::Name("Identity");
		static const auto Image = pdf::Name("Image");
		static const auto Interpolate = pdf::Name("Interpolate");
		static const auto ItalicAngle = pdf::Name("ItalicAngle");
		static const auto Kids = pdf::Name("Kids");
		static const auto LastChar = pdf::Name("LastChar");
		static const auto Length = pdf::Name("Length");
		static const auto Length1 = pdf::Name("Length1");
		static const auto MediaBox = pdf::Name("MediaBox");
		static const auto Name = pdf::Name("Name");
		static const auto OpenType = pdf::Name("OpenType");
		static const auto Ordering = pdf::Name("Ordering");
		static const auto Page = pdf::Name("Page");
		static const auto Pages = pdf::Name("Pages");
		static const auto Parent = pdf::Name("Parent");
		static const auto Registry = pdf::Name("Registry");
		static const auto Resources = pdf::Name("Resources");
		static const auto Root = pdf::Name("Root");
		static const auto Sap = pdf::Name("Sap");
		static const auto Size = pdf::Name("Size");
		static const auto StemV = pdf::Name("StemV");
		static const auto Subtype = pdf::Name("Subtype");
		static const auto Supplement = pdf::Name("Supplement");
		static const auto ToUnicode = pdf::Name("ToUnicode");
		static const auto TrueType = pdf::Name("TrueType");
		static const auto Type = pdf::Name("Type");
		static const auto Type0 = pdf::Name("Type0");
		static const auto Type1 = pdf::Name("Type1");
		static const auto W = pdf::Name("W");
		static const auto Width = pdf::Name("Width");
		static const auto Widths = pdf::Name("Widths");
		static const auto XHeight = pdf::Name("XHeight");
		static const auto XObject = pdf::Name("XObject");
	}
}
