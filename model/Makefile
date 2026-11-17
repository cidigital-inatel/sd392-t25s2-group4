# ISS do projeto: executável autônomo, autoteste e biblioteca para DPI-C.
#
#   make          compila build/iss e build/test_iss
#   make test     roda o autoteste
#   make lib      biblioteca compartilhada para o Xcelium (DPI-C, M3.2)
#   make clean

CC      ?= gcc
CFLAGS  ?= -std=gnu99 -O2 -Wall -Wextra
BUILD   := build
EXE     :=
ifeq ($(OS),Windows_NT)
EXE     := .exe
endif

all: $(BUILD)/iss$(EXE) $(BUILD)/test_iss$(EXE)

$(BUILD):
	mkdir -p $(BUILD)

$(BUILD)/iss$(EXE): iss.c iss_main.c iss.h | $(BUILD)
	$(CC) $(CFLAGS) -o $@ iss.c iss_main.c

$(BUILD)/test_iss$(EXE): iss.c tests/test_iss.c iss.h | $(BUILD)
	$(CC) $(CFLAGS) -o $@ iss.c tests/test_iss.c

test: $(BUILD)/test_iss$(EXE)
	./$(BUILD)/test_iss$(EXE)

lib: $(BUILD)/libiss.so

$(BUILD)/libiss.so: iss.c iss.h | $(BUILD)
	$(CC) $(CFLAGS) -fPIC -shared -o $@ iss.c

clean:
	rm -rf $(BUILD)

.PHONY: all test lib clean
