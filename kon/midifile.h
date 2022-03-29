#pragma once

#include <stdint.h>
#include <stdbool.h>
#include <stdio.h>

typedef struct {
	uint8_t trackCount;
} midi_file_t;

typedef struct {
	uint8_t status;
	uint8_t *data;
} midi_event_t;

midi_file_t *midi_open(const char *filename);
midi_file_t *midi_close(midi_file_t *mf);
bool midi_get_event(midi_file_t *mf, uint8_t track, midi_event_t *ev);
