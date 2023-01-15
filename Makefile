# Makefile
# Copyright (c) 2021, zhiayang
# Licensed under the Apache License Version 2.0.

WARNINGS        = -Wno-padded -Wno-cast-align -Wno-unreachable-code -Wno-packed -Wno-missing-noreturn -Wno-float-equal -Wno-unused-macros -Wextra -Wconversion -Wpedantic -Wall -Wno-unused-parameter -Wno-trigraphs
WARNINGS += -Werror
WARNINGS += -Wno-error=unused-parameter
WARNINGS += -Wno-error=unused-variable
WARNINGS += -Wno-error=unused-function
WARNINGS += -Wno-unused-but-set-variable

OPT_FLAGS           := -O0 -fsanitize=address
LINKER_OPT_FLAGS    :=
COMMON_CFLAGS       := -g $(OPT_FLAGS)

OUTPUT_DIR          := build
TEST_DIR            := $(OUTPUT_DIR)/test

CC                  := clang
CXX                 := clang++

CFLAGS              = $(COMMON_CFLAGS) -std=c99 -fPIC -O3
CXXFLAGS            = $(COMMON_CFLAGS) -Wno-old-style-cast -std=c++20 -fno-exceptions

CXXSRC              := $(shell find source -iname "*.cpp" -print)
CXXOBJ              := $(CXXSRC:%.cpp=$(OUTPUT_DIR)/%.cpp.o)
CXXLIBOBJ           := $(filter-out $(OUTPUT_DIR)/source/main.cpp.o,$(CXXOBJ))
CXXDEPS             := $(CXXOBJ:.o=.d)

CXXHDR              := $(shell find source -iname "*.h" -print)

SPECIAL_HEADERS     := $(addprefix source/include/,defs.h pool.h util.h units.h error.h types.h)
SPECIAL_HDRS_COMPDB := $(SPECIAL_HEADERS:%=%.special_compile_db)

CXX_COMPDB_HDRS     := $(filter-out $(SPECIAL_HEADERS),$(CXXHDR))
CXX_COMPDB          := $(CXX_COMPDB_HDRS:%=%.compile_db) $(CXXSRC:%=%.compile_db)

TESTSRC             = $(shell find test -iname "*.cpp" -print)
TESTOBJ             = $(TESTSRC:%.cpp=$(OUTPUT_DIR)/%.cpp.o)
TESTDEPS            = $(TESTOBJ:.o=.d)
TESTS               = $(TESTSRC:test/%.cpp=$(TEST_DIR)/%)

UTF8PROC_SRCS       := external/utf8proc/utf8proc.c
UTF8PROC_OBJS       := $(UTF8PROC_SRCS:%.c=$(OUTPUT_DIR)/%.c.o)

MINIZ_SRCS          := external/miniz/miniz.c
MINIZ_OBJS          := $(MINIZ_SRCS:%.c=$(OUTPUT_DIR)/%.c.o)

PRECOMP_HDR         := source/include/precompile.h
PRECOMP_GCH         := $(PRECOMP_HDR:%.h=$(OUTPUT_DIR)/%.h.gch)
PRECOMP_INCLUDE     := $(PRECOMP_HDR:%.h=$(OUTPUT_DIR)/%.h)
PRECOMP_OBJ         := $(PRECOMP_HDR:%.h=$(OUTPUT_DIR)/%.h.gch.o)

CLANG_PCH_FASTER    :=
PCH_INCLUDE_FLAGS   := -include $(PRECOMP_INCLUDE)


DEFINES             :=
INCLUDES            := -Isource/include -Iexternal

UNAME_IDENT := $(shell uname)
ifeq ("$(UNAME_IDENT)", "Linux")
	USE_FONTCONFIG := 1
endif

ifeq ("$(UNAME_IDENT)", "Darwin")
	USE_CORETEXT := 1
endif

ifeq ($(USE_FONTCONFIG), 1)
	DEFINES  += -DUSE_FONTCONFIG=1
	NONGCH_CXXFLAGS += $(shell pkg-config --cflags fontconfig)
	LDFLAGS  += $(shell pkg-config --libs fontconfig)
endif

ifeq ($(USE_CORETEXT), 1)
	DEFINES  += -DUSE_CORETEXT=1
	LDFLAGS  += -framework Foundation -framework CoreText
endif

OUTPUT_BIN      := $(OUTPUT_DIR)/sap

