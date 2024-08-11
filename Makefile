# This directory should never be created by the makefile!
DIR_SRC	:= src/c
DIR_OBJ	:= obj
DIR_BIN	:= bin
DIR_ALL	:= $(DIR_OBJ) $(DIR_BIN)

IN_LIST	:= $(wildcard $(DIR_SRC)/*)
IN_SRC  := $(filter %.c,$(IN_LIST))
OUT_EXE	:= $(DIR_BIN)/main.exe
OUT_OBJ	:= $(IN_SRC:$(DIR_SRC)/%.c=$(DIR_OBJ)/%.obj)
OUT_ALL	:= $(OUT_EXE) $(OUT_OBJ)

# CL.EXE flags that are indepdendent of C or C++.
#
# NOTE
#	The slash flag notation doesn't work well with GNU Make.
#	Foruntately Microsoft has the sense to support dash notation.
#
COMMON_FLAGS := -nologo -W4 -WX -permissive- -Zc:preprocessor \
				-Fe:"$(OUT_EXE)" -Fo:"$(DIR_OBJ)/"

# --- DEBUG FLAGS --------------------------------------------------------------
#
# /Od			disable optimizations (default).
# /Zi			enable debugging information. Needed for AddressSanitizer.
# 				NOTE: Generates a `vc*.pdb` file in the current directory.
#				Set the output directory with `-Fd:"<path>/"`.
# /D<macro>		define <macro> for this compilation unit.
# 				NOTE: #define _DEBUG in MSVC requires linking to a debug lib.
# /MDd			linked with MSVCRTD.LIB debug lib. Also defines _DEBUG.
# 
# --- DEBUG MACROS -------------------------------------------------------------
#
# DEBUG_USE_LONGJMP		Enable `src/main.c` to use a longjmp-based handler.
#
DEBUG_FLAGS := -Od -Zi -Fd:"$(DIR_BIN)/" -fsanitize=address

CC		:= cl
CC_FLAGS := $(COMMON_FLAGS) -std:c11 -Zc:__STDC__

CXX		:= cl
CXX_FLAGS := $(COMMON_FLAGS) -std:c++17 -Zc:__cplusplus

# Clear the builtin suffix rules.
.SUFFIXES:
.SUFFIXES: .c .h .cpp .hpp .obj .exe

.PHONY: all
all: debug
	
.PHONY: debug
debug: CC_FLAGS  += $(DEBUG_FLAGS)
debug: CXX_FLAGS += $(DEBUG_FLAGS)
debug: build
	
.PHONY: help
help:
	@echo DIR_SRC: $(DIR_SRC)
	@echo DIR_OBJ: $(DIR_OBJ)
	@echo DIR_BIN: $(DIR_BIN)
	@echo IN_LIST: $(IN_LIST)
	@echo OUT_EXE: $(OUT_EXE)
	@echo OUT_OBJ: $(OUT_OBJ)
	
# --- RELEASE FLAGS ------------------------------------------------------------
#
# /O1		maximum optimizations (favor space)
# /O2		maximum optimizations (favor speed)
# /Os		favor code space
# /Ot 		favor code speed
#
.PHONY: release
release: CC_FLAGS  += -O1
release: CXX_FLAGS += -O1
release: build
	
.PHONY: build
build: $(OUT_EXE)

$(DIR_ALL):
	$(MKDIR) $@

$(OUT_EXE): $(IN_LIST) | $(DIR_ALL)
	$(CC) $(CC_FLAGS) $(filter %.c,$^)

.PHONY: clean
clean:
	$(RM) $(OUT_ALL)
