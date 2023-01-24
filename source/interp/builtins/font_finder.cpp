// font_finder.cpp
// Copyright (c) 2022, zhiayang
// SPDX-License-Identifier: Apache-2.0

#include "pdf/font.h"

#include "font/handle.h"
#include "font/search.h"
#include "font/font_file.h"

#include "interp/interp.h"
#include "interp/builtin_fns.h"
#include "interp/builtin_types.h"

namespace sap::interp::builtin
{
	static ErrorOr<pdf::PdfFont*> find_font(Evaluator* ev,
	    const std::vector<std::string>& font_names,
	    int64_t weight,
	    bool is_italic,
	    double stretch)
	{
		static util::hashmap<font::FontHandle, pdf::PdfFont*> s_cached_handles {};

		auto maybe_handle = font::findFont(font_names,
		    font::FontProperties {
		        .style = is_italic ? font::FontStyle::ITALIC : font::FontStyle::NORMAL,
		        .weight = static_cast<font::FontWeight>(weight),
		        .stretch = stretch,
		    });

		if(not maybe_handle.has_value())
			return ErrMsg(ev, "failed to find font matching given properties");

		if(auto it = s_cached_handles.find(*maybe_handle); it != s_cached_handles.end())
			return Ok(it->second);

		auto font_file = font::FontFile::fromHandle(*maybe_handle);
		if(not font_file.has_value())
			return ErrMsg(ev, "failed to load font from handle");

		auto pdf_font = pdf::PdfFont::fromSource(std::move(*font_file));
		auto font_ptr = &ev->interpreter()->addLoadedFont(std::move(pdf_font));

		util::log("loaded font {}", maybe_handle->postscript_name);
		return Ok(font_ptr);
	}



	ErrorOr<EvalResult> find_font(Evaluator* ev, std::vector<Value>& args)
	{
		assert(args.size() == 4);

		auto names_ = std::move(args[0]).takeArray();
		std::vector<std::string> font_names {};
		font_names.reserve(names_.size());

		for(auto& name : names_)
			font_names.push_back(name.getUtf8String());

		auto weight = unwrap_optional<int64_t>(std::move(args[1]), static_cast<int64_t>(font::FontWeight::NORMAL));
		auto is_italic = unwrap_optional<bool>(std::move(args[2]), false);
		auto stretch = unwrap_optional<double>(std::move(args[3]), font::FontStretch::NORMAL);

		auto pdf_font = TRY(find_font(ev, font_names, weight, is_italic, stretch));
		return EvalResult::ofValue(Value::optional(BS_Font::type, BS_Font::make(ev, pdf_font)));
	}


	ErrorOr<EvalResult> find_font_family(Evaluator* ev, std::vector<Value>& args)
	{
		assert(args.size() == 1);

		auto names_ = std::move(args[0]).takeArray();
		std::vector<std::string> font_names {};
		font_names.reserve(names_.size());

		for(auto& name : names_)
			font_names.push_back(name.getUtf8String());

		auto regular_font = TRY(find_font(ev, font_names, static_cast<int64_t>(font::FontWeight::NORMAL),
		    /* italic: */ false, font::FontStretch::NORMAL));

		auto italic_font = TRY(find_font(ev, font_names, static_cast<int64_t>(font::FontWeight::NORMAL),
		    /* italic: */ true, font::FontStretch::NORMAL));

		auto bold_font = TRY(find_font(ev, font_names, static_cast<int64_t>(font::FontWeight::BOLD),
		    /* italic: */ false, font::FontStretch::NORMAL));

		auto bold_italic_font = TRY(find_font(ev, font_names, static_cast<int64_t>(font::FontWeight::BOLD),
		    /* italic: */ true, font::FontStretch::NORMAL));

		auto fam = sap::FontFamily(regular_font, italic_font, bold_font, bold_italic_font);

		return EvalResult::ofValue(Value::optional(BS_FontFamily::type, BS_FontFamily::make(ev, std::move(fam))));
	}
}
