# This directory should never be created by the makefile!
DIR_SRC	:= src
DIR_OBJ	:= obj
DIR_BIN	:= bin
DIR_ALL	:= $(DIR_OBJ) $(DIR_BIN)

IN_LIST	:= $(addprefix $(DIR_SRC)/,common.hpp arena.hpp arena.cpp main.cpp)
OUT_EXE	:= $(DIR_BIN)/main.exe
OUT_OBJ	:= $(IN_LIST:$(DIR_SRC)/*.cpp=$(DIR_OBJ)/*.obj)
OUT_ALL	:= $(OUT_EXE) $(OUT_OBJ)

CXX		:= cl
CXX_FLAGS := -nologo -EHsc -W4 -WX -std:c++17 -permissive- -Zc:__cplusplus \
			-Zc:preprocessor -Fe"$(OUT_EXE)" -Fo"obj/"

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
debug: CXX_FLAGS += -Od -DBIGINT_DEBUG
debug: build
	
# /O1		maximum optimizations (favor space)
# /O2		maximum optimizations (favor speed)
# /Os		favor code space
# /Ot 		favor code speed
.PHONY: release
release: CXX_FLAGS += -O1
release: build
	
.PHONY: build
build: $(OUT_EXE)

$(DIR_ALL):
	$(MKDIR) $@

$(OUT_EXE): $(IN_LIST) | $(DIR_ALL)
	$(CXX) $(CXX_FLAGS) $(filter %.cpp,$^)

.PHONY: clean
clean:
	$(RM) $(OUT_ALL)
