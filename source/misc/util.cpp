// util.cpp
// Copyright (c) 2021, zhiayang
// SPDX-License-Identifier: Apache-2.0

#include <fcntl.h>
#include <unistd.h>
#include <sys/stat.h>
#include <sys/mman.h>

#include <cerrno>

#include "util.h"
#include "error.h"

namespace util
{
	std::pair<uint8_t*, size_t> readEntireFile(const std::string& path)
	{
		auto fd = open(path.c_str(), O_RDONLY);
		if(fd < 0)
			sap::internal_error("failed to open '{}'; read(): {}", path, strerror(errno));

		struct stat st;
		if(fstat(fd, &st) < 0)
			sap::internal_error("failed to stat '{}'; fstat(): {}", path, strerror(errno));

		auto size = static_cast<size_t>(st.st_size);
		auto ptr = mmap(0, size, PROT_READ, MAP_PRIVATE, fd, /* offset: */ 0);
		if(ptr == reinterpret_cast<void*>(-1))
			sap::internal_error("failed to read '{}': mmap(): {}", path, strerror(errno));

		return { reinterpret_cast<uint8_t*>(ptr), size };
	}

	uint16_t convertBEU16(uint16_t x)
	{
		return ((x & 0x00ff) << 8) | ((x & 0xff00) >> 8);
	}

	uint32_t convertBEU32(uint32_t x)
	{
		return ((x & 0x000000ff) << 24) | ((x & 0x0000ff00) << 8) | ((x & 0x00ff0000) >> 8) | ((x & 0xff000000) >> 24);
	}
}
