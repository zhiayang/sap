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

		int img_width_ = 0;
		int img_height_ = 0;

		// always ask for 3 channels -- r g b.
		int num_actual_channels = 0;
		auto image_data_buf = stbi_load_from_memory(file_buf.get(), (int) file_buf.size(), &img_width_, &img_height_,
		    &num_actual_channels, 3);

		if(image_data_buf == nullptr)
			sap::internal_error("failed to load image '{}': {}", file_path, stbi_failure_reason());

		auto img_width = (size_t) img_width_;
		auto img_height = (size_t) img_height_;

		auto image_rgb_data = zst::unique_span<uint8_t[]>(image_data_buf, img_height * img_width * 3, //
		    [](const void* ptr, size_t n) {
			    stbi_image_free((void*) ptr);
		    });

		auto aspect = (double) img_width / (double) img_height;
		if(not height.has_value())
			height = width / aspect;

		auto image = OwnedImageBitmap {
			.pixel_width = img_width,
			.pixel_height = img_height,
			.bits_per_pixel = 8,
			.rgb = std::move(image_rgb_data),
			.alpha {},
		};

		if(num_actual_channels > 3)
		{
			// ok, we need to load the alpha channel now. however, stbi doesn't let us load *just* the alpha
			// channel; so, we load 2 channels, which they will convert to grey+alpha. we then just
			// take every other byte.
			int x = 0;
			int y = 0;
			auto alpha_data_buf = stbi_load_from_memory(file_buf.get(), (int) file_buf.size(), &x, &y, nullptr, 2);

			auto alpha_data = zst::unique_span<uint8_t[]>(new uint8_t[img_width * img_height], img_height * img_width);
			for(size_t i = 0; i < img_width * img_height; i++)
				alpha_data[i] = alpha_data_buf[i * 2 + 1];

			image.alpha = std::move(alpha_data);
		}

		return std::make_unique<Image>(std::move(image), sap::Vector2(width, *height));
	}
}
