// charset.cpp
// Copyright (c) 2022, yuki / zhiayang
// SPDX-License-Identifier: Apache-2.0

#include "font/cff.h" // for getPredefinedCharset, readCharsetTable
#include "font/misc.h"

namespace font::cff
{
	std::map<uint16_t, uint16_t> readCharsetTable(size_t num_glyphs, zst::byte_span buffer)
	{
		auto format = buffer[0];
		buffer.remove_prefix(1);

		std::map<uint16_t, uint16_t> mapping {};

		if(format == 0)
		{
			// sids... and num_glyphs - 1 sids...
			for(uint16_t i = 1; i < num_glyphs; i++)
				mapping[i] = consume_u16(buffer);
		}
		else if(format == 1 || format == 2)
		{
			// fuck you, adobe. "The number of ranges is not explicitly specified in the font.
			// Instead, software utilising this data simply processes ranges until all glyphs
			// in the font are covered". stupid design, absolutely garbage.

			uint16_t gid = 1;
			while(gid < num_glyphs)
			{
				// read the sid.
				auto sid = consume_u16(buffer);

				// read the nLeft (1 byte if format == 1, 2 if format == 2)
				size_t num_sids = 1;
				if(format == 1)
					num_sids += consume_u8(buffer);
				else
					num_sids += consume_u16(buffer);

				for(uint16_t k = 0; k < num_sids + 1; k++)
					mapping[gid + k] = sid + k;

				gid += checked_cast<uint16_t>(num_sids + 1);
			}
		}
		else
		{
			sap::error("font/cff", "unsupported charset format '{}' (expected 0, 1, or 2)", format);
		}

		// regardless, gid 0 = cid = 0 = .notdef
		mapping[0] = 0;

		return mapping;
	}


	static std::map<uint16_t, uint16_t> g_ISOAdobeCharset = { { 0, 0 }, { 1, 1 }, { 2, 2 }, { 3, 3 }, { 4, 4 }, { 5, 5 },
		{ 6, 6 }, { 7, 7 }, { 8, 8 }, { 9, 9 }, { 10, 10 }, { 11, 11 }, { 12, 12 }, { 13, 13 }, { 14, 14 }, { 15, 15 },
		{ 16, 16 }, { 17, 17 }, { 18, 18 }, { 19, 19 }, { 20, 20 }, { 21, 21 }, { 22, 22 }, { 23, 23 }, { 24, 24 }, { 25, 25 },
		{ 26, 26 }, { 27, 27 }, { 28, 28 }, { 29, 29 }, { 30, 30 }, { 31, 31 }, { 32, 32 }, { 33, 33 }, { 34, 34 }, { 35, 35 },
		{ 36, 36 }, { 37, 37 }, { 38, 38 }, { 39, 39 }, { 40, 40 }, { 41, 41 }, { 42, 42 }, { 43, 43 }, { 44, 44 }, { 45, 45 },
		{ 46, 46 }, { 47, 47 }, { 48, 48 }, { 49, 49 }, { 50, 50 }, { 51, 51 }, { 52, 52 }, { 53, 53 }, { 54, 54 }, { 55, 55 },
		{ 56, 56 }, { 57, 57 }, { 58, 58 }, { 59, 59 }, { 60, 60 }, { 61, 61 }, { 62, 62 }, { 63, 63 }, { 64, 64 }, { 65, 65 },
		{ 66, 66 }, { 67, 67 }, { 68, 68 }, { 69, 69 }, { 70, 70 }, { 71, 71 }, { 72, 72 }, { 73, 73 }, { 74, 74 }, { 75, 75 },
		{ 76, 76 }, { 77, 77 }, { 78, 78 }, { 79, 79 }, { 80, 80 }, { 81, 81 }, { 82, 82 }, { 83, 83 }, { 84, 84 }, { 85, 85 },
		{ 86, 86 }, { 87, 87 }, { 88, 88 }, { 89, 89 }, { 90, 90 }, { 91, 91 }, { 92, 92 }, { 93, 93 }, { 94, 94 }, { 95, 95 },
		{ 96, 96 }, { 97, 97 }, { 98, 98 }, { 99, 99 }, { 100, 100 }, { 101, 101 }, { 102, 102 }, { 103, 103 }, { 104, 104 },
		{ 105, 105 }, { 106, 106 }, { 107, 107 }, { 108, 108 }, { 109, 109 }, { 110, 110 }, { 111, 111 }, { 112, 112 },
		{ 113, 113 }, { 114, 114 }, { 115, 115 }, { 116, 116 }, { 117, 117 }, { 118, 118 }, { 119, 119 }, { 120, 120 },
		{ 121, 121 }, { 122, 122 }, { 123, 123 }, { 124, 124 }, { 125, 125 }, { 126, 126 }, { 127, 127 }, { 128, 128 },
		{ 129, 129 }, { 130, 130 }, { 131, 131 }, { 132, 132 }, { 133, 133 }, { 134, 134 }, { 135, 135 }, { 136, 136 },
		{ 137, 137 }, { 138, 138 }, { 139, 139 }, { 140, 140 }, { 141, 141 }, { 142, 142 }, { 143, 143 }, { 144, 144 },
		{ 145, 145 }, { 146, 146 }, { 147, 147 }, { 148, 148 }, { 149, 149 }, { 150, 150 }, { 151, 151 }, { 152, 152 },
		{ 153, 153 }, { 154, 154 }, { 155, 155 }, { 156, 156 }, { 157, 157 }, { 158, 158 }, { 159, 159 }, { 160, 160 },
		{ 161, 161 }, { 162, 162 }, { 163, 163 }, { 164, 164 }, { 165, 165 }, { 166, 166 }, { 167, 167 }, { 168, 168 },
		{ 169, 169 }, { 170, 170 }, { 171, 171 }, { 172, 172 }, { 173, 173 }, { 174, 174 }, { 175, 175 }, { 176, 176 },
		{ 177, 177 }, { 178, 178 }, { 179, 179 }, { 180, 180 }, { 181, 181 }, { 182, 182 }, { 183, 183 }, { 184, 184 },
		{ 185, 185 }, { 186, 186 }, { 187, 187 }, { 188, 188 }, { 189, 189 }, { 190, 190 }, { 191, 191 }, { 192, 192 },
		{ 193, 193 }, { 194, 194 }, { 195, 195 }, { 196, 196 }, { 197, 197 }, { 198, 198 }, { 199, 199 }, { 200, 200 },
		{ 201, 201 }, { 202, 202 }, { 203, 203 }, { 204, 204 }, { 205, 205 }, { 206, 206 }, { 207, 207 }, { 208, 208 },
		{ 209, 209 }, { 210, 210 }, { 211, 211 }, { 212, 212 }, { 213, 213 }, { 214, 214 }, { 215, 215 }, { 216, 216 },
		{ 217, 217 }, { 218, 218 }, { 219, 219 }, { 220, 220 }, { 221, 221 }, { 222, 222 }, { 223, 223 }, { 224, 224 },
		{ 225, 225 }, { 226, 226 }, { 227, 227 }, { 228, 228 } };

