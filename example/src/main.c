#include "wasm4.h"
#include "../../w4on.c"

#include "wasm4nia_theme.h"

w4on_seq_t song;

void start()
{
	w4on_seq_init(&song, wasm4nia_theme);
}

void update()
{
	w4on_seq_tick(&song);
}
