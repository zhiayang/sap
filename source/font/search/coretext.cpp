// coretext.cpp
// Copyright (c) 2022, zhiayang
// SPDX-License-Identifier: Apache-2.0

#if defined(USE_CORETEXT)

#include <cmath>
#include <cstring>
#include <utility>

#include "defs.h"
#include "util.h"
#include "font/search.h"

#include <CoreText/CoreText.h>
#include <CoreFoundation/CoreFoundation.h>

namespace font::coretext
{
	using sap::Ok;
	using sap::Err;
	using sap::ErrFmt;
	using sap::ErrorOr;

	struct dont_retain_again
	{
	};

	template <typename T>
	requires(std::is_pointer_v<T>) struct CFWrapper
	{
		explicit CFWrapper(T ptr) : m_ptr(ptr) { CFRetain(m_ptr); }
		explicit CFWrapper(T ptr, dont_retain_again _) : m_ptr(ptr) { }

		~CFWrapper() { CFRelease(m_ptr); }

		CFWrapper(const CFWrapper& cfw) : m_ptr(cfw.m_ptr) { CFRetain(m_ptr); }
		CFWrapper& operator=(const CFWrapper& cfw)
		{
			if(this == &cfw)
				return *this;

			CFRelease(m_ptr);
			m_ptr = cfw.m_ptr;
			CFRetain(m_ptr);
			return *this;
		}

		operator T() { return m_ptr; }
		operator const T() const { return m_ptr; }

		T& ptr() { return m_ptr; }
		T ptr() const { return m_ptr; }

	private:
		T m_ptr;
	};

	template <typename T>
	requires(std::is_pointer_v<T>) static CFWrapper<T> cf_retain(T x)
	{
		if(x == nullptr)
			abort();

		return CFWrapper<T>(x);
	}

	template <typename T>
	requires(std::is_pointer_v<T>) static CFWrapper<T> cf_wrap_retained(T x)
	{
		if(x == nullptr)
			abort();

		return CFWrapper<T>(x, dont_retain_again {});
	}

	template <typename T>
	static CFWrapper<T> cf_retain(const void* x)
	{
		if(x == nullptr)
			abort();

		return CFWrapper<T>(static_cast<T>(x));
	}

	template <typename T>
	static CFWrapper<T> cf_wrap_retained(const void* x)
	{
		if(x == nullptr)
			abort();

		return CFWrapper<T>(static_cast<T>(x), dont_retain_again {});
	}





	struct FontNames
	{
		std::string display_name;
		std::string postscript_name;
	};

	static CFWrapper<CFStringRef> string_to_cf(zst::str_view str)
	{
		return cf_wrap_retained(CFStringCreateWithBytes(CFAllocatorGetDefault(), str.bytes().data(),
		    static_cast<CFIndex>(str.size()), kCFStringEncodingUTF8, false));
	}

	static std::string cf_to_string(CFStringRef str)
	{
		char buf[1024] {};
		if(not CFStringGetCString(str, &buf[0], 1024, kCFStringEncodingUTF8))
			return "";

		return std::string((char*) buf);
	}

	static double cf_to_double(CFNumberRef num)
	{
		double ret = 0;
		CFNumberGetValue(num, kCFNumberDoubleType, &ret);
		return ret;
	}

	static uint32_t cf_to_u32(CFNumberRef num)
	{
		int32_t ret = 0;
		CFNumberGetValue(num, kCFNumberSInt32Type, &ret);
		return static_cast<uint32_t>(ret);
	}


	static std::vector<CFWrapper<CTFontDescriptorRef>> get_descriptors_from_collection( //
	    CFWrapper<CTFontCollectionRef> collection)
	{
		std::vector<CFWrapper<CTFontDescriptorRef>> ret {};

		auto tmp = CTFontCollectionCreateMatchingFontDescriptors(collection);
		if(tmp == nullptr)
			return {};

		auto cf_array = cf_retain(tmp);
		for(CFIndex i = 0; i < CFArrayGetCount(cf_array); i++)
			ret.push_back(cf_retain<CTFontDescriptorRef>(CFArrayGetValueAtIndex(cf_array, i)));

		return ret;
	}

	template <typename T>
	static ErrorOr<CFWrapper<T>> get_font_attribute(CTFontDescriptorRef font, CFStringRef attribute)
	{
		auto value = CTFontDescriptorCopyAttribute(font, attribute);
		if(value == nullptr)
			return ErrFmt("failed to get font attribute '{}'", cf_to_string(attribute));

		return Ok(cf_wrap_retained<T>(value));
	}