	static std::map<uint16_t, uint16_t> g_ExpertCharset = { { 0, 0 }, { 1, 1 }, { 2, 229 }, { 3, 230 }, { 4, 231 }, { 5, 232 },
		{ 6, 233 }, { 7, 234 }, { 8, 235 }, { 9, 236 }, { 10, 237 }, { 11, 238 }, { 12, 13 }, { 13, 14 }, { 14, 15 }, { 15, 99 },
		{ 16, 239 }, { 17, 240 }, { 18, 241 }, { 19, 242 }, { 20, 243 }, { 21, 244 }, { 22, 245 }, { 23, 246 }, { 24, 247 },
		{ 25, 248 }, { 26, 27 }, { 27, 28 }, { 28, 249 }, { 29, 250 }, { 30, 251 }, { 31, 252 }, { 32, 253 }, { 33, 254 },
		{ 34, 255 }, { 35, 256 }, { 36, 257 }, { 37, 258 }, { 38, 259 }, { 39, 260 }, { 40, 261 }, { 41, 262 }, { 42, 263 },
		{ 43, 264 }, { 44, 265 }, { 45, 266 }, { 46, 109 }, { 47, 110 }, { 48, 267 }, { 49, 268 }, { 50, 269 }, { 51, 270 },
		{ 52, 271 }, { 53, 272 }, { 54, 273 }, { 55, 274 }, { 56, 275 }, { 57, 276 }, { 58, 277 }, { 59, 278 }, { 60, 279 },
		{ 61, 280 }, { 62, 281 }, { 63, 282 }, { 64, 283 }, { 65, 284 }, { 66, 285 }, { 67, 286 }, { 68, 287 }, { 69, 288 },
		{ 70, 289 }, { 71, 290 }, { 72, 291 }, { 73, 292 }, { 74, 293 }, { 75, 294 }, { 76, 295 }, { 77, 296 }, { 78, 297 },
		{ 79, 298 }, { 80, 299 }, { 81, 300 }, { 82, 301 }, { 83, 302 }, { 84, 303 }, { 85, 304 }, { 86, 305 }, { 87, 306 },
		{ 88, 307 }, { 89, 308 }, { 90, 309 }, { 91, 310 }, { 92, 311 }, { 93, 312 }, { 94, 313 }, { 95, 314 }, { 96, 315 },
		{ 97, 316 }, { 98, 317 }, { 99, 318 }, { 100, 158 }, { 101, 155 }, { 102, 163 }, { 103, 319 }, { 104, 320 }, { 105, 321 },
		{ 106, 322 }, { 107, 323 }, { 108, 324 }, { 109, 325 }, { 110, 326 }, { 111, 150 }, { 112, 164 }, { 113, 169 },
		{ 114, 327 }, { 115, 328 }, { 116, 329 }, { 117, 330 }, { 118, 331 }, { 119, 332 }, { 120, 333 }, { 121, 334 },
		{ 122, 335 }, { 123, 336 }, { 124, 337 }, { 125, 338 }, { 126, 339 }, { 127, 340 }, { 128, 341 }, { 129, 342 },
		{ 130, 343 }, { 131, 344 }, { 132, 345 }, { 133, 346 }, { 134, 347 }, { 135, 348 }, { 136, 349 }, { 137, 350 },
		{ 138, 351 }, { 139, 352 }, { 140, 353 }, { 141, 354 }, { 142, 355 }, { 143, 356 }, { 144, 357 }, { 145, 358 },
		{ 146, 359 }, { 147, 360 }, { 148, 361 }, { 149, 362 }, { 150, 363 }, { 151, 364 }, { 152, 365 }, { 153, 366 },
		{ 154, 367 }, { 155, 368 }, { 156, 369 }, { 157, 370 }, { 158, 371 }, { 159, 372 }, { 160, 373 }, { 161, 374 },
		{ 162, 375 }, { 163, 376 }, { 164, 377 }, { 165, 378 } };

