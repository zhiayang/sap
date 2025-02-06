# Makefile
# Copyright (c) 2021, yuki
# Licensed under the Apache License Version 2.0.

CC                  := clang
CXX                 := clang++

WARNINGS        = -Wno-padded -Wno-cast-align -Wno-unreachable-code -Wno-packed -Wno-missing-noreturn -Wno-float-equal -Wno-unused-macros -Wextra -Wconversion -Wpedantic -Wall -Wno-unused-parameter -Wno-trigraphs
WARNINGS += -Werror
WARNINGS += -Wno-error=unused-parameter
WARNINGS += -Wno-error=unused-variable
WARNINGS += -Wno-error=unused-function
WARNINGS += -Wno-unused-but-set-variable
WARNINGS += -Wno-string-conversion
WARNINGS += -Wshadow
WARNINGS += -Wno-error=shadow

CXX_VERSION_STRING = $(shell $(CXX) --version | head -n1 | tr '[:upper:]' '[:lower:]')
CXX_MAJOR_VERSION = $(shell echo "$(CXX_VERSION_STRING)" | cut -d' ' -f3 | cut -d. -f1)

ifeq ("$(findstring gcc,$(CXX_VERSION_STRING))", "gcc")
	WARNINGS += -Wno-missing-field-initializers
else ifeq ("$(shell echo '$(CXX_MAJOR_VERSION) >= 19' | bc)", "1")
	WARNINGS += -Wno-missing-designated-field-initializers
endif

OPT_FLAGS           := -O0 -fsanitize=address
LINKER_OPT_FLAGS    :=
COMMON_CFLAGS       := $(OPT_FLAGS)

OUTPUT_DIR          := build
TEST_DIR            := $(OUTPUT_DIR)/test

CFLAGS              = $(COMMON_CFLAGS) -std=c99 -O3 -march=native
CXXFLAGS            = $(COMMON_CFLAGS) -Wno-old-style-cast -std=c++20 -fno-exceptions

CXXSRC              := $(shell find source -iname "*.cpp" -print)
CXXOBJ              := $(CXXSRC:%.cpp=$(OUTPUT_DIR)/%.cpp.o)
CXXDEPS             := $(CXXOBJ:.o=.d)

TESTSRC             := $(shell find tests/source -iname "*.cpp" -print)
TESTOBJ             := $(TESTSRC:%.cpp=$(OUTPUT_DIR)/tester/%.cpp.o)
TESTDEPS            := $(TESTOBJ:.o=.d)

CXXHDR              := $(shell find source -iname "*.h" -print) \
                       $(shell find tests/source -iname "*.h" -print)

SPECIAL_HEADERS     := $(addprefix source/,defs.h pool.h util.h units.h error.h types.h)
SPECIAL_HDRS_COMPDB := $(SPECIAL_HEADERS:%=%.special_compile_db)

CXX_COMPDB_HDRS     := $(filter-out $(SPECIAL_HEADERS),$(CXXHDR))
CXX_COMPDB          := $(CXX_COMPDB_HDRS:%=%.compile_db) $(CXXSRC:%=%.compile_db)

EXTERNAL_SRCS       := $(shell find external/libdeflate/lib -iname "*.c" -print) \
						external/utf8proc/utf8proc.c \
						external/stb_image.c \
						external/xxhash.c
EXTERNAL_OBJS       := $(EXTERNAL_SRCS:%.c=$(OUTPUT_DIR)/%.c.o)


PRECOMP_HDR         := source/precompile.h
PRECOMP_GCH         := $(PRECOMP_HDR:%.h=$(OUTPUT_DIR)/%.h.pch)
PRECOMP_INCLUDE     := $(PRECOMP_HDR:%.h=$(OUTPUT_DIR)/%.h)
PRECOMP_OBJ         := $(PRECOMP_HDR:%.h=$(OUTPUT_DIR)/%.h.pch.o)

UNAME_IDENT := $(shell uname)

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
INCLUDES            := -Isource -Iexternal

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
TESTER_BIN      := $(OUTPUT_DIR)/sap-test

PREFIX  := $(shell pwd)
DEFINES += -DSAP_PREFIX=\"$(PREFIX)\"


.PHONY: all clean build tester check %.pdf.gdb %.pdf.lldb
.PRECIOUS: $(PRECOMP_GCH) $(OUTPUT_DIR)/%.cpp.o
.DEFAULT_GOAL = all

all: build

build: $(OUTPUT_BIN)

tester: $(TESTER_BIN)

check: tester
	@env MallocNanoZone=0 $(TESTER_BIN) $(shell pwd)/tests

compdb: $(CXX_COMPDB) $(SPECIAL_HDRS_COMPDB)

%.pdf: %.sap build
	@env MallocNanoZone=0 $(OUTPUT_BIN) $<

%.pdf.gdb: %.sap build
	@gdb --args $(OUTPUT_BIN) $<

%.pdf.lldb: %.sap build
	@lldb --args $(OUTPUT_BIN) $<

$(OUTPUT_BIN): $(PRECOMP_OBJ) $(CXXOBJ) $(EXTERNAL_OBJS)
	@echo "  $(notdir $@)"
	@mkdir -p $(shell dirname $@)
	@$(CXX) $(CXXFLAGS) $(WARNINGS) $(DEFINES) $(LDFLAGS) $(LINKER_OPT_FLAGS) -Iexternal -o $@ $^
	@case "$(CXXFLAGS)" in "-g "* | *" -g" | *" -g "*) \
		echo "  debuginfo"; dsymutil $@ 2>/dev/null &  \
		disown -a ;; *) ;; esac


$(TESTER_BIN): $(PRECOMP_OBJ) $(EXTERNAL_OBJS) $(filter-out $(OUTPUT_DIR)/source/main.cpp.o,$(CXXOBJ)) $(TESTOBJ)
	@echo "  $(notdir $@)"
	@mkdir -p $(shell dirname $@)
	@$(CXX) $(CXXFLAGS) $(WARNINGS) $(DEFINES) $(LDFLAGS) $(LINKER_OPT_FLAGS) -Iexternal -o $@ $^
	@# @case "$(CXXFLAGS)" in "-g "* | *" -g" | *" -g "*) \
	@#	echo "  debuginfo"; dsymutil $@ 2>/dev/null ;; *) ;; esac




$(OUTPUT_DIR)/%.cpp.o $(OUTPUT_DIR)/tester/%.cpp.o: %.cpp $(PRECOMP_GCH)
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
	@cp $< $(@:%.pch=%)

$(PRECOMP_OBJ): $(PRECOMP_GCH)
	@printf "# compiling pch\n"
	@mkdir -p $(shell dirname $@)
	@$(CXX) $(CXXFLAGS) $(WARNINGS) -c -o $@ $<

%.h.compile_db: %.h
	@$(CXX) $(CXXFLAGS) $(WARNINGS) $(INCLUDES) $(DEFINES) -include $(PRECOMP_HDR) -x c++-header -o /dev/null $<

%.h.special_compile_db: %.h
	@$(CXX) $(CXXFLAGS) $(WARNINGS) $(INCLUDES) $(DEFINES) -x c++-header -o /dev/null $<

%.cpp.compile_db: %.cpp
	@$(CXX) $(CXXFLAGS) $(WARNINGS) $(INCLUDES) $(DEFINES) -include $(PRECOMP_HDR) -o /dev/null $<

clean:
	-@rm -fr $(OUTPUT_DIR)

-include $(CXXDEPS)
-include $(TESTDEPS)
-include $(CDEPS)
-include $(PRECOMP_GCH:%.pch=%.d)