	template <typename T>
	static ErrorOr<CFWrapper<T>> get_font_trait(CTFontDescriptorRef font, CFStringRef trait)
	{
		auto traits = TRY(get_font_attribute<CFDictionaryRef>(font, kCTFontTraitsAttribute));

		T trait_value {};
		if(not CFDictionaryGetValueIfPresent(traits, trait, reinterpret_cast<const void**>(&trait_value)))
			return ErrFmt("font does not have trait '{}'", cf_to_string(trait));

		return Ok(cf_retain(trait_value));
	}

	static ErrorOr<std::filesystem::path> get_font_path(CTFontDescriptorRef font)
	{
		auto url = TRY(get_font_attribute<CFURLRef>(font, kCTFontURLAttribute));

		char buf[1024] {};
		auto result = CFURLGetFileSystemRepresentation(url, true, (uint8_t*) &buf[0], 1024);
		if(not result)
			return ErrFmt("failed to convert path");

		return Ok(std::filesystem::path(&buf[0], &buf[strlen(buf)]));
	}

	static ErrorOr<FontHandle> get_handle_from_descriptor(CTFontDescriptorRef font)
	{
		auto path = TRY(get_font_path(font));
		auto display_name = cf_to_string(TRY(get_font_attribute<CFStringRef>(font, kCTFontDisplayNameAttribute)));
		auto postscript_name = cf_to_string(TRY(get_font_attribute<CFStringRef>(font, kCTFontNameAttribute)));

		int font_weight = static_cast<int>(TRY([font]() -> ErrorOr<double> {
			auto ct_weight = cf_to_double(TRY(get_font_trait<CFNumberRef>(font, kCTFontWeightTrait)));
			/*
			    -0.7    -> 100
			    -0.5    -> 200
			    -0.23   -> 300
			    0.0     -> 400
			    0.2     -> 500
			    0.3     -> 600
			    0.4     -> 700
			    0.6     -> 800
			    0.8     -> 900
			*/
			if(ct_weight <= -0.7)
				return Ok(0 + 100 * (1.0 + ct_weight) / 0.3);
			else if(ct_weight <= -0.5)
				return Ok(100 + 100 * (1.0 + ct_weight) / 0.5);
			else if(ct_weight <= -0.23)
				return Ok(200 + 100 * (1.0 + ct_weight) / 0.77);
			else if(ct_weight <= 0.00)
				return Ok(300 + 100 * (1.0 + ct_weight) / 1.0);
			else if(ct_weight <= 0.20)
				return Ok(400 + 100 * ct_weight / 0.2);
			else if(ct_weight <= 0.30)
				return Ok(500 + 100 * ct_weight / 0.3);
			else if(ct_weight <= 0.40)
				return Ok(600 + 100 * ct_weight / 0.4);
			else if(ct_weight <= 0.60)
				return Ok(700 + 100 * ct_weight / 0.6);
			else if(ct_weight <= 0.80)
				return Ok(800 + 100 * ct_weight / 0.8);
			else
				return Ok(900 + 100 * ct_weight);
		}()));

		auto misc_traits = cf_to_u32(TRY(get_font_trait<CFNumberRef>(font, kCTFontSymbolicTrait)));
		auto font_style = (misc_traits & kCTFontTraitItalic) //
		                    ? FontStyle::ITALIC
		                    : FontStyle::NORMAL;

		// coretext gives -1 to +1, with 0 being normal; css wants 0.5 to 2.0
		auto cf_width = cf_to_double(TRY(get_font_trait<CFNumberRef>(font, kCTFontWidthTrait)));
		auto font_stretch = [](double cf_width) {
			// convert the cf width into an index corresponding to the 9 css stretches
			// (ULTRA_CONDENSED to ULTRA_EXPANDED)
			double lookup[] = {
				FontStretch::ULTRA_CONDENSED,
				FontStretch::EXTRA_CONDENSED,
				FontStretch::CONDENSED,
				FontStretch::SEMI_CONDENSED,
				FontStretch::NORMAL,
				FontStretch::SEMI_EXPANDED,
				FontStretch::EXPANDED,
				FontStretch::EXTRA_EXPANDED,
				FontStretch::ULTRA_EXPANDED,
			};
			cf_width = std::max(-1.0, std::min(cf_width, +1.0));
			auto idx = 4 * (cf_width + 1.0);
			auto lower = static_cast<size_t>(floor(idx));
			auto upper = static_cast<size_t>(ceil(idx));
			auto frac = idx - (int) idx;

			return lookup[lower] + frac * (lookup[upper] - lookup[lower]);
		}(cf_width);

		return Ok(FontHandle {
		    .display_name = std::move(display_name),
		    .postscript_name = std::move(postscript_name),
		    .properties = {
		    	.style = font_style,
		    	.weight = static_cast<FontWeight>(font_weight),
		    	.stretch = font_stretch,
		    },
		    .path = std::move(path),
		});
	}