	static std::map<uint16_t, uint16_t> g_ExpertSubsetCharset = { { 0, 0 }, { 1, 1 }, { 2, 231 }, { 3, 232 }, { 4, 235 },
		{ 5, 236 }, { 6, 237 }, { 7, 238 }, { 8, 13 }, { 9, 14 }, { 10, 15 }, { 11, 99 }, { 12, 239 }, { 13, 240 }, { 14, 241 },
		{ 15, 242 }, { 16, 243 }, { 17, 244 }, { 18, 245 }, { 19, 246 }, { 20, 247 }, { 21, 248 }, { 22, 27 }, { 23, 28 },
		{ 24, 249 }, { 25, 250 }, { 26, 251 }, { 27, 253 }, { 28, 254 }, { 29, 255 }, { 30, 256 }, { 31, 257 }, { 32, 258 },
		{ 33, 259 }, { 34, 260 }, { 35, 261 }, { 36, 262 }, { 37, 263 }, { 38, 264 }, { 39, 265 }, { 40, 266 }, { 41, 109 },
		{ 42, 110 }, { 43, 267 }, { 44, 268 }, { 45, 269 }, { 46, 270 }, { 47, 272 }, { 48, 300 }, { 49, 301 }, { 50, 302 },
		{ 51, 305 }, { 52, 314 }, { 53, 315 }, { 54, 158 }, { 55, 155 }, { 56, 163 }, { 57, 320 }, { 58, 321 }, { 59, 322 },
		{ 60, 323 }, { 61, 324 }, { 62, 325 }, { 63, 326 }, { 64, 150 }, { 65, 164 }, { 66, 169 }, { 67, 327 }, { 68, 328 },
		{ 69, 329 }, { 70, 330 }, { 71, 331 }, { 72, 332 }, { 73, 333 }, { 74, 334 }, { 75, 335 }, { 76, 336 }, { 77, 337 },
		{ 78, 338 }, { 79, 339 }, { 80, 340 }, { 81, 341 }, { 82, 342 }, { 83, 343 }, { 84, 344 }, { 85, 345 }, { 86, 346 } };


	std::map<uint16_t, uint16_t> getPredefinedCharset(int num)
	{
		if(num == 0)
			return g_ISOAdobeCharset;
		else if(num == 1)
			return g_ExpertCharset;
		else if(num == 2)
			return g_ExpertSubsetCharset;
		else
			sap::error("font/cff", "invalid predefined charset '{}'", num);
	}
}
