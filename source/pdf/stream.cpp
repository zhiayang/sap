// stream.cpp
// Copyright (c) 2021, zhiayang
// SPDX-License-Identifier: Apache-2.0

#include <miniz/miniz.h>

#include "pdf/misc.h"
#include "pdf/writer.h"
#include "pdf/object.h"
#include "pdf/document.h"

namespace pdf
{
	constexpr int COMPRESSION_LEVEL = 2048;

	// TODO: FOR DEBUGGING
	void Stream::write_to_file(void* f) const
	{
		auto file = (FILE*) f;
		fwrite(this->bytes.data(), 1, this->bytes.size(), file);
	}

	Stream::~Stream()
	{
		// since we made the compressor state, we need to free it.
		if(this->is_compressed && this->compressor_state != nullptr)
			tdefl_compressor_free(reinterpret_cast<tdefl_compressor*>(this->compressor_state));
	}

	void Stream::setCompressed(bool compressed)
	{
		if(compressed != this->is_compressed)
		{
			if(this->bytes.size() > 0)
				pdf::error("cannot change stream compression when it has data");

			this->is_compressed = compressed;
			if(this->is_compressed)
			{
				this->dict->addOrReplace(names::Filter, names::FlateDecode.ptr());
				this->compressor_state = tdefl_compressor_alloc();

				auto res = tdefl_init(
					reinterpret_cast<tdefl_compressor*>(this->compressor_state),
					[](const void* buf, int len, void* user) -> int {
						reinterpret_cast<Stream*>(user)->bytes.append(reinterpret_cast<const uint8_t*>(buf), len);
						return 1;
					},
					this, COMPRESSION_LEVEL | TDEFL_WRITE_ZLIB_HEADER);

				if(res != TDEFL_STATUS_OKAY)
					pdf::error("failed to initialise deflate state");
			}
			else
			{
				this->dict->remove(names::Filter);
				if(this->compressor_state != nullptr)
				{
					tdefl_compressor_free(reinterpret_cast<tdefl_compressor*>(this->compressor_state));
					this->compressor_state = nullptr;
				}
			}
		}
	}

	void Stream::writeFull(Writer* w) const
	{
		if(!this->is_indirect)
			pdf::error("cannot write non-materialised stream (not bound to a document)");

		if(this->is_compressed)
		{
			// if we are compressed, flush the stream.
			auto res = tdefl_compress_buffer(reinterpret_cast<tdefl_compressor*>(this->compressor_state), 0, 0, TDEFL_FINISH);
			if(res != TDEFL_STATUS_DONE)
				pdf::error("failed to flush the stream: {}", res);
		}
		this->dict->addOrReplace(names::Length, Integer::create(this->bytes.size()));

		IndirHelper helper(w, this);

		this->dict->writeFull(w);

		w->writeln();
		w->writeln("stream\r");

		w->writeBytes(this->bytes.data(), this->bytes.size());

		w->writeln("\r");
		w->write("endstream");
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
		if(this->is_compressed)
		{
			auto res =
				tdefl_compress_buffer(reinterpret_cast<tdefl_compressor*>(this->compressor_state), arr, num, TDEFL_SYNC_FLUSH);

			if(res != TDEFL_STATUS_OKAY)
				pdf::error("stream compression failed");
		}
		else
		{
			this->bytes.append(arr, num);
		}

		this->dict->addOrReplace(names::Length, Integer::create(this->bytes.size()));
		this->uncompressed_length += num;
	}

	void Stream::attach(Document* document)
	{
		if(this->is_indirect)
			pdf::error("stream has already been attached to a document");

		this->makeIndirect(document);
	}


	Stream* Stream::create(Document* doc, zst::byte_buffer bytes)
	{
		auto dict = Dictionary::create({ { names::Length, Integer::create(bytes.size()) } });

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

}
