# Makefile
# Copyright (c) 2021, zhiayang
# Licensed under the Apache License Version 2.0.

WARNINGS        = -Wno-padded -Wno-cast-align -Wno-unreachable-code -Wno-packed -Wno-missing-noreturn -Wno-float-equal -Wno-unused-macros -Wextra -Wconversion -Wpedantic -Wall -Wno-unused-parameter -Wno-trigraphs
WARNINGS += -Werror
WARNINGS += -Wno-error=unused-parameter
WARNINGS += -Wno-error=unused-variable
WARNINGS += -Wno-error=unused-function
WARNINGS += -Wno-unused-but-set-variable

COMMON_CFLAGS       = -O0 -g -fsanitize=address

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

CXXHDRS             := $(shell find source -iname "*.h" -print)
CXXGCHS             := $(CXXHDRS:%.h=$(OUTPUT_DIR)/%.h.gch)
CXXGCHS             := $(filter-out $(OUTPUT_DIR)/source/include/defs.h.gch,$(CXXGCHS))
CXXGCHS             := $(filter-out $(OUTPUT_DIR)/source/include/pool.h.gch,$(CXXGCHS))
CXXGCHS             := $(filter-out $(OUTPUT_DIR)/source/include/units.h.gch,$(CXXGCHS))
CXXGCHS             := $(filter-out $(OUTPUT_DIR)/source/include/error.h.gch,$(CXXGCHS))

TESTSRC             = $(shell find test -iname "*.cpp" -print)
TESTOBJ             = $(TESTSRC:%.cpp=$(OUTPUT_DIR)/%.cpp.o)
TESTDEPS            = $(TESTOBJ:.o=.d)
TESTS               = $(TESTSRC:test/%.cpp=$(TEST_DIR)/%)

UTF8PROC_SRCS       := external/utf8proc/utf8proc.c
UTF8PROC_OBJS       := $(UTF8PROC_SRCS:%.c=$(OUTPUT_DIR)/%.c.o)

MINIZ_SRCS          := external/miniz/miniz.c
MINIZ_OBJS          := $(MINIZ_SRCS:%.c=$(OUTPUT_DIR)/%.c.o)

PRECOMP_HDR         := source/include/precompile.h
OUTPUT_BIN          := $(OUTPUT_DIR)/sap

DEFINES             :=
INCLUDES            := -Isource/include -Iexternal


ifeq ("$(findstring clang,$(CXX))", "clang")
	CLANG_PCH_FASTER    := -fpch-instantiate-templates -fpch-codegen
	PCH_INCLUDE_FLAGS   := -include-pch
	PCH_OBJS            := $(CXXSRC:%.cpp=$(OUTPUT_DIR)/%.cpp.gch.o)
else
	CLANG_PCH_FASTER    :=
	PCH_INCLUDE_FLAGS   := -include
	PCH_OBJS            :=
endif

UNAME_IDENT := $(shell uname)
ifeq ("$(UNAME_IDENT)", "Linux")
	USE_FONTCONFIG := 1
endif

ifeq ("$(UNAME_IDENT)", "Darwin")
	USE_CORETEXT := 1
endif

ifeq ($(USE_FONTCONFIG), 1)
	DEFINES  += -DUSE_FONTCONFIG=1
	CXXFLAGS += $(shell pkg-config --cflags fontconfig)
	LDFLAGS  += $(shell pkg-config --libs fontconfig)
endif

ifeq ($(USE_CORETEXT), 1)
	DEFINES  += -DUSE_CORETEXT=1
	LDFLAGS  += -framework Foundation -framework CoreText
endif

.SUFFIXES:
.SECONDARY:
.SECONDEXPANSION:

.PHONY: all clean build test format regenerate_pch_files iwyu %.pdf.gdb %.pdf.lldb
.PRECIOUS: $(OUTPUT_DIR)/%.gch $(OUTPUT_DIR)/%.cpp.o

.DEFAULT_GOAL = all

all: build test

build: $(OUTPUT_BIN)

build-headers: $(CXXGCHS)

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

