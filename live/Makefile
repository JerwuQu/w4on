CC=x86_64-w64-mingw32-gcc
SRC=main.c vendor.c ../w4on.c
DEP=vendor.h ../w4on.h
OBJ=$(patsubst %.c, obj/%.o, $(SRC))
CFLAGS=-Wall -std=c11 -s -O2

w4on-live.exe: $(OBJ) $(DEP)
	$(CC) $(CFLAGS) $(OBJ) -o $@ -static -mconsole -luser32 -lshlwapi -lwinmm

obj/%.o: %.c
	@mkdir -p obj
	$(CC) $(CFLAGS) -c $^ -o $@

.PHONY: clean
clean:
	rm -rf obj/ w4on-live.exe
