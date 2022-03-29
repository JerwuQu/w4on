OUT=w4on-wav

$(OUT): w4on-wav.c ../w4on.c ../w4on.h
	gcc -std=c99 -Wall -o $@ $<

.PHONY: clean
clean:
	rm -f $(OUT)

