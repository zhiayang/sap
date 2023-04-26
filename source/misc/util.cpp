// util.cpp
// Copyright (c) 2021, zhiayang
// SPDX-License-Identifier: Apache-2.0

#include <fcntl.h> // for open, O_RDONLY
#include <unistd.h>
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

		close(fd);
		return zst::unique_span<uint8_t[]>((uint8_t*) ptr, size, [](const void* p, size_t n) {
			munmap(const_cast<void*>(p), n);
		});
	}
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
