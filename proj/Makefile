# --- Emscripten + SDL2/TTF (multi-arquivo) ---

CC       = emcc
OUT      = index.html

# fontes do projeto
SRC      = main.c game_context.c

INC_PATH = -I.
CSTD     = -std=c99

SDL      = -sUSE_SDL=2 -sUSE_SDL_TTF=2
FS       = --preload-file FiraCode.ttf

# NÃO usar o nome SHELL aqui! Evita conflito com o make.
# Só inclui --shell-file se shell.html existir no diretório.
ifneq ($(wildcard shell.html),)
  SHELL_HTML = --shell-file shell.html
else
  SHELL_HTML =
endif

# compilação
CFLAGS_COMMON  = $(INC_PATH) $(CSTD) -MMD -MP
CFLAGS_DEBUG   = -O0 -g4 -sSAFE_HEAP=1 -sASSERTIONS=2 -sSTACK_OVERFLOW_CHECK=2
CFLAGS_RELEASE = -O3

# linkagem (flags -s do emscripten aqui)
LDFLAGS_COMMON = $(SDL)
LDFLAGS_DEBUG  = -sASSERTIONS=2 -sSAFE_HEAP=1 -sSTACK_OVERFLOW_CHECK=2
LDFLAGS_RELEASE= -sASSERTIONS=0 -sALLOW_MEMORY_GROWTH=1

# opcional: HTML único com tudo embutido
ifeq ($(SINGLE_FILE),1)
  LDFLAGS_DEBUG   += -sSINGLE_FILE=1
  LDFLAGS_RELEASE += -sSINGLE_FILE=1
endif

OBJS = $(SRC:.c=.o)
DEPS = $(OBJS:.o=.d)

.PHONY: all release debug clean

# 'make' == clean + release
all: clean release

# build de release
release: CFLAGS_MODE=$(CFLAGS_RELEASE)
release: LDFLAGS_MODE=$(LDFLAGS_RELEASE)
release: $(OUT)

# build de debug
debug: CFLAGS_MODE=$(CFLAGS_DEBUG)
debug: LDFLAGS_MODE=$(LDFLAGS_DEBUG)
debug: $(OUT)

# link final (sem flags de compilação para evitar warnings)
$(OUT): $(OBJS)
	$(CC) $(OBJS) $(LDFLAGS_COMMON) $(LDFLAGS_MODE) -o $@ $(FS) $(SHELL_HTML)

# .c -> .o
%.o: %.c
	$(CC) $(CFLAGS_COMMON) $(CFLAGS_MODE) -c $< -o $@

# limpeza (roda antes de 'all')
clean:
	rm -f $(OUT) index.js index.wasm $(OBJS) $(DEPS)

-include $(DEPS)