$(OUTPUT_BIN): $(CXXOBJ) $(UTF8PROC_OBJS) $(MINIZ_OBJS)
	@echo "    $(notdir $@)"
	@mkdir -p $(shell dirname $@)
	@$(CXX) $(CXXFLAGS) $(WARNINGS) $(DEFINES) $(LDFLAGS) -Iexternal -o $@ \
		$^ $(shell python3 tools/get_pch_objs.py $(CXX) $(CXXOBJ:%.o=%))
	@nice -n 15 python3 tools/regenerate_pch_in_background.py $(PCH_OBJS:%.gch.o=%.gch)

$(TEST_DIR)/%: $(OUTPUT_DIR)/test/%.cpp.o $(CXXLIBOBJ) $(UTF8PROC_OBJS) $(MINIZ_OBJS) $(PCH_OBJS)
	@echo "    $(notdir $@)"
	@mkdir -p $(shell dirname $@)
	@$(CXX) $(CXXFLAGS) $(WARNINGS) $(DEFINES) $(LDFLAGS) -Iexternal -o $@ $^

$(OUTPUT_DIR)/%.cpp.o: %.cpp tools/include_pch
	@echo "    $<"
	@mkdir -p $(shell dirname $@)
	@$(CXX) $(shell tools/include_pch $(CXX) $(PRECOMP_HDR) $(OUTPUT_DIR)/$*.cpp)   \
		$(CXXFLAGS) $(WARNINGS) $(INCLUDES) $(DEFINES) -c -o $@ $<


$(OUTPUT_DIR)/%.cpp.gch.o $(OUTPUT_DIR)/%.cpp.gch $(OUTPUT_DIR)/%.cpp.inc: %.cpp
	@mkdir -p $(shell dirname $@)
	@echo "$@"
	@python3 tools/generate_pch.py $(OUTPUT_DIR)/$*.cpp                                     \
		----make-deps   $(CXX) $(CXXFLAGS) -include $(PRECOMP_HDR) $(INCLUDES) $(DEFINES)   \
						-MT $(OUTPUT_DIR)/$*.cpp.o -MT $(OUTPUT_DIR)/$*.cpp.gch -MM -E $<   \
		----make-pch    $(CXX) $(CXXFLAGS) -include $(PRECOMP_HDR) $(INCLUDES)              \
						-I. $(DEFINES) -x c++-header $(CLANG_PCH_FASTER)                    \
						-o $(OUTPUT_DIR)/$*.cpp.gch.tmp                                     \
						$(OUTPUT_DIR)/$*.cpp.inc                                            \
		----make-pchobj $(CXX) $(CXXFLAGS) -c -o $(OUTPUT_DIR)/$*.cpp.gch.o.tmp             \
						$(OUTPUT_DIR)/$*.cpp.gch


$(OUTPUT_DIR)/%.c.o: %.c
	@echo "    $<"
	@mkdir -p $(shell dirname $@)
	@$(CC) $(CFLAGS) -MMD -MP -c -o $@ $<


tools/%: tools/%.cpp
	@echo "  # tool: $@"
	@$(CXX) $(WARNINGS) -std=c++20 -O3 -march=native -o $@ $<

clean:
	-@rm -r $(OUTPUT_DIR) 2> /dev/null

format:
	find source -iname '*.cpp' -or -iname '*.h' | xargs -I{} -- ./tools/sort_includes.py -i {}
	clang-format -i $(shell find source -iname "*.cpp" -or -iname "*.h")

iwyu:
	iwyu-tool -j 8 -p . source -- -Xiwyu --update_comments -Xiwyu --no_fwd_decls -Xiwyu --prefix_header_includes=keep | iwyu-fix-includes --comments --update_comments
	find source -iname '*.cpp' -or -iname '*.h' | xargs -I{} -- ./tools/sort_includes.py -i {}
	clang-format -i $(shell find source -iname "*.cpp" -or -iname "*.h")

-include $(CDEPS)
-include $(CXXDEPS)
-include $(TESTDEPS)










