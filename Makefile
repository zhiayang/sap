# Makefile
# Copyright (c) 2021, zhiayang
# Licensed under the Apache License Version 2.0.

CC                  := clang
CXX                 := clang++

WARNINGS        = -Wno-padded -Wno-cast-align -Wno-unreachable-code -Wno-packed -Wno-missing-noreturn -Wno-float-equal -Wno-unused-macros -Wextra -Wconversion -Wpedantic -Wall -Wno-unused-parameter -Wno-trigraphs
WARNINGS += -Werror
WARNINGS += -Wno-error=unused-parameter
WARNINGS += -Wno-error=unused-variable
WARNINGS += -Wno-error=unused-function
WARNINGS += -Wno-unused-but-set-variable
WARNINGS += -Wshadow
WARNINGS += -Wno-error=shadow

CXX_VERSION_STRING = $(shell $(CXX) --version | head -n1 | tr '[:upper:]' '[:lower:]')

ifeq ("$(findstring gcc,$(CXX_VERSION_STRING))", "gcc")
	WARNINGS += -Wno-missing-field-initializers
else
endif

OPT_FLAGS           := -march=native -O0 -fsanitize=address
LINKER_OPT_FLAGS    :=
COMMON_CFLAGS       := -g $(OPT_FLAGS)

OUTPUT_DIR          := build
TEST_DIR            := $(OUTPUT_DIR)/test

CFLAGS              = $(COMMON_CFLAGS) -std=c99 -fPIC -O3 -march=native
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

EXTERNAL_SRCS       := $(shell find external/libdeflate/lib -iname "*.c" -print) \
						external/utf8proc/utf8proc.c \
						external/stb_image.c \
						external/xxhash.c
EXTERNAL_OBJS       := $(EXTERNAL_SRCS:%.c=$(OUTPUT_DIR)/%.c.o)


PRECOMP_HDR         := source/include/precompile.h
PRECOMP_GCH         := $(PRECOMP_HDR:%.h=$(OUTPUT_DIR)/%.h.gch)
PRECOMP_INCLUDE     := $(PRECOMP_HDR:%.h=$(OUTPUT_DIR)/%.h)
PRECOMP_OBJ         := $(PRECOMP_HDR:%.h=$(OUTPUT_DIR)/%.h.gch.o)

ifeq ("$(findstring clang,$(shell $(CXX) --version | head -n1))", "clang")
	ifeq ("$(UNAME_IDENT)", "Darwin")
		CLANG_PCH_FASTER    := -fpch-instantiate-templates -fpch-codegen
		PCH_INCLUDE_FLAGS   := -include-pch $(PRECOMP_GCH)
	else
		CLANG_PCH_FASTER    :=
		PCH_INCLUDE_FLAGS   := -include $(PRECOMP_INCLUDE)
	endif
else
	PRECOMP_OBJ         :=
	CLANG_PCH_FASTER    :=
	PCH_INCLUDE_FLAGS   := -include $(PRECOMP_INCLUDE)
endif

GIT_REVISION        := $(shell git describe --always --dirty)
DEFINES             := -DGIT_REVISION=\"$(GIT_REVISION)\"
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

PREFIX  := $(shell pwd)
DEFINES += -DSAP_PREFIX=\"$(PREFIX)\"


.PHONY: all clean build test format iwyu %.pdf.gdb %.pdf.lldb compile_commands.json
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

$(OUTPUT_BIN): $(PRECOMP_OBJ) $(CXXOBJ) $(EXTERNAL_OBJS)
	@echo "  $(notdir $@)"
	@mkdir -p $(shell dirname $@)
	@$(CXX) $(CXXFLAGS) $(WARNINGS) $(DEFINES) $(LDFLAGS) $(LINKER_OPT_FLAGS) -Iexternal -o $@ $^

$(TEST_DIR)/%: $(OUTPUT_DIR)/test/%.cpp.o $(CXXLIBOBJ) $(EXTERNAL_OBJS) $(PRECOMP_OBJ)
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
	@$(CXX) $(CXXFLAGS) $(WARNINGS) $(INCLUDES) -x c++-header -o /dev/null $<

%.cpp.compile_db: %.cpp
	@$(CXX) $(CXXFLAGS) $(WARNINGS) $(INCLUDES) -include $(PRECOMP_HDR) -o /dev/null $<




compile_commands.json:
	@echo "  $@"
	@# first build list of commands
	@echo -n > $(OUTPUT_DIR)/cmds
	@for f in $(EXTERNAL_SRCS); do echo $(CC) $(CFLAGS) -MMD -MP -c -o $(OUTPUT_DIR)/$$f.o $(OUTPUT_DIR)/$$f; done >> $(OUTPUT_DIR)/cmds
	@for f in $(CXXSRC); do echo $(CXX) -include $(PRECOMP_HDR) $(CXXFLAGS) $(NONGCH_CXXFLAGS) $(WARNINGS) $(INCLUDES) $(DEFINES) -MMD -MP -c -o $(OUTPUT_DIR)/$$f $$f; done >> $(OUTPUT_DIR)/cmds
	@# now convert cmd list to compile_commands.json
	@cat $(OUTPUT_DIR)/cmds | awk -v CWD=$$(pwd) 'BEGIN { print "[" } END { print "]"} { print "{\"arguments\": ["; for (i = 1; i <= NF; i++) { print "\"" $$i "\"," } print "], \"directory\": \"" CWD "\", \"file\": \"" $$NF "\", \"output\": \"" $$(NF - 1) "\"}, " }' > $@
	@# do some cleaning
	@cat $@ | tr '\n' ' ' | sed -e 's/ \+/ /g' | sed -e 's/, *]/]/g' -e 's/, *]/}/g' -e 's/},/},\n/g' -e 's/ *$$/\n/' > $@.new
	@mv $@.new $@

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










