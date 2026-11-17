# The Mojibake library
#
# This file is distributed under the MIT License. See LICENSE for details.
#
# Build the Mojibake NIF. elixir_make runs this from `mix compile` and sets:
#
#   MIX_APP_PATH        the application's build directory, which gets priv/mojibake_nif.so
#   ERTS_INCLUDE_DIR    the directory that holds erl_nif.h
#
# mix.exs also sets:
#
#   MOJIBAKE_SRC_DIR    the directory that holds the C library sources
#   MOJIBAKE_SOURCES    the C library sources, from the SOURCES block of its CMakeLists.txt
#   MOJIBAKE_DEFINES    the -DMJB_FEATURE_* flags

ifeq ($(MIX_APP_PATH),)
$(error MIX_APP_PATH is not set: build with `mix compile`)
endif

ERTS_INCLUDE_DIR ?= $(shell erl -noshell -eval \
	'io:format("~ts/erts-~ts/include", [code:root_dir(), erlang:system_info(version)])' \
	-s init stop)

PRIV_DIR = $(MIX_APP_PATH)/priv
OBJ_DIR = $(MIX_APP_PATH)/obj
NIF = $(PRIV_DIR)/mojibake_nif.so

CFLAGS ?= -O2
CFLAGS += -std=c11 -fPIC -fvisibility=hidden -Wall -Wextra
CPPFLAGS += -I"$(ERTS_INCLUDE_DIR)" -I"$(MOJIBAKE_SRC_DIR)" $(MOJIBAKE_DEFINES)

UNAME_S := $(shell uname -s)

ifeq ($(UNAME_S),Darwin)
	LDFLAGS += -dynamiclib -undefined dynamic_lookup -flat_namespace
else
	LDFLAGS += -shared
endif

LIB_OBJECTS = $(patsubst %.c,$(OBJ_DIR)/lib/%.o,$(MOJIBAKE_SOURCES))
NIF_OBJECT = $(OBJ_DIR)/mojibake_nif.o
HEADERS = $(wildcard $(MOJIBAKE_SRC_DIR)/*.h)

# Rebuild everything when the compiler flags or the source directory change.
FLAGS_FILE = $(OBJ_DIR)/flags
FLAGS = $(CC) $(CFLAGS) $(CPPFLAGS)

# Set V=1 to print the full commands.
ifeq ($(V),1)
Q =
else
Q = @
endif

all: $(NIF)

$(FLAGS_FILE): FORCE
	@mkdir -p $(OBJ_DIR)
	@echo '$(FLAGS)' | cmp -s - $@ || echo '$(FLAGS)' > $@

$(OBJ_DIR)/lib/%.o: $(MOJIBAKE_SRC_DIR)/%.c $(HEADERS) $(FLAGS_FILE)
	@mkdir -p $(dir $@)
	@$(if $(Q),echo " CC $(notdir $<)")
	$(Q)$(CC) $(CFLAGS) $(CPPFLAGS) -c $< -o $@

$(NIF_OBJECT): c_src/mojibake_nif.c $(HEADERS) $(FLAGS_FILE)
	@mkdir -p $(dir $@)
	@$(if $(Q),echo " CC $(notdir $<)")
	$(Q)$(CC) $(CFLAGS) $(CPPFLAGS) -c $< -o $@

$(NIF): $(LIB_OBJECTS) $(NIF_OBJECT)
	@mkdir -p $(PRIV_DIR)
	@$(if $(Q),echo " LD $(notdir $@)")
	$(Q)$(CC) $(LDFLAGS) $^ -o $@

clean:
	rm -rf $(OBJ_DIR) $(NIF)

FORCE:

.PHONY: all clean FORCE
