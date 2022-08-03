MIDIS = $(patsubst %.mid,%,$(wildcard *.mid))
MIDI_C = $(patsubst %,%.c,$(MIDIS))

.PHONY: all
all: $(MIDI_C)

%.c: %.mid %.json
	../../convert/midi2c.sh w4on_data_$(patsubst %.mid,%,$<) $^ $(patsubst %.mid,%,$<)

.PHONY: clean
clean:
	rm *.c *.w4on