# Makefile
# Copyright (c) 2021, zhiayang
# Licensed under the Apache License Version 2.0.

WARNINGS        = -Wno-padded -Wno-cast-align -Wno-unreachable-code -Wno-packed -Wno-missing-noreturn -Wno-float-equal -Wno-unused-macros -Wextra -Wconversion -Wpedantic -Wall -Wno-unused-parameter -Wno-trigraphs
WARNINGS += -Werror
WARNINGS += -Wno-error=unused-parameter
WARNINGS += -Wno-error=unused-variable
WARNINGS += -Wno-error=unused-function
WARNINGS += -Wno-unused-but-set-variable

COMMON_CFLAGS   = -O0 -g -fsanitize=address

OUTPUT_DIR      := build
TEST_DIR        := $(OUTPUT_DIR)/test

CC              := clang
CXX             := clang++

CFLAGS          = $(COMMON_CFLAGS) -std=c99 -fPIC -O3
CXXFLAGS        = $(COMMON_CFLAGS) -Wno-old-style-cast -std=c++20 -fno-exceptions

CXXSRC          := $(shell find source -iname "*.cpp" -print)
CXXOBJ          := $(CXXSRC:%.cpp=$(OUTPUT_DIR)/%.cpp.o)
CXXLIBOBJ       := $(filter-out $(OUTPUT_DIR)/source/main.cpp.o,$(CXXOBJ))
CXXDEPS         := $(CXXOBJ:.o=.d)

CXXHDRS         := $(shell find source -iname "*.h" -print)
CXXGCHS         := $(CXXHDRS:%.h=$(OUTPUT_DIR)/%.h.gch)
CXXGCHS         := $(filter-out $(OUTPUT_DIR)/source/include/defs.h.gch,$(CXXGCHS))
CXXGCHS         := $(filter-out $(OUTPUT_DIR)/source/include/pool.h.gch,$(CXXGCHS))
CXXGCHS         := $(filter-out $(OUTPUT_DIR)/source/include/units.h.gch,$(CXXGCHS))
CXXGCHS         := $(filter-out $(OUTPUT_DIR)/source/include/error.h.gch,$(CXXGCHS))

TESTSRC          = $(shell find test -iname "*.cpp" -print)
TESTOBJ          = $(TESTSRC:%.cpp=$(OUTPUT_DIR)/%.cpp.o)
TESTDEPS         = $(TESTOBJ:.o=.d)
TESTS            = $(TESTSRC:test/%.cpp=$(TEST_DIR)/%)

UTF8PROC_SRCS   = external/utf8proc/utf8proc.c
UTF8PROC_OBJS   = $(UTF8PROC_SRCS:%.c=$(OUTPUT_DIR)/%.c.o)

MINIZ_SRCS      = external/miniz/miniz.c
MINIZ_OBJS      = $(MINIZ_SRCS:%.c=$(OUTPUT_DIR)/%.c.o)

PRECOMP_HDR     := source/include/precompile.h
PRECOMP_GCH     := $(PRECOMP_HDR:%.h=$(OUTPUT_DIR)/%.h.gch)

DEFINES         :=
INCLUDES        := -Isource/include -Iexternal

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




OUTPUT_BIN      := $(OUTPUT_DIR)/sap

.PHONY: all clean build test format iwyu %.pdf.gdb %.pdf.lldb
.PRECIOUS: $(PRECOMP_GCH) $(OUTPUT_DIR)/%.cpp.o
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
	@echo "  $(notdir $@)"
	@mkdir -p $(shell dirname $@)
	@$(CXX) $(CXXFLAGS) $(WARNINGS) $(DEFINES) $(LDFLAGS) -Iexternal -o $@ $^

$(TEST_DIR)/%: $(OUTPUT_DIR)/test/%.cpp.o $(CXXLIBOBJ) $(UTF8PROC_OBJS) $(MINIZ_OBJS)
	@echo "  $(notdir $@)"
	@mkdir -p $(shell dirname $@)
	@$(CXX) $(CXXFLAGS) $(WARNINGS) $(DEFINES) $(LDFLAGS) -Iexternal -o $@ $^

$(OUTPUT_DIR)/%.cpp.o: %.cpp Makefile $(PRECOMP_GCH)
	@echo "  $<"
	@mkdir -p $(shell dirname $@)
	@$(CXX) $(CXXFLAGS) $(WARNINGS) $(INCLUDES) $(DEFINES) -include $(PRECOMP_HDR) -MMD -MP -c -o $@ $<

$(OUTPUT_DIR)/%.c.o: %.c Makefile
	@echo "  $<"
	@mkdir -p $(shell dirname $@)
	@$(CC) $(CFLAGS) -MMD -MP -c -o $@ $<

$(PRECOMP_GCH): $(PRECOMP_HDR) Makefile
	@printf "# precompiling header $<\n"
	@mkdir -p $(shell dirname $@)
	@$(CXX) $(CXXFLAGS) $(WARNINGS) $(INCLUDES) -MMD -MP -x c++-header -o $@ $<

$(OUTPUT_DIR)/%.h.gch: %.h $(PRECOMP_GCH) Makefile
	@printf "# precompiling header $<\n"
	@mkdir -p $(shell dirname $@)
	@$(CXX) $(CXXFLAGS) $(WARNINGS) $(INCLUDES) -include $(PRECOMP_HDR) -MMD -MP -x c++-header -o $@ $<

clean:
	-@rm -rf $(OUTPUT_DIR)

format:
	find source -iname '*.cpp' -or -iname '*.h' | xargs -I{} -- ./sort_includes.py -i {}
	clang-format -i $(shell find source -iname "*.cpp" -or -iname "*.h")

iwyu:
	iwyu-tool -j 8 -p . source -- -Xiwyu --update_comments -Xiwyu --no_fwd_decls -Xiwyu --prefix_header_includes=keep | iwyu-fix-includes --comments --update_comments
	find source -iname '*.cpp' -or -iname '*.h' | xargs -I{} -- ./sort_includes.py -i {}
	clang-format -i $(shell find source -iname "*.cpp" -or -iname "*.h")

-include $(CXXDEPS)
-include $(TESTDEPS)
-include $(CDEPS)
-include $(PRECOMP_GCH:%.gch=%.d)










