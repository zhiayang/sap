// writer.cpp
// Copyright (c) 2021, yuki / zhiayang
// SPDX-License-Identifier: Apache-2.0

#if defined(_WIN32)
#include <io.h>
#else
#include <unistd.h>
#endif

#include <fcntl.h> // for open, O_CREAT, O_TRUNC, O_WRONLY

#include "pdf/misc.h"   // for error
#include "pdf/object.h" // for Object
#include "pdf/writer.h" // for Writer

namespace pdf
{
	Writer::Writer(zst::str_view path_)
	{
		this->path = std::move(path_);
		this->bytes_written = 0;
		this->nesting = 0;

#if defined(_WIN32)
		if(this->fd = _open(path.str().c_str(), _O_WRONLY | _O_CREAT | _O_TRUNC, _S_IREAD | _S_IWRITE); this->fd < 0)
#else
		if(this->fd = open(path.str().c_str(), O_WRONLY | O_CREAT | O_TRUNC, 0664); this->fd < 0)
#endif
			pdf::error("failed to open file for writing; open(): {}", strerror(errno));
	}

	Writer::~Writer()
	{
		this->close();
	}

	size_t Writer::position() const
	{
		return this->bytes_written;
	}

	void Writer::close()
	{
		if(this->fd != -1)
		{
#if defined(_WIN32)
			_close(this->fd);
#else
			::close(this->fd);
#endif
		}

		this->fd = -1;
	}

	void Writer::write(const Object* obj)
	{
		obj->write(this);
	}

	void Writer::writeFull(const Object* obj)
	{
		obj->writeFull(this);
	}

	size_t Writer::write(zst::str_view sv)
	{
		return this->writeBytes(reinterpret_cast<const uint8_t*>(sv.data()), sv.size());
	}

	size_t Writer::writeBytes(const uint8_t* bytes, size_t len)
	{
#if defined(_WIN32)
		auto n = _write(this->fd, bytes, static_cast<unsigned int>(len));
#else
		auto n = ::write(this->fd, bytes, len);
#endif
		if(n < 0 || static_cast<size_t>(n) != len)
			pdf::error("file write failed; write(): {}", strerror(errno));

		this->bytes_written += len;
		return len;
	}

	size_t Writer::writeln(zst::str_view sv)
	{
		return this->write(sv) + this->write("\n");
	}

	size_t Writer::writeln()
	{
		return this->writeln("");
	}
}