.PHONY: all clean build test format iwyu %.pdf.gdb %.pdf.lldb
.PRECIOUS: $(PRECOMP_GCH) $(OUTPUT_DIR)/%.cpp.o
.DEFAULT_GOAL = all

all: build test

build: $(OUTPUT_BIN)

compdb: $(CXX_COMPDB) $(SPECIAL_HDRS_COMPDB)

test: $(TESTS)

%.pdf: %.sap build
	@env MallocNanoZone=0 $(OUTPUT_BIN) $<

%.pdf.gdb: %.sap build
	@gdb --args $(OUTPUT_BIN) $<

%.pdf.lldb: %.sap build
	@lldb --args $(OUTPUT_BIN) $<

check: test
	@for test in $(TESTS); do \
		echo Running test $$test; \
		$$test; \
	done

$(OUTPUT_BIN): $(PRECOMP_OBJ) $(CXXOBJ) $(UTF8PROC_OBJS) $(MINIZ_OBJS)
	@echo "  $(notdir $@)"
	@mkdir -p $(shell dirname $@)
	@$(CXX) $(CXXFLAGS) $(WARNINGS) $(DEFINES) $(LDFLAGS) $(LINKER_OPT_FLAGS) -Iexternal -o $@ $^

$(TEST_DIR)/%: $(OUTPUT_DIR)/test/%.cpp.o $(CXXLIBOBJ) $(UTF8PROC_OBJS) $(MINIZ_OBJS) $(PRECOMP_OBJ)
	@echo "  $(notdir $@)"
	@mkdir -p $(shell dirname $@)
	@$(CXX) $(CXXFLAGS) $(NONGCH_CXXFLAGS) $(WARNINGS) $(DEFINES) $(LDFLAGS) -Iexternal -o $@ $^

$(OUTPUT_DIR)/%.cpp.o: %.cpp $(PRECOMP_GCH)
	@echo "  $<"
	@mkdir -p $(shell dirname $@)
	@$(CXX) $(PCH_INCLUDE_FLAGS) $(CXXFLAGS) $(NONGCH_CXXFLAGS) $(WARNINGS) $(INCLUDES) $(DEFINES) -MMD -MP -c -o $@ $<

$(OUTPUT_DIR)/%.c.o: %.c
	@echo "  $<"
	@mkdir -p $(shell dirname $@)
	@$(CC) $(CFLAGS) -MMD -MP -c -o $@ $<

$(PRECOMP_GCH): $(PRECOMP_HDR)
	@printf "# precompiling header $<\n"
	@mkdir -p $(shell dirname $@)
	@$(CXX) $(CXXFLAGS) $(NONGCH_CXXFLAGS) $(WARNINGS) $(INCLUDES) $(CLANG_PCH_FASTER) -MMD -MP -x c++-header -o $@ $<
	@cp $< $(@:%.gch=%)

$(PRECOMP_OBJ): $(PRECOMP_GCH)
	@printf "# compiling pch\n"
	@mkdir -p $(shell dirname $@)
	@$(CXX) $(CXXFLAGS) $(WARNINGS) -c -o $@ $<

%.h.compile_db: %.h
	@$(CXX) $(CXXFLAGS) $(WARNINGS) $(INCLUDES) -include $(PRECOMP_HDR) -x c++-header -o /dev/null $<

%.h.special_compile_db: %.h
	@$(CXX) $(CXXFLAGS) $(WARNINGS) $(INCLUDES) -include $(PRECOMP_HDR) -x c++-header -o /dev/null $<

%.cpp.compile_db: %.cpp
	@$(CXX) $(CXXFLAGS) $(WARNINGS) $(INCLUDES) -include $(PRECOMP_HDR) -o /dev/null $<




clean:
	-@rm -fr $(OUTPUT_DIR)

format:
	find source -iname '*.cpp' -or -iname '*.h' | xargs -I{} -- ./tools/sort_includes.py -i {}
	clang-format -i $(shell find source -iname "*.cpp" -or -iname "*.h")

iwyu:
	iwyu-tool -j 8 -p . source -- -Xiwyu --update_comments -Xiwyu --no_fwd_decls -Xiwyu --prefix_header_includes=keep | iwyu-fix-includes --comments --update_comments
	find source -iname '*.cpp' -or -iname '*.h' | xargs -I{} -- ./tools/sort_includes.py -i {}
	clang-format -i $(shell find source -iname "*.cpp" -or -iname "*.h")

-include $(CXXDEPS)
-include $(TESTDEPS)
-include $(CDEPS)
-include $(PRECOMP_GCH:%.gch=%.d)










