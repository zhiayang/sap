cmake_minimum_required(VERSION 3.18)
project(sap)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED True)

set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

add_executable(sap)
target_include_directories(sap PRIVATE
	source/
	external/
)

target_precompile_headers(sap PRIVATE source/precompile.h)

file(COPY_FILE
	"${CMAKE_BINARY_DIR}/compile_commands.json"
	"${CMAKE_SOURCE_DIR}/compile_commands.json"
	RESULT _copy_success
)

if(MSVC)
	target_compile_options(sap PRIVATE
		/W4
		/wd4141
		/wd4100
		/wd4458
		/Zc:__cplusplus
	)
else()
	target_compile_options(sap PRIVATE
		-Wall -Wextra
		-Werror
		-Wno-error=unused-parameter
		-Wno-error=unused-variable
		-Wno-error=unused-function
		-Wno-unused-but-set-variable
		-Wshadow
		-Wno-error=shadow
		-Wno-padded
		-Wno-cast-align
		-Wno-unreachable-code
		-Wno-packed
		-Wno-missing-noreturn
		-Wno-float-equal
		-Wno-unused-macros
		-Wconversion
		-Wpedantic
		-Wno-unused-parameter
		-Wno-trigraphs
	)

	if(CMAKE_CXX_COMPILER_ID STREQUAL "Clang")
		target_compile_options(sap PRIVATE
			-Wno-missing-field-initializers
			-Wno-string-conversion
		)
		if(WIN32)
			target_compile_options(sap PRIVATE
				-Wno-microsoft-include
			)
		endif()
	elseif(CMAKE_CXX_COMPILER_ID STREQUAL "GNU")
		target_compile_options(sap PRIVATE
			-Wno-missing-field-initializers
			-Wno-shadow # broken on gcc, warns on everything
		)
	endif()
endif()


# add external sources
add_library(external_libs STATIC
	external/libdeflate/lib/arm/cpu_features.c
	external/libdeflate/lib/x86/cpu_features.c
	external/libdeflate/lib/adler32.c
	external/libdeflate/lib/crc32.c
	external/libdeflate/lib/deflate_compress.c
	external/libdeflate/lib/deflate_decompress.c
	external/libdeflate/lib/gzip_compress.c
	external/libdeflate/lib/gzip_decompress.c
	external/libdeflate/lib/utils.c
	external/libdeflate/lib/zlib_compress.c
	external/libdeflate/lib/zlib_decompress.c

	external/utf8proc/utf8proc.c
	external/stb_image.c
	external/xxhash.c
)

if(WIN32)
	# include_directories(SYSTEM "$ENV{INCLUDE}")
	target_compile_definitions(sap PRIVATE
		"_CRT_SECURE_NO_WARNINGS"
		"UTF8PROC_STATIC"
	)

	target_compile_definitions(external_libs PRIVATE "UTF8PROC_STATIC")
endif()


target_compile_definitions(sap PRIVATE "SAP_PREFIX=\"${CMAKE_CURRENT_SOURCE_DIR}\"")

target_link_libraries(sap PRIVATE external_libs)

