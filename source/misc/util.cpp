// util.cpp
// Copyright (c) 2021, yuki / zhiayang
// SPDX-License-Identifier: Apache-2.0

#include <stdio.h>
#include <fcntl.h>
#include <sys/stat.h>

#if !defined(_WIN32)
#include <unistd.h>
#include <sys/mman.h>
#endif

#include "util.h"

namespace util
{
#if defined(_WIN32)
	zst::unique_span<uint8_t[]> readEntireFile(const std::string& path)
	{
		FILE* fd = fopen(path.c_str(), "rb");
		if(fd == nullptr)
			sap::internal_error("failed to open '{}'; fopen(): {}", path, strerror(errno));

		const size_t size = stdfs::file_size(path);
		auto ptr = new uint8_t[size];

		if(fread(&ptr[0], 1, size, fd) != size)
			sap::internal_error("failed to read '{}': fread(): {}", path, strerror(errno));

		fclose(fd);
		return zst::unique_span<uint8_t[]>((uint8_t*) ptr, size, [](const void* p, size_t n) {
			delete[] reinterpret_cast<uint8_t*>(const_cast<void*>(p));
		});
	}

#else
	zst::unique_span<uint8_t[]> readEntireFile(const std::string& path)
	{
		auto fd = open(path.c_str(), O_RDONLY);
		if(fd < 0)
			sap::internal_error("failed to open '{}'; open(): {}", path, strerror(errno));

		struct stat st;
		if(fstat(fd, &st) < 0)
			sap::internal_error("failed to stat '{}'; fstat(): {}", path, strerror(errno));

		auto size = static_cast<size_t>(st.st_size);
		auto ptr = mmap(0, size, PROT_READ, MAP_PRIVATE, fd, /* offset: */ 0);
		if(ptr == reinterpret_cast<void*>(-1))
			sap::internal_error("failed to read '{}': mmap(): {}", path, strerror(errno));

		close(fd);
		return zst::unique_span<uint8_t[]>((uint8_t*) ptr, size, [](const void* p, size_t n) {
			munmap(const_cast<void*>(p), n);
		});
	}
#endif
}

namespace sap
{
	OwnedImageBitmap ImageBitmap::clone() const
	{
		auto num_pixels = this->pixel_width * this->pixel_height;
		auto new_rgb = zst::unique_span<uint8_t[]>(new uint8_t[num_pixels * 3], num_pixels * 3);
		memcpy(new_rgb.get(), this->rgb.data(), num_pixels * 3);

		zst::unique_span<uint8_t[]> new_alpha {};
		if(this->haveAlpha())
		{
			new_alpha = zst::unique_span<uint8_t[]>(new uint8_t[num_pixels], num_pixels);
			memcpy(new_alpha.get(), this->alpha.data(), num_pixels);
		}

		return OwnedImageBitmap {
			.pixel_width = pixel_width,
			.pixel_height = pixel_height,
			.bits_per_pixel = bits_per_pixel,
			.rgb = std::move(new_rgb),
			.alpha = std::move(new_alpha),
		};
	}

	OwnedImageBitmap OwnedImageBitmap::clone() const
	{
		return this->span().clone();
	}
}
