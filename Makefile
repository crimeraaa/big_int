CC		:= cl
CC_FLAGS := -nologo -EHsc -std:c++17 -W3 -WX -Fe"bin/" -Fo"obj/"

# This directory should never be created by the makefile!
DIR_SRC	:= src
DIR_OBJ	:= obj
DIR_BIN	:= bin
DIR_ALL	:= $(DIR_OBJ) $(DIR_BIN)

TARGETS	:= big_int arena
OUT_EXE := $(TARGETS:%=$(DIR_BIN)/%.exe)
OUT_OBJ := $(TARGETS:%=$(DIR_OBJ)/%.obj)
OUT_ALL := $(OUT_EXE) $(OUT_OBJ)

# Clear the builtin suffix rules.
.SUFFIXES:
.SUFFIXES: .hpp .cpp .obj .exe

.PHONY: all
all: debug
	
# /Od		disable optimizations (default).
# /Zi		enable debugging information.
# /D<macro>	define <macro> for this compilation unit.
# 			NOTE: #define _DEBUG in MSVC requires linking to a debug lib.
# /MDd		linked with MSVCRTD.LIB debug lib. Also defines _DEBUG.
.PHONY: debug
debug: CC_FLAGS += -Od -DBIGINT_DEBUG
debug: build
	
# /O1		maximum optimizations (favor space)
# /O2		maximum optimizations (favor speed)
# /Os		favor code space
# /Ot 		favor code speed
.PHONY: release
release: CC_FLAGS += -O1
release: build
	
.PHONY: build
build: $(OUT_EXE)

$(DIR_ALL):
	$(MKDIR) $@

$(DIR_BIN)/%.exe: $(DIR_SRC)/%.cpp $(DIR_SRC)/%.hpp $(DIR_SRC)/common.hpp | $(DIR_ALL)
	$(CC) $(CC_FLAGS) $<

.PHONY: clean
clean:
	$(RM) $(OUT_ALL)
