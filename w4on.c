#include "w4on.h"

// (   = ´  _ `=)

extern void tone(uint32_t frequency, uint32_t duration, uint32_t volume, uint32_t flags);

void w4on_seq_init(w4on_seq_t *seq, const uint8_t *data)
{
	seq->data = data;
	seq->chns[0].note = 0;
	seq->chns[1].note = 0;
	seq->chns[2].note = 0;
	seq->chns[3].note = 0;

	// TODO set channel data points
}

void w4on_seq_tick(w4on_seq_t *seq)
{
}
