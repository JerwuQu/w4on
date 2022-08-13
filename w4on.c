#include "w4on.h"

#ifndef W4ON_TONE_EXTERN_NAME
#define W4ON_TONE_EXTERN_NAME tone
#endif

extern void (W4ON_TONE_EXTERN_NAME)(uint32_t frequency, uint32_t duration, uint32_t volume, uint32_t flags);

#ifdef DEBUG
	// TODO: set depending on platform
	#define W4ON_TRACEF tracef
#else
	#define W4ON_TRACEF(...)
#endif

#define MIN(a, b) ((a) < (b) ? (a) : (b))
#define MAX(a, b) ((a) > (b) ? (a) : (b))

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

static size_t readLength(const uint8_t *data, uint16_t *offset)
{
	size_t a = data[(*offset)++];
	return (a & 0x80) ? (((a & 0x7F) << 8) | data[(*offset)++]) : a;
}

static void instTone(w4on_seq_t *seq, uint8_t trkI, bool isSegment, uint32_t freq, size_t length)
{
	w4on_track_t *trk = &seq->tracks[trkI];
	uint32_t lengthParam;
	if (isSegment) {
		// segment notes only have release as to not retrigger attack/decay
		lengthParam = length | (trk->r << 8);
	} else {
		// normal notes will have full ADSR
		// NOTE: the release is not needed if there are segments after, but it also doesn't hurt, so it is kept for brevity
		// NOTE: this also adjust ADSR to behave more like normal ADSR
		const int32_t a = MIN(length, trk->a);
		const int32_t d = MAX(0, MIN((int32_t)length - a, (int32_t)trk->d));
		const int32_t len = MAX(0, (int32_t)length - a - d);
		lengthParam = len | (trk->r << 8) | (d << 16) | (a << 24);
	}
	if (trk->segmentsLeft) {
		// prevent pops between segments
		lengthParam += 2;
	}
	const size_t s = (size_t)trk->volume * (size_t)trk->s / 255;
	W4ON_TONE_EXTERN_NAME(
		freq,
		lengthParam,
		s | (trk->volume << 8),
		trk->channel | (trk->pulseMode << 2) | (trk->pan << 4)
	);
}

void w4on_seq_init(w4on_seq_t *seq, const uint8_t *data)
{
	size_t trkCount = data[0];
	size_t offset = 1;
	seq->data = data;
	for (size_t trkI = 0; trkI < trkCount; trkI++) {
		w4on_track_t *trk = &seq->tracks[trkI];

		const size_t trkSize = (data[offset] << 8) | data[offset + 1];
		offset += 2;

		const uint8_t flags = data[offset];
		trk->channel = flags & 0x3;
		trk->pulseMode = (flags >> 2) & 0x3;

		trk->dataI = offset + 1;
		offset += trkSize;
		trk->dataEndI = offset;

		trk->volume = 100;
		trk->a = trk->d = trk->r = 0;
		trk->s = 255;
		trk->arpSpeed = 1;

		trk->arpNoteDataI = 0;
		trk->eventValue = 0;
		trk->eventTick = 0;
		trk->eventTicksLeft = 0;

		W4ON_TRACEF("Track #%d | size %d | flags %d", (int)(trkI + 1), (int)trkSize, (int)flags);
	}
}

bool w4on_seq_tick(w4on_seq_t *seq)
{
	bool playing = false;
	size_t trkCount = seq->data[0];
	for (size_t trkI = 0; trkI < trkCount; trkI++) {
		w4on_track_t *trk = &seq->tracks[trkI];
		uint8_t segmentCount = 0;

		// Handle new events
		while (!trk->eventTicksLeft && trk->dataI < trk->dataEndI) {
			const uint8_t msg = seq->data[trk->dataI++];

			trk->eventTick = 0;
			trk->arpNoteDataI = 0;

			W4ON_TRACEF("Track: #%d, MSG: %d\n", (int)(trkI + 1), msg);
			if (msg == W4ON_MSG_ID_SET_ARP_SPEED_EX) {
				trk->arpSpeed = seq->data[trk->dataI++];
			} else if (msg >= W4ON_MSG_SPAN_SET_PAN) {
				trk->pan = msg - W4ON_MSG_SPAN_SET_PAN;
			} else if (msg == W4ON_MSG_ID_SET_R_EX) {
				trk->r = seq->data[trk->dataI++];
			} else if (msg == W4ON_MSG_ID_SET_S_EX) {
				trk->s = seq->data[trk->dataI++];
			} else if (msg == W4ON_MSG_ID_SET_D_EX) {
				trk->d = seq->data[trk->dataI++];
			} else if (msg == W4ON_MSG_ID_SET_A_EX) {
				trk->a = seq->data[trk->dataI++];
			} else if (msg == W4ON_MSG_ID_SET_VOLUME_EX) {
				trk->volume = seq->data[trk->dataI++];
			} else if (msg >= W4ON_MSG_SPAN_ARP_EX) {
				trk->eventValue = msg - W4ON_MSG_SPAN_ARP_EX + 2; // arp note count
				trk->eventTicksLeft = readLength(seq->data, &trk->dataI);
				trk->arpNoteDataI = trk->dataI;
				trk->dataI += trk->eventValue;
			} else if (msg >= W4ON_MSG_SPAN_SEGMENTS) {
				segmentCount = msg - W4ON_MSG_SPAN_SEGMENTS + 1; // NOTE: needs to surivive till first note comes (meaning immediately)
			} else if (msg >= W4ON_MSG_SPAN_NOTE_EX) {
				uint8_t newNote = msg - W4ON_MSG_SPAN_NOTE_EX;
				trk->eventTicksLeft = readLength(seq->data, &trk->dataI);
				if (trk->segmentsLeft) { // segment/slide
					trk->segmentsLeft--;
					instTone(seq, trkI, true, getNoteFreq(trk->eventValue) | (getNoteFreq(newNote) << 16), trk->eventTicksLeft);
				} else {
					instTone(seq, trkI, false, getNoteFreq(newNote), trk->eventTicksLeft);
					if (segmentCount) {
						trk->segmentsLeft = segmentCount;
					}
				}
				trk->eventValue = newNote;
			} else if (msg >= W4ON_MSG_SPAN_SHORT_WAIT) {
				trk->eventTicksLeft = msg - W4ON_MSG_SPAN_SHORT_WAIT + 1;
			} else if (msg == W4ON_MSG_ID_LONG_WAIT_EX) {
				trk->eventTicksLeft = readLength(seq->data, &trk->dataI) + W4ON_MSG_COUNT_SHORT_WAIT + 1;
			}
		}

		if (trk->eventTicksLeft) {
			playing = true; // TODO: consider note release

			// Arps
			if (trk->arpNoteDataI && (trk->eventTick % trk->arpSpeed) == 0) {
				size_t n = (trk->eventTick / trk->arpSpeed) % trk->eventValue;
				instTone(seq, trkI, false, getNoteFreq(seq->data[trk->arpNoteDataI + n]), trk->arpSpeed);
			}

			trk->eventTick++;
			trk->eventTicksLeft--;
		}
	}

	return playing;
}
