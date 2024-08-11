ODIN_DIR := $(DIR_SRC)/odin
ODIN_SRC := $(wildcard $(ODIN_DIR)/*.odin)
ODIN_OUT := $(DIR_BIN)/odin-out.exe
ODIN_FLAGS := -vet -warnings-as-errors

.PHONY: all
all: debug
	
.PHONY: debug
debug: ODIN_FLAGS += -debug
debug: $(ODIN_OUT)

.PHONY: release
release: ODIN_FLAGS += -o:size
release: $(ODIN_OUT)

$(ODIN_OUT): $(ODIN_SRC) | $(DIR_BIN)
	odin build $(ODIN_DIR) $(ODIN_FLAGS) -out:"$@"

.PHONY: clean
	$(RM) $(ODIN_OUT)
