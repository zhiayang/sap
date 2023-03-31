// stream.cpp
// Copyright (c) 2021, zhiayang
// SPDX-License-Identifier: Apache-2.0

#include <miniz/miniz.h> // for tdefl_compress_buffer, tdefl_init, TDEFL_F...

#include "util.h" // for checked_cast

#include "pdf/file.h"   // for File
#include "pdf/misc.h"   // for error, IndirHelper
#include "pdf/object.h" // for Stream, Dictionary, Integer, Name, Length
#include "pdf/writer.h" // for Writer

namespace pdf
{
	constexpr int COMPRESSION_LEVEL = 2048;

	// TODO: FOR DEBUGGING
	void Stream::write_to_file(void* f) const
	{
		auto file = (FILE*) f;
		fwrite(m_bytes.data(), 1, m_bytes.size(), file);
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

		auto write_the_thing = [](Writer* w, Dictionary* dict, const zst::byte_buffer& buf) {
			dict->writeFull(w);

			w->writeln();
			w->writeln("stream\r");

			w->writeBytes(buf.data(), buf.size());

			w->writeln("\r");
			w->write("endstream");
		};


		if(m_compressed)
		{
			tdefl_compressor compressor {};
			zst::byte_buffer compressed_bytes {};

			tdefl_init(
				&compressor,
				[](const void* buf, int len, void* user) -> int {
					static_cast<zst::byte_buffer*>(user)->append(static_cast<const uint8_t*>(buf),
						util::checked_cast<size_t>(len));
					return 1;
				},
				(void*) &compressed_bytes, COMPRESSION_LEVEL | TDEFL_WRITE_ZLIB_HEADER);


			auto res = tdefl_compress_buffer(&compressor, m_bytes.data(), m_bytes.size(), TDEFL_SYNC_FLUSH);
			if(res != TDEFL_STATUS_OKAY)
				pdf::error("stream compression failed");

			res = tdefl_compress_buffer(&compressor, 0, 0, TDEFL_FINISH);
			if(res != TDEFL_STATUS_DONE)
				pdf::error("failed to flush the stream: {}", res);

			m_dict->addOrReplace(names::Length1, Integer::create(util::checked_cast<int64_t>(m_bytes.size())));
			m_dict->addOrReplace(names::Length, Integer::create(util::checked_cast<int64_t>(compressed_bytes.size())));
			m_dict->addOrReplace(names::Filter, names::FlateDecode.ptr());

			write_the_thing(w, m_dict, compressed_bytes);
		}
		else
		{
			m_dict->addOrReplace(names::Length, Integer::create(util::checked_cast<int64_t>(m_bytes.size())));
			write_the_thing(w, m_dict, m_bytes);
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
