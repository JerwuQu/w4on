CC=x86_64-w64-mingw32-gcc
SRC=main.c vendor.c
OBJ=$(patsubst %.c, obj/%.o, $(SRC))
CFLAGS=-Wall -std=c11 -s -O2

w4on-live.exe: $(OBJ)
	$(CC) $(CFLAGS) $^ -o $@ -static -mconsole -luser32 -lshlwapi -lwinmm

obj/%.o: %.c
	@mkdir -p obj
	$(CC) $(CFLAGS) -c $^ -o $@

.PHONY: clean
clean:
	rm -rf obj/ w4on-live.exe
