// stream.cpp
// Copyright (c) 2021, zhiayang
// SPDX-License-Identifier: Apache-2.0

#include <libdeflate/libdeflate.h>

#include "util.h" // for checked_cast

#include "pdf/file.h"   // for File
#include "pdf/misc.h"   // for error, IndirHelper
#include "pdf/object.h" // for Stream, Dictionary, Integer, Name, Length
#include "pdf/writer.h" // for Writer

namespace pdf
{
	// TODO: FOR DEBUGGING
	void Stream::write_to_file(void* f) const
	{
		auto file = (FILE*) f;
		fwrite(m_bytes.data(), 1, m_bytes.size(), file);
	}

	Stream::Stream(Dictionary* dict, zst::byte_buffer bytes)
		: m_bytes(std::move(bytes)), m_compressed(true), m_dict(dict)
	{
	}

	Stream::~Stream()
	{
	}

	void Stream::clear()
	{
		m_bytes.clear();
	}

	void Stream::setCompressed(bool compressed)
	{
		m_compressed = compressed;
	}

	void Stream::writeFull(Writer* w) const
	{
		if(not this->isIndirect())
			pdf::error("cannot write non-materialised stream (not bound to a document)");

		auto helper = IndirHelper(w, this);

		auto write_the_thing = [](Writer* w, Dictionary* dict, zst::byte_span buf) {
			dict->writeFull(w);

			w->writeln();
			w->writeln("stream\r");

			w->writeBytes(buf.data(), buf.size());

			w->writeln("\r");
			w->write("endstream");
		};


		bool did_compress = false;
		do
		{
			// needs slack space
			auto buf_size = m_bytes.size() + 10;
			auto compressed_bytes = new uint8_t[buf_size];
			auto _ = util::Defer([&]() { delete[] compressed_bytes; });

			auto compressor = libdeflate_alloc_compressor(5);
			assert(compressor != nullptr);

			auto compressed_len = libdeflate_deflate_compress(compressor, m_bytes.data(), m_bytes.size(),
				compressed_bytes, buf_size);

			if(compressed_len == 0)
				break;

			did_compress = true;

			m_dict->addOrReplace(names::Length1, Integer::create(util::checked_cast<int64_t>(m_bytes.size())));
			m_dict->addOrReplace(names::Length, Integer::create(util::checked_cast<int64_t>(compressed_len)));
			m_dict->addOrReplace(names::Filter, names::FlateDecode.ptr());

			libdeflate_free_compressor(compressor);
			write_the_thing(w, m_dict, zst::byte_span(compressed_bytes, compressed_len));
		} while(false);

		if(not did_compress)
		{
			m_dict->addOrReplace(names::Length, Integer::create(util::checked_cast<int64_t>(m_bytes.size())));
			write_the_thing(w, m_dict, m_bytes.span());
		}
	}

	void Stream::append(zst::str_view xs)
	{
		this->append(reinterpret_cast<const uint8_t*>(xs.data()), xs.size());
	}

	void Stream::append(zst::byte_span xs)
	{
		this->append(xs.data(), xs.size());
	}

	void Stream::append(const uint8_t* arr, size_t num)
	{
		m_bytes.append(arr, num);
	}

	void Stream::setContents(zst::byte_span bytes)
	{
		this->clear();
		this->append(bytes);
	}

	Stream* Stream::create()
	{
		return Stream::create(zst::byte_buffer());
	}

	Stream* Stream::create(zst::byte_buffer bytes)
	{
		auto dict = Dictionary::create({ { names::Length,
			Integer::create(util::checked_cast<int64_t>(bytes.size())) } });

		return Object::createIndirect<Stream>(dict, std::move(bytes));
	}

	Stream* Stream::create(Dictionary* dict, zst::byte_buffer bytes)
	{
		dict->addOrReplace(names::Length, Integer::create(util::checked_cast<int64_t>(bytes.size())));
		return Object::createIndirect<Stream>(dict, std::move(bytes));
	}
}
