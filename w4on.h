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
//     [8:delay speed]
//     [2:delay ping pong][6 bit delay decay]
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
// Length:
//   [is extended][7:length]([8:extended length])
//
// Note:
//   A "segment" note makes pitch slide during its duration

// Message IDs:
// The number (if any) is how many message IDs the type spans
// "EX" means there is additional data after the message ID
// TODO: separate by EX and not...?
// TODO: move to "internal" header
#define W4ON_MSG_LONG_WAIT_EX 0x00 // EX: Length
#define W4ON_MSG_SHORT_WAIT_63 0x01
#define W4ON_MSG_NOTE_88_EX 0x40 // EX: [is segment][is extended][6:length]([8:extended length])
#define W4ON_MSG_ARP_24_EX 0x98 // Starts at 1, EX: Length, then N note codes (0-87)
#define W4ON_MSG_USE_INSTRUMENT_15 0xB0
#define W4ON_MSG_USE_INSTRUMENT_EX 0xBF // EX: [8:index]
#define W4ON_MSG_SET_VOLUME_EX 0xC0 // EX: [8:volume]
#define W4ON_MSG_SET_PAN_3 0xC1 // center, left, right
#define W4ON_MSG_RESERVED 0xC4

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
	uint8_t note; // which note: 0 = none, 1-88 = midi note number - *if arp is active* it instead acts as "arp note count"
	uint8_t noteSlide; // white note to slide to during the duration of this one: 0 = none, 1-88 = midi note number
	uint16_t noteTick; // how many ticks has passed for this note
	uint16_t noteTicksLeft; // how long the current note is
	uint16_t arpNoteDataI; // 0 = no arp active, otherwise index to note list
	uint16_t dataStart, dataI, dataLen; // position in data stream
	w4on_inst_t inst; // the instrument being used for the current note
} w4on_chn_t;

typedef struct {
	const uint8_t *data;
	w4on_chn_t chns[4]; // PU1, PU2, TRI, NOI
} w4on_seq_t;

void w4on_seq_init(w4on_seq_t *seq, const uint8_t *data);
void w4on_seq_tick(w4on_seq_t *seq);