	static CFWrapper<CFDictionaryRef> get_remove_dupes_option_dict()
	{
		auto alloc = CFWrapper(CFAllocatorGetDefault());
		auto cf_key = CFWrapper(kCTFontCollectionRemoveDuplicatesOption);

		int val = 1;
		auto cf_val = CFWrapper(CFNumberCreate(alloc, kCFNumberIntType, &val));
		auto options_dict = cf_wrap_retained(CFDictionaryCreate(alloc, //
		    (const void**) &cf_key.ptr(),                              //
		    (const void**) &cf_val.ptr(),                              //
		    1, &kCFCopyStringDictionaryKeyCallBacks, &kCFTypeDictionaryValueCallBacks));

		return options_dict;
	}

	static std::vector<FontHandle> get_handles_matching_search(zst::str_view search_key, zst::str_view search_value)
	{
		auto alloc = CFWrapper(CFAllocatorGetDefault());

		auto cf_key = string_to_cf(search_key);
		auto cf_val = string_to_cf(search_value);

		auto names_dict = CFWrapper(CFDictionaryCreate(alloc, //
		    (const void**) &cf_key.ptr(),                     //
		    (const void**) &cf_val.ptr(),                     //
		    1, &kCFCopyStringDictionaryKeyCallBacks, &kCFTypeDictionaryValueCallBacks));

		auto descriptor = cf_wrap_retained(CTFontDescriptorCreateWithAttributes(names_dict));
		auto cf_descriptor_array = cf_wrap_retained(CFArrayCreate(alloc, (const void**) &descriptor.ptr(), 1,
		    &kCFTypeArrayCallBacks));

		auto options_dict = get_remove_dupes_option_dict();
		auto collection = cf_wrap_retained(CTFontCollectionCreateWithFontDescriptors(cf_descriptor_array, options_dict));
		auto found_descs = get_descriptors_from_collection(collection);

		std::vector<FontHandle> handles {};
		for(auto& desc : found_descs)
		{
			if(auto e = get_handle_from_descriptor(desc); e.is_err())
				sap::warn("CoreText", "failed to get handle: {}", e.take_error());
			else
				handles.push_back(e.take_value());
		}

		return handles;
	}


	std::vector<FontHandle> get_all_fonts()
	{
		auto options_dict = get_remove_dupes_option_dict();
		auto descriptors = get_descriptors_from_collection(
		    cf_wrap_retained(CTFontCollectionCreateFromAvailableFonts(options_dict)));

		std::vector<FontHandle> handles {};
		for(auto& desc : descriptors)
		{
			if(auto handle = get_handle_from_descriptor(desc); handle.ok())
				handles.push_back(handle.take_value());
		}

		return handles;
	}

	std::vector<std::string> get_all_typefaces()
	{
		auto tmp = CTFontManagerCopyAvailableFontFamilyNames();
		if(tmp == nullptr)
			return {};

		std::vector<std::string> family_names {};

		auto cf_array = cf_wrap_retained<CFArrayRef>(tmp);
		for(CFIndex i = 0; i < CFArrayGetCount(cf_array); i++)
			family_names.push_back(cf_to_string(static_cast<CFStringRef>(CFArrayGetValueAtIndex(cf_array, i))));

		return family_names;
	}

	std::optional<FontHandle> search_for_font_with_postscript_name(zst::str_view postscript_name)
	{
		auto handles = get_handles_matching_search("NSFontNameAttribute", postscript_name);

		if(handles.empty())
			return std::nullopt;

		else if(handles.size() > 1)
			sap::warn("CoreText", "postscript name '{}' matched {} fonts! returning first one", postscript_name, handles.size());

		return handles[0];
	}


	std::optional<FontHandle> search_for_font(zst::str_view typeface_name, FontProperties properties)
	{
		auto handles = get_handles_matching_search("NSFontFamilyAttribute", typeface_name);
		if(handles.empty())
			return std::nullopt;

		return getBestFontWithProperties(std::move(handles), std::move(properties));
	}

	std::optional<FontHandle> get_font_for_generic_name(zst::str_view name, FontProperties properties)
	{
		if(name == GENERIC_SERIF)
			return search_for_font("Times New Roman", properties);
		else if(name == GENERIC_SANS_SERIF)
			return search_for_font("Helvetica", properties);
		else if(name == GENERIC_MONOSPACE)
			return search_for_font("Courier New", properties);
		else if(name == GENERIC_EMOJI)
			return search_for_font("Apple Color Emoji", properties);
		else
			return std::nullopt;
	}
}

#endif
