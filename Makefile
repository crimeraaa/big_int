# This directory should never be created by the makefile!
DIR_SRC	:= src
DIR_OBJ	:= obj
DIR_BIN	:= bin
DIR_ALL	:= $(DIR_OBJ) $(DIR_BIN)

# These may be overriden in the command-line invocation.
TARGET = debug
ACTION = build

# Export all variables to child makefiles.
export

# WARNING: On Windows, `printf` comes with Git for Windows, but is otherwise
# not included when you install GNU Make via winget.
.PHONY: all
all: | $(DIR_ALL)
	@printf "\
	Please run 'make <arg> [TARGET=<target>] [ACTION=<action>]'\n\
		<arg> is required.\n\
	TARGET and ACTION are optional.\n\
		They default to 'debug' and 'build', respectively.\n\
	Options:\n\
		<arg>		msvc odin odin-build odin-test odin-run\n\
		<target>	debug release\n\
		<action>	build test run\n\
	"

# https://www.gnu.org/software/make/manual/html_node/Target_002dspecific.html
.PHONY: odin-build odin-test odin-run
odin-build: ACTION ::= build
odin-run: 	ACTION ::= run
odin-test: 	ACTION ::= test
odin-build odin-test odin-run: odin

.PHONY: msvc odin 
msvc odin:
	$(MAKE) -f "$@.mk" $(TARGET) ACTION=$(ACTION)
	
$(DIR_ALL):
	$(MKDIR) $@
