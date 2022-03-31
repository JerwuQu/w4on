#include "w4on.h"

// (   = ´  _ `=)

extern void tone(uint32_t frequency, uint32_t duration, uint32_t volume, uint32_t flags);

static inline void zeromem(void *ptr, size_t sz)
{
	while (sz--) {
		((char*)ptr)[sz] = 0;
	}
}

static w4on_inst_t instrumentByIndex(const uint8_t *data, size_t index);
static w4on_inst_t parseInstrument(const uint8_t *data, size_t *offset)
{
	// TODO: use stack local offset...?
	w4on_inst_t inst;
	uint8_t flags = data[(*offset)++];
	if (flags & 0x80) { // Copy
		inst = instrumentByIndex(data, data[(*offset)++]);
	} else {
		zeromem(&inst, sizeof(w4on_inst_t));
		inst.vol = 100;
		inst.s = 127;
	}
	if (flags & 0x40) { // Flags
		uint8_t flags = data[(*offset)++];
		inst.pulseMode = flags & 0x3;
		inst.pan = (flags >> 2) & 0x3;
	}
	if (flags & 0x20) { // Volume
		inst.vol = data[(*offset)++];
	}
	if (flags & 0x10) { // ASDR
		inst.a = data[(*offset)++];
		inst.d = data[(*offset)++];
		inst.s = data[(*offset)++];
		inst.r = data[(*offset)++];
	}
	if (flags & 0x8) { // Delay
		inst.delayLevel = data[(*offset)++];
		inst.delaySpeed = data[(*offset)++];
		uint8_t tmp = data[(*offset)++];;
		inst.delayDecay = tmp & 0x3F;
		inst.delayPingPong = (tmp >> 6) & 0x3;
	}
	if (flags & 0x4) { // Tremolo
		inst.tremoloSpeed = data[(*offset)++];
		inst.tremoloDepth = data[(*offset)++];
	}
	if (flags & 0x2) { // Vibrato
		inst.vibratoSpeed = data[(*offset)++];
		inst.vibratoDepth = data[(*offset)++];
	}
	if (flags & 0x1) { // Arpeggio
		inst.arpSpeed = data[(*offset)++];
		inst.arpGate = data[(*offset)++];
	}
	return inst;
}

static w4on_inst_t instrumentByIndex(const uint8_t *data, size_t index)
{
	size_t instCount = data[0];
	if (index < instCount) {
		size_t offset = 1;
		while (index--) {
			parseInstrument(data, &offset);
		}
		return parseInstrument(data, &offset);
	} else {
		w4on_inst_t inst;
		zeromem(&inst, sizeof(w4on_inst_t));
		return inst;
	}
}

void w4on_seq_init(w4on_seq_t *seq, const uint8_t *data)
{
	size_t instCount = data[0];
	size_t offset = 1;
	for (size_t i = 0; i < instCount; i++) {
		parseInstrument(data, &offset);
	}

	seq->data = data;
	for (size_t i = 0; i < 4; i++) {
		seq->chns[i].note = 0;
		seq->chns[i].dataI = 0;
		seq->chns[i].inst = instrumentByIndex(data, i);
		size_t sz = (data[offset] << 8) | data[offset + 1];
		seq->chns[i].dataStart = offset + 2;
		seq->chns[i].dataLen = sz;
		offset += sz + 2;
	}
}

void w4on_seq_tick(w4on_seq_t *seq)
{
	for (size_t i = 0; i < 4; i++) {
		// Handle new events
		if (seq->chns[i].dataI < seq->chns[i].dataLen) {
			uint8_t msg = seq->data[seq->chns[i].dataI++];

			if (msg >= W4ON_MSG_RESERVED) {
				// TODO: dump error
			} else if (msg >= W4ON_MSG_SET_PAN_3) {
				seq->chns[i].inst.pan = msg - W4ON_MSG_SET_PAN_3;
			} else if (msg == W4ON_MSG_SET_VOLUME_EX) {
				seq->chns[i].inst.vol = seq->data[seq->chns[i].dataI++];
			} else if (msg == W4ON_MSG_USE_INSTRUMENT_EX) {
				seq->chns[i].inst = instrumentByIndex(seq->data, seq->data[seq->chns[i].dataI++]);
			} else if (msg >= W4ON_MSG_USE_INSTRUMENT_15) {
				seq->chns[i].inst = instrumentByIndex(seq->data, msg - W4ON_MSG_USE_INSTRUMENT_15);
			} else if (msg >= W4ON_MSG_ARP_24_EX) {
				seq->chns[i].noteTick = 0;
				seq->chns[i].noteSlide = 0;
				// TODO
			} else if (msg >= W4ON_MSG_NOTE_88_EX) {
				seq->chns[i].note = msg - W4ON_MSG_NOTE_88_EX;
				seq->chns[i].noteTick = 0;
				// seq->chns[i].noteLength
				// seq->chns[i].noteSlide
				// TODO
			} else if (msg >= W4ON_MSG_SHORT_WAIT_63) {
				seq->chns[i].note = 0;
				seq->chns[i].noteTick = 0;
				seq->chns[i].noteLength = W4ON_MSG_SHORT_WAIT_63 - msg;
			} else if (msg == W4ON_MSG_LONG_WAIT_EX) {
				seq->chns[i].note = 0;
				seq->chns[i].noteTick = 0;
				// TODO
			}
		}

		// Playback
		// (   = ´  _ `=)
	}
}
