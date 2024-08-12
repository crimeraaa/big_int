ODIN_DIR := $(DIR_SRC)/odin
ODIN_SRC := $(wildcard $(ODIN_DIR)/*.odin)
ODIN_OUT := $(DIR_BIN)/odin-out.exe
ODIN_FLAGS := -vet

# Expand only when referenced as $(TARGET) may update this.
ODIN_CMDLINE = $(ODIN_DIR) $(ODIN_FLAGS) -out:"$(ODIN_OUT)"

# These may be defined in the command-line invocation.
TARGET = debug
ACTION = build

.PHONY: all
all: $(TARGET)

# --- TARGETS ------------------------------------------------------------- {{{1
.PHONY: debug release

debug: 	 ODIN_FLAGS += -debug
release: ODIN_FLAGS += -o:size
debug release: $(ACTION)

# 1}}} -------------------------------------------------------------------------

# --- ACTIONS ------------------------------------------------------------- {{{1
.PHONY: build test run
	
# Must be run only when build needs updating.
build: $(ODIN_OUT)
$(ODIN_OUT): $(ODIN_SRC) | $(DIR_BIN)
	odin build $(ODIN_CMDLINE)

# Must be run regardless of the dependency graph.
test run: $(ODIN_SRC) | $(DIR_BIN)
	odin $@ $(ODIN_CMDLINE)

# 1}}} -------------------------------------------------------------------------

.PHONY: clean
clean:
	$(RM) $(ODIN_OUT)