target_sources(sap PRIVATE
	source/main.cpp
	source/compile.cpp

	source/watch/watch.cpp

	source/tree/base.cpp
	source/tree/container.cpp
	source/tree/image.cpp
	source/tree/paragraph.cpp
	source/tree/path.cpp
	source/tree/raw.cpp
	source/tree/scripts.cpp
	source/tree/separators.cpp
	source/tree/spacer.cpp
	source/tree/wrappers.cpp

	source/pdf/annotation.cpp
	source/pdf/file.cpp
	source/pdf/graphics.cpp
	source/pdf/image.cpp
	source/pdf/indirect.cpp
	source/pdf/object.cpp
	source/pdf/outlines.cpp
	source/pdf/page.cpp
	source/pdf/path.cpp
	source/pdf/resource.cpp
	source/pdf/stream.cpp
	source/pdf/text.cpp
	source/pdf/xobject.cpp

	source/pdf/font/builtin_font.cpp
	source/pdf/font/cidset.cpp
	source/pdf/font/cmap.cpp
	source/pdf/font/glyph_names.cpp
	source/pdf/font/misc.cpp
	source/pdf/font/pdf_font.cpp
	source/pdf/font/serialise.cpp

	source/pdf/util/misc.cpp
	source/pdf/util/writer.cpp

	source/misc/dyn_length.cpp
	source/misc/error.cpp
	source/misc/hyph.cpp
	source/misc/path_segment.cpp
	source/misc/paths.cpp
	source/misc/unicode.cpp
	source/misc/util.cpp

	source/layout/container.cpp
	source/layout/cursor.cpp
	source/layout/default_settings.cpp
	source/layout/document.cpp
	source/layout/image.cpp
	source/layout/layout_object.cpp
	source/layout/line.cpp
	source/layout/linebreak.cpp
	source/layout/page_layout.cpp
	source/layout/paragraph.cpp
	source/layout/path.cpp
	source/layout/spacer.cpp
	source/layout/style.cpp
	source/layout/word.cpp

	source/interp/builtins.cpp
	source/interp/defn_tree.cpp
	source/interp/destructors.cpp
	source/interp/evaluator.cpp
	source/interp/interp.cpp
	source/interp/overload_resolution.cpp
	source/interp/polymorph.cpp
	source/interp/typechecker.cpp
	source/interp/value.cpp

	source/interp/ast/assign_op.cpp
	source/interp/ast/binary_op.cpp
	source/interp/ast/block.cpp
	source/interp/ast/call.cpp
	source/interp/ast/cast.cpp
	source/interp/ast/compare_op.cpp
	source/interp/ast/deref.cpp
	source/interp/ast/dotop.cpp
	source/interp/ast/enum.cpp
	source/interp/ast/for.cpp
	source/interp/ast/fstring.cpp
	source/interp/ast/func.cpp
	source/interp/ast/hook.cpp
	source/interp/ast/ident.cpp
	source/interp/ast/if.cpp
	source/interp/ast/if_let.cpp
	source/interp/ast/import.cpp
	source/interp/ast/literal.cpp
	source/interp/ast/logical_op.cpp
	source/interp/ast/misc.cpp
	source/interp/ast/move.cpp
	source/interp/ast/optional.cpp
	source/interp/ast/return.cpp
	source/interp/ast/struct.cpp
	source/interp/ast/struct_literal.cpp
	source/interp/ast/subscript_op.cpp
	source/interp/ast/tree_expr.cpp
	source/interp/ast/type_expr.cpp
	source/interp/ast/unary_op.cpp
	source/interp/ast/union.cpp
	source/interp/ast/using.cpp
	source/interp/ast/variable.cpp
	source/interp/ast/while.cpp

	source/interp/builtins/be_alignment.cpp
	source/interp/builtins/bs_border_style.cpp
	source/interp/builtins/bs_colour.cpp
	source/interp/builtins/bs_doc_settings.cpp
	source/interp/builtins/bs_document_proxy.cpp
	source/interp/builtins/bs_font.cpp
	source/interp/builtins/bs_glyph_adjustment.cpp
	source/interp/builtins/bs_link_annotation.cpp
	source/interp/builtins/bs_outline_item.cpp
	source/interp/builtins/bs_path_style.cpp
	source/interp/builtins/bs_pos2d.cpp
	source/interp/builtins/bs_position.cpp
	source/interp/builtins/bs_size2d.cpp
	source/interp/builtins/bs_state.cpp
	source/interp/builtins/bs_style.cpp
	source/interp/builtins/bu_path_segment.cpp
	source/interp/builtins/font_finder.cpp
	source/interp/builtins/include.cpp
	source/interp/builtins/object_refs.cpp
	source/interp/builtins/positioning.cpp
	source/interp/builtins/print.cpp
	source/interp/builtins/start.cpp
	source/interp/builtins/struct_maker.cpp
	source/interp/builtins/styling.cpp
	source/interp/builtins/tree_manip.cpp

	source/interp/cst/assign_op.cpp
	source/interp/cst/binary_op.cpp
	source/interp/cst/block.cpp
	source/interp/cst/call.cpp
	source/interp/cst/cast.cpp
	source/interp/cst/compare_op.cpp
	source/interp/cst/deref.cpp
	source/interp/cst/dotop.cpp
	source/interp/cst/enum.cpp
	source/interp/cst/for.cpp
	source/interp/cst/fstring.cpp
	source/interp/cst/func.cpp
	source/interp/cst/hook.cpp
	source/interp/cst/ident.cpp
	source/interp/cst/if.cpp
	source/interp/cst/if_let.cpp
	source/interp/cst/import.cpp
	source/interp/cst/literal.cpp
	source/interp/cst/logical_op.cpp
	source/interp/cst/misc.cpp
	source/interp/cst/move.cpp
	source/interp/cst/optional.cpp
	source/interp/cst/return.cpp
	source/interp/cst/struct.cpp
	source/interp/cst/subscript_op.cpp
	source/interp/cst/tree_expr.cpp
	source/interp/cst/unary_op.cpp
	source/interp/cst/union.cpp
	source/interp/cst/variable.cpp
	source/interp/cst/while.cpp

	source/interp/types/array.cpp
	source/interp/types/enum.cpp
	source/interp/types/function.cpp
	source/interp/types/optional.cpp
	source/interp/types/pointer.cpp
	source/interp/types/struct.cpp
	source/interp/types/type.cpp
	source/interp/types/union.cpp

	source/frontend/lexer.cpp
	source/frontend/parser.cpp
	source/frontend/parser_type.cpp

	source/font/cmap.cpp
	source/font/font_file.cpp
	source/font/loader.cpp
	source/font/mac_encodings.cpp
	source/font/metrics.cpp
	source/font/subset.cpp

	source/font/cff/cff.cpp
	source/font/cff/cff_builder.cpp
	source/font/cff/charset.cpp
	source/font/cff/charstring.cpp
	source/font/cff/dict.cpp
	source/font/cff/operands.cpp
	source/font/cff/strings.cpp
	source/font/cff/subset.cpp

	source/font/search/coretext.cpp
	source/font/search/fontconfig.cpp
	source/font/search/search.cpp

	source/font/tables/aat/feature.cpp
	source/font/tables/aat/kern.cpp
	source/font/tables/aat/lookup_table.cpp
	source/font/tables/aat/morx.cpp
	source/font/tables/aat/state_table.cpp

	source/font/tables/off/common.cpp
	source/font/tables/off/context_lookup.cpp
	source/font/tables/off/coverage.cpp
	source/font/tables/off/gdef.cpp
	source/font/tables/off/gpos.cpp
	source/font/tables/off/gsub.cpp

	source/font/truetype/subset.cpp
	source/font/truetype/truetype.cpp

	source/configs/config_parser.cpp
)
