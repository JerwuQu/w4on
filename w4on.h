#pragma once

#include <stdint.h>

// Project specs:
// - w4on-format: Binary format used by the w4on sequencer
// - w4on-midi: Outlines MIDI parameters mapping to the ones in w4on

// Project code:
// - w4on: WASM-4 library playing the w4on format
// - w4on-kon: Reads MIDI conforming to w4on-params and packs into w4on data
// - w4on-live: Listens to MIDI events conforming to w4on-params and plays back in realtime

// w4on format:
//   There are 4 streams, one for each channel (pu1, pu2, tri, noise).
//
// Stream:
//   [16:stream size]
//   {...messages...}
//
// Instrument:
//   [8:volume]
//   [8:adsr attack]
//   [8:adsr decay]
//   [8:adsr sustain]
//   [8:adsr release]
//   [2:pan mode][2:pulse mode][has delay][has tremolo][has vibrato][has arpeggio]
//   (
//     [8:delay level]
//     [2:delay ping pong][6 bit delay decay]
//     [8:delay speed]
//   ) (
//     [8:tremolo depth]
//     [8:tremolo speed]
//   ) (
//     [8:vibrato depth]
//     [8:vibrato speed]
//   ) (
//     [8:arp speed]
//     [8:arp gate]
//   )
//
// Message IDs:
//   0x00: Long wait (Length after)
//   0x01-0x3F (63): Short wait
//   0x40-0x47 (8): Setup and Use Instrument 0-7 (Instrument after)
//   0x48-0x4F (8): Use Instrument 0-7
//   0x50-0xA7 (88): Note (See below)
//   0xA8-0xBF (24): Arp (followed by Length and N note codes)
//   0xC0: Set volume (byte after)
//   0xC1-0xC2 (3): Set pan center/left/right
//
// Length:
//   [is extended][7:length]([8:extended length])
//
// Note:
//   [is segment][is extended][6:length]([8:extended length])
//
//   "Segment" makes pitch slide during its duration

#ifndef W4ON_INST_COUNT
#define W4ON_INST_COUNT 8
#endif

typedef struct {
	uint8_t a, d, r; // 0-127
	uint8_t s; // 0-127 (ratio)
	uint8_t vol; // 0-100
	uint8_t pan; // 0-2 (center, left, right)
	uint8_t pulseMode; // 0-3

#ifndef W4ON_NO_FX
	uint8_t delayLevel; // 0-127 (0 = disabled, ratio)
	uint8_t delayDecay; // 0-63 (ratio)
	uint8_t delaySpeed; // 1-127
	uint8_t delayPingPong; // 0-2 (no, start left, start right)

	uint8_t tremoloSpeed; // 0-127 (0 = disabled)
	uint8_t tremoloDepth; // 1-127 (ratio)

	uint8_t vibratoSpeed; // 0-127 (0 = disabled)
	uint8_t vibratoDepth; // 1-127

	uint8_t arpSpeed; // 0-127 (0 = default of 1)
	uint8_t	arpGate; // 0-127 (between 0 and arpSpeed)
#endif
} w4on_inst_t;

typedef struct {
	uint8_t note; // which note: 0 = none, 1-88 = midi note number
	uint8_t noteSlide; // white note to slide to during the duration of this one: 0 = none, 1-88 = midi note number
	uint16_t noteTick; // how many ticks has passed for this note
	uint16_t noteLength; // how long the current note is
	w4on_inst_t currentInst; // instrument for the current note
	w4on_inst_t insts[W4ON_INST_COUNT];
} w4on_chn_t;

typedef int (*w4on_get_byte_fn)(void); // 0-255 for byte, negative for EOF

typedef struct {
	w4on_get_byte_fn getByteFn;
	w4on_chn_t chns[4]; // PU1, PU2, TRI, NOI
} w4on_seq_t;

static inline void w4on_seq_init(w4on_seq_t *seq, w4on_get_byte_fn getByteFn)
{
	seq->getByteFn = getByteFn;
	seq->chns[0].note = 0;
	seq->chns[1].note = 0;
	seq->chns[2].note = 0;
	seq->chns[3].note = 0;
}

void w4on_seq_tick(w4on_seq_t *seq);
