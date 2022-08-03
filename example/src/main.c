#include "wasm4.h"
#include "../../w4on.c"

#include "../midi/wasm4nia_theme.c"

w4on_seq_t song;

void start()
{
	w4on_seq_init(&song, w4on_data_wasm4nia_theme);
}

void update()
{
	w4on_seq_tick(&song);
}
