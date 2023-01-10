// util.cpp
// Copyright (c) 2021, zhiayang
// SPDX-License-Identifier: Apache-2.0

#include <fcntl.h>    // for open, O_RDONLY
#include <sys/mman.h> // for size_t, mmap, MAP_PRIVATE, PROT_READ
#include <sys/stat.h> // for fstat, stat

#include "util.h" // for readEntireFile

namespace util
{
	zst::unique_span<uint8_t[]> readEntireFile(const std::string& path)
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

		return zst::unique_span<uint8_t[]>((uint8_t*) ptr, size, [](const void* ptr, size_t n) {
			munmap((void*) ptr, n);
		});
	}
}
