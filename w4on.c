#include "w4on.h"

// (   = ´  _ `=)

extern void tone(uint32_t frequency, uint32_t duration, uint32_t volume, uint32_t flags);

static inline void zeromem(void *ptr, size_t sz)
{
	while (sz--) {
		((char*)ptr)[sz] = 0;
	}
}

static uint16_t getNoteFreq(uint8_t n)
{
	static const uint16_t freqs[] = {
		28160, // A10
		29834,
		31609,
		33488,
		35479,
		37589,
		39824,
		42192,
		44701,
		47359,
		50175,
		53159,
	};

	return freqs[n % 12] >> (10 - n / 12);
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

static size_t readLength(const uint8_t *data, uint16_t *offset)
{
	size_t a = data[(*offset)++];
	return (a & 0x80) ? ((a & 0x7F) | data[(*offset)++]) : a;
}

void w4on_seq_init(w4on_seq_t *seq, const uint8_t *data)
{
	size_t instCount = data[0];
	size_t offset = 1;
	for (size_t i = 0; i < instCount; i++) {
		parseInstrument(data, &offset);
	}

	seq->data = data;
	for (size_t chI = 0; chI < 4; chI++) {
		seq->chns[chI].note = 0;
		seq->chns[chI].dataI = 0;
		seq->chns[chI].inst = instrumentByIndex(data, chI);
		size_t sz = (data[offset] << 8) | data[offset + 1];
		seq->chns[chI].dataStart = offset + 2;
		seq->chns[chI].dataLen = sz;
		offset += sz + 2;
	}
}

void w4on_seq_tick(w4on_seq_t *seq)
{
	for (size_t chI = 0; chI < 4; chI++) {
		w4on_chn_t *ch = &seq->chns[chI];

		// Handle new events
		while (!ch->noteTicksLeft && ch->dataI < ch->dataLen) {
			uint8_t msg = seq->data[ch->dataI++];

			if (msg >= W4ON_MSG_RESERVED) {
				// XXX
			} else if (msg >= W4ON_MSG_SET_PAN_3) {
				ch->inst.pan = msg - W4ON_MSG_SET_PAN_3;
			} else if (msg == W4ON_MSG_SET_VOLUME_EX) {
				ch->inst.vol = seq->data[ch->dataI++];
			} else if (msg == W4ON_MSG_USE_INSTRUMENT_EX) {
				ch->inst = instrumentByIndex(seq->data, seq->data[ch->dataI++]);
			} else if (msg >= W4ON_MSG_USE_INSTRUMENT_15) {
				ch->inst = instrumentByIndex(seq->data, msg - W4ON_MSG_USE_INSTRUMENT_15);
			} else if (msg >= W4ON_MSG_ARP_24_EX) {
				ch->note = msg - W4ON_MSG_ARP_24_EX + 1; // arp note count
				ch->noteTick = 0;
				ch->noteTicksLeft = readLength(seq->data, &ch->dataI);
				ch->arpNoteDataI = ch->dataI;
				ch->dataI += ch->note;
			} else if (msg >= W4ON_MSG_NOTE_88_EX) {
				size_t tmp = seq->data[ch->dataI++];
				size_t length = (tmp & 0x40) ? ((tmp & 0x7F) | seq->data[ch->dataI++]) : tmp;
				ch->noteTicksLeft += length;
				if (tmp & 0x80) {
					ch->noteSlide = msg - W4ON_MSG_NOTE_88_EX;
					// TODO
				} else {
					ch->noteTick = 0;
					ch->note = msg - W4ON_MSG_NOTE_88_EX;
					ch->noteSlide = 0;
				}
			} else if (msg >= W4ON_MSG_SHORT_WAIT_63) {
				ch->note = 0;
				ch->noteTicksLeft = W4ON_MSG_SHORT_WAIT_63 - msg;
			} else if (msg == W4ON_MSG_LONG_WAIT_EX) {
				ch->note = 0;
				ch->noteTicksLeft = readLength(seq->data, &ch->dataI);
			}
		}

		// Playback
		// (   = ´  _ `=)
		// Determine note param
		if (ch->arpNoteDataI) {
		} else if (ch->noteSlide) {
		} else if (ch->note) {
		}

		// ADSR emulation is needed for realtime playback and to have segments following ADSR
		if (ch->noteTicksLeft) {
			// TODO note attack/sustain/decay
			ch->noteTicksLeft--;
		} else {
			// TODO note release
		}

		// Refresh tone every tick
		// tone(freq,
		// 		1 + (adsr slope),
		// 		adsr level,
		// 		chI | (ch->inst.pulseMode << 2) | (ch->inst.pan << 4));

		if (ch->note) {
			ch->noteTick++;
		}
	}
}
