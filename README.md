# w4on

WIPs all around

# Outline

## Specs
- w4on format: Binary format used by the w4on sequencer

## Code
- w4on: WASM-4 library playing the w4on format
- w4on-convert: Reads a regular MIDI together with an instrument definition file and packs into w4on data
- w4on-wav: Converts a w4on data file into a wav audio file

# Usage

You're kinda on your own for now...

## Clone

- `git clone https://github.com/JerwuQu/w4on.git`
- `git submodule init`
- `git submodule update`

# w4on format

## Format
	[8:track count]
	{...tracks...}

### Track
	[16:track size (bytes)]
	[4:reserved][2:pulse mode][2:channel]
	{...messages...}

### ExLength
	[is extended][7:length]([8:extended length])

### Messages

Messages are comprised of one or more bytes and are (for now) mostly documented as #defines in w4on.h.

Lengths are given in WASM-4 ticks, and most parameters are the same as in WASM-4 (pan, pulse mode, etc).

A "segment" note makes frequency slide from the previous frequency to the new one during its duration

# MIDI Conversion

## MIDI

The velocity of a MIDI note will be used as its volume in WASM-4. 100 is the highest.

If multiple notes happen at the same time, they will be treated as either slides or arps. Multiple notes with the same velocity at the same time is invalid.

If arps are enabled, arps will happen from the note with the *lowest* velocity (with that velocity) to the note with the *highest* velocity. Notes cannot be added to a currently playing arp.

If arps are disabled, slides will happen from the note with the *lower* velocity (with that velocity) to the note with the *higher* velocity, with the slide duration being determined by the length of the note with the *higher* velocity. There can only be two notes in a slide at any time.

## Instrument Definition Parameters

Some values can be overridden using a formula.

Formulas are written as `value * mul + offset`
e.g.: `"freq": "value * 0.25 + 1"`, `"duration": "30"`

### Valid formula parameters
- freq
- slideToFreq *(Any non-zero value means slide will happen regardless if there is one in note data)*
- velocity
- duration

### Normal parameters
- channel (PULSE_1, PULSE_2, TRIANGLE, NOISE)
- pulseMode (12.5%, 25%, 50%, 75%)
- amplitude (0.0 - 1.0)
- arpSpeed (ticks, 0 = disabled)
- attack (ticks)
- decay (ticks)
- sustain (0.0 - 1.0)
- release (ticks)

# License

All code related to w4on (e.g. `w4on.h`, `w4on.c`) is licensed under MIT. See the LICENSE file for more details.