#pragma once

#include <stdint.h>
#include <stddef.h>

// Project specs:
// - w4on-format: Binary format used by the w4on sequencer
// - w4on-midi: Outlines MIDI parameters mapping to the ones in w4on

// Project code:
// - w4on: WASM-4 library playing the w4on format
// - w4on-kon: Reads MIDI conforming to w4on-params and packs into w4on data
// - w4on-live: Listens to MIDI events conforming to w4on-params and plays back in realtime

// w4on format:
//   There are 4 streams, one for each channel (PU1, PU2, TRI, NOI)
//
//   On init, each channel will get an instrument according to its index (if possible).
//   Meaning PU1 will get instrument 0 and NOI will get instrument 3 as default.
//
// Format:
//   [8:instrument count]
//   {...instruments...}
//   {...channel streams...}
//
// Channel stream:
//   [16:stream size]
//   {...messages...}
//
// Instrument:
//   [copy instrument][has tone flags][has volume][has adsr][has delay][has tremolo][has vibrato][has arpeggio]
//   ([8:source instrument index])
//   ([4:RESERVED][2:pan mode][2:pulse mode])
//   ([8:volume])
//   (
//     [8:adsr attack]
//     [8:adsr decay]
//     [8:adsr sustain]
//     [8:adsr release]
//   ) (
//     [8:delay level]
//     [2:delay ping pong][6 bit delay decay]
//     [8:delay speed]
//   ) (
//     [8:tremolo speed]
//     [8:tremolo depth]
//   ) (
//     [8:vibrato speed]
//     [8:vibrato depth]
//   ) (
//     [8:arp speed]
//     [8:arp gate]
//   )
//
// Message IDs:
//   0x00: Long wait (Length after)
//   0x01-0x3F (63): Short wait
//   0x40-0x97 (88): Note (See below)
//   0x98-0xAF (24): Arp (followed by Length and N note codes)
//   0xB0-0xBE (15): Use instrument N
//   0xBF: Use instrument 15+ (index after)
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

typedef struct {
	uint8_t vol; // 0-100
	uint8_t pan; // 0-2 (center, left, right)

	uint8_t pulseMode; // 0-3

	uint8_t a, d, r; // 0-127
	uint8_t s; // 0-127 (ratio)

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
} w4on_inst_t;

typedef struct {
	uint8_t note; // which note: 0 = none, 1-88 = midi note number
	uint8_t noteSlide; // white note to slide to during the duration of this one: 0 = none, 1-88 = midi note number
	uint16_t noteTick; // how many ticks has passed for this note
	uint16_t noteLength; // how long the current note is
	uint16_t dataStart, dataI, dataLen; // position in data stream
	w4on_inst_t inst; // the instrument being used for the current note
} w4on_chn_t;

typedef struct {
	const uint8_t *data;
	w4on_chn_t chns[4]; // PU1, PU2, TRI, NOI
} w4on_seq_t;

void w4on_seq_init(w4on_seq_t *seq, const uint8_t *data);
void w4on_seq_tick(w4on_seq_t *seq);
