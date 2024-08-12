# This directory should never be created by the makefile!
DIR_SRC	:= src
DIR_OBJ	:= obj
DIR_BIN	:= bin
DIR_ALL	:= $(DIR_OBJ) $(DIR_BIN)

# Export all variables to child makefiles.
export

.PHONY: all
all:
	@echo "Please run 'make <arg> [TARGET=<target>] [ACTION=<action>]'"
	@echo "<arg> is required while TARGET and ACTION are optional."
	@echo "They default to 'debug' and 'build', respectively."
	@echo "Options:"
	@echo "<arg>	msvc odin odin-build odin-test odin-run"
	@echo "<target>	debug release"
	@echo "<action>	build test run"

.PHONY: msvc
msvc:
	$(MAKE) -f msvc.mk $(TARGET)

.PHONY: odin odin-test odin-run
odin-build odin-test odin-run: ACTION=$(patsubst odin-%,%,$@)
odin-build odin-test odin-run odin:
	$(MAKE) -f odin.mk $(TARGET) ACTION=$(ACTION)
	
$(DIR_ALL):
	$(MKDIR) $@
