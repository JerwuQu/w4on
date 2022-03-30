CC=x86_64-w64-mingw32-gcc
CFLAGS=-Wall -std=c11 -s -O2

w4on-live.exe: obj/main.o obj/w4on.o obj/vendor.o
	$(CC) $(CFLAGS) $^ -o $@ -static -mconsole -luser32 -lshlwapi -lwinmm

obj/main.o: main.c
	@mkdir -p obj
	$(CC) $(CFLAGS) -c $< -o $@

obj/w4on.o: ../w4on.c ../w4on.h
	@mkdir -p obj
	$(CC) $(CFLAGS) -c $< -o $@

obj/vendor.o: vendor.c vendor.h
	@mkdir -p obj
	$(CC) $(CFLAGS) -c $< -o $@

.PHONY: clean
clean:
	rm -rf obj/ w4on-live.exe
