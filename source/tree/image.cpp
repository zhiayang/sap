// image.cpp
// Copyright (c) 2022, zhiayang
// SPDX-License-Identifier: Apache-2.0

#include "interp/tree.h"

#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wsign-conversion"
#pragma GCC diagnostic ignored "-Wimplicit-int-conversion"

#define STB_IMAGE_IMPLEMENTATION
#include "stb_image.h"

#pragma GCC diagnostic pop


namespace sap::tree
{
	// TODO: make ErrorOr<>
	std::unique_ptr<Image> Image::fromImageFile(zst::str_view file_path, sap::Length width, std::optional<sap::Length> height)
	{
		auto file_buf = util::readEntireFile(file_path.str());

		int img_width = 0;
		int img_height = 0;

		// always ask for 3 channels -- r g b.
		auto image_data_buf = stbi_load_from_memory(file_buf.get(), (int) file_buf.size(), &img_width, &img_height, nullptr, 3);
		if(image_data_buf == nullptr)
			sap::internal_error("failed to load image '{}': {}", file_path, stbi_failure_reason());

		auto image_data = zst::unique_span<uint8_t[]>(image_data_buf, //
		    static_cast<size_t>(img_height * img_width * 3),          //
		    [](const void* ptr, size_t n) {
			    stbi_image_free((void*) ptr);
		    });

		auto aspect = (double) img_width / (double) img_height;
		if(not height.has_value())
			height = width / aspect;

		auto ret = std::make_unique<Image>(std::move(image_data), sap::Vector2(width, *height));
		ret->m_pixel_width = (size_t) img_width;
		ret->m_pixel_height = (size_t) img_height;

		return ret;
	}
}
