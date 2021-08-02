// writer.cpp
// Copyright (c) 2021, zhiayang
// Licensed under the Apache License Version 2.0.

#include <fcntl.h>
#include <unistd.h>

#include <cerrno>
#include <cassert>
#include <cstring>
#include <cstdlib>

#include "pdf/misc.h"
#include "pdf/writer.h"
#include "pdf/object.h"

namespace pdf
{
	Writer::Writer(zst::str_view path)
	{
		this->path = std::move(path);
		this->bytes_written = 0;
		this->nesting = 0;

		if(this->fd = open(path.str().c_str(), O_WRONLY | O_CREAT | O_TRUNC, 0664); this->fd < 0)
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
			::close(this->fd);

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
		auto n = ::write(this->fd, bytes, len);
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
