# This directory should never be created by the makefile!
DIR_SRC	:= src
DIR_OBJ	:= obj
DIR_BIN	:= bin
DIR_ALL	:= $(DIR_OBJ) $(DIR_BIN)

export DIR_SRC DIR_OBJ DIR_BIN DIR_ALL

.PHONY: all
all:
	@echo Please run 'make arg TARGET=target'
	@echo TARGET=target is optional.
	@echo 	arg is one of:
	@echo 		msvc odin
	@echo 	target is one of:
	@echo 		build release

.PHONY: msvc
msvc:
	$(MAKE) -f msvc.mk $(TARGET)

.PHONY: odin
odin:
	$(MAKE) -f odin.mk $(TARGET)
	
$(DIR_ALL):
	$(MKDIR) $@
