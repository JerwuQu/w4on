// https://www.midi.org/specifications-old/item/table-1-summary-of-midi-message
// http://www.somascape.org/midi/tech/mfile.html

const fs = require('fs');
const midiParser = require('./midi-parser-js');

// Generated by protospan.js
const W4ON_MSG_ID_LONG_WAIT_EX = 0x00; // ExLength + W4ON_MSG_COUNT_SHORT_WAIT
const W4ON_MSG_SPAN_SHORT_WAIT = 0x01;
const W4ON_MSG_COUNT_SHORT_WAIT = 120;
const W4ON_MSG_SPAN_NOTE_EX = 0x79; // ExLength
const W4ON_MSG_COUNT_NOTE = 88;
const W4ON_MSG_SPAN_SEGMENTS = 0xd1;
const W4ON_MSG_COUNT_SEGMENTS = 16;
const W4ON_MSG_SPAN_ARP_EX = 0xe1; // ExLength + 2, [notes...]
const W4ON_MSG_COUNT_ARP = 16;
const W4ON_MSG_ID_SET_VOLUME_EX = 0xf1; // [8:volume]
const W4ON_MSG_ID_SET_A_EX = 0xf2; // [8:A]
const W4ON_MSG_ID_SET_D_EX = 0xf3; // [8:D]
const W4ON_MSG_ID_SET_S_EX = 0xf4; // [8:S]
const W4ON_MSG_ID_SET_R_EX = 0xf5; // [8:A]
const W4ON_MSG_SPAN_SET_PAN = 0xf6;
const W4ON_MSG_COUNT_SET_PAN = 3;
const W4ON_MSG_ID_SET_ARP_SPEED_EX = 0xf9; // [8:ArpSpeed]
const W4ON_MSG_RESERVED = 0xfa;
// Unused values: 5

// Unused values: 9

// - - - - -
const CHANNEL_NAMES = {'PULSE_1': 0, 'PULSE_2': 1, 'TRIANGLE': 2, 'NOISE': 3};
const PULSE_MODES = {'12.5%': 0, '25%': 1, '50%': 2, '75%': 3};
// - - - - -
// Used for tempo adjustment
const NOTES_PER_BEAT = 4; // TODO: customizable
// - - - - -
const pushExtLength = (arr, len) => {
    if (len > 0x7fff) {
        throw `Length too long`;
    } else if (len > 0x7f) {
        arr.push((len >> 8) | 0x80);
        arr.push(len & 0xff);
    } else {
        arr.push(len);
    }
};
// - - - - -

const midiInFile = process.argv[2];
const instrSpecFile = process.argv[3];
const w4onOutFile = process.argv[4];
if (!midiInFile || !instrSpecFile) {
    console.error('convert <in.mid> <instr.json> <out.w4on>')
    process.exit(1);
}

const instrSpec = JSON.parse(fs.readFileSync(instrSpecFile));
const midi = midiParser.parse(fs.readFileSync(midiInFile));

let tickInaccuracy = 0;
const trackInfos = [];
const trackDataBufs = [];
const trackNotes = [];
let originalBPM = null;
let midiTickDivisor = null;
midi.track.forEach(track => {
    // Iterate over midi events to get all notes
    const channelInfos = {};
    let trackName = 'Unnamed';
    let tick = 0;
    let pan = 0;
    track.event.forEach(event => {
        if (event.deltaTime > 0 && !originalBPM) {
            throw `BPM needs to be set in MIDI file`;
        }
        tick += event.deltaTime;
        if (event.type === 255 && event.metaType === 3) { // Track Name
            trackName = event.data;
        } else if (event.type === 255 && event.metaType === 81) { // Tempo
            const newBPM = 60000000 / event.data;
            if (newBPM !== originalBPM) {
                if (originalBPM) throw `BPM changes are not allowed`;
                originalBPM = newBPM;
                const bestTickWait = Math.round(3600 / (originalBPM * NOTES_PER_BEAT));
                const bestBPM = 3600 / (bestTickWait * NOTES_PER_BEAT);
                midiTickDivisor = midi.timeDivision / NOTES_PER_BEAT / bestTickWait;
                console.log('Original BPM:', originalBPM, ' Best tick wait:', bestTickWait, ' Best BPM:', bestBPM, ' Midi tick divisor:', midiTickDivisor);
            }
        } else if (event.type === 9 && event.data[1] > 0) { // Note On
            const note = event.data[0];
            const channel = event.channel;
            channelInfos[channel] ??= { rawNotes: [], noteStarts: [] }
            const { noteStarts } = channelInfos[channel];
            noteStarts[note] = {tick, velocity: event.data[1]};
        } else if (event.type === 8 || (event.type === 9 && event.data[1] == 0)) { // Note off
            const note = event.data[0];
            if (note < 21 || note > 127) {
                throw `Invalid note number ${note} (${trackName})`;
            } else if (note > 108) {
                throw `Only the 88 piano notes are allowed (${trackName})`;
            }
            const channel = event.channel;
            const { rawNotes, noteStarts } = channelInfos[channel];
            const noteStart = noteStarts[note];
            const startTick = noteStart.tick;
            rawNotes.push({
                tick: startTick,
                length: tick - startTick,
                note: note - 21, // This means notes range from A0 (0) to C8 (87)
                velocity: noteStart.velocity,
                pan: pan,
            });
        } else if (event.type === 11 && event.data[0] == 10) { // Pan
            pan = event.data[1] < 43 ? 1 : (event.data[1] > 84 ? 2 : 0);
        } else {
            // console.log(`Unhandled event type: ${event.type}, data: ${event.data}`)
        }
    });
    const rawChannels = Object.keys(channelInfos);
    if (rawChannels.length == 1) {
        const channel = rawChannels[0];
        const { rawNotes } = channelInfos[channel];
        trackNotes.push({ rawNotes, trackName });
        return;
    }
    for (const channel of rawChannels) {
        const { rawNotes } = channelInfos[channel];
        trackNotes.push({ rawNotes, trackName: `${trackName} (channel ${channel})` });
    }
});

trackNotes.forEach(({ rawNotes, trackName }) => {
    // Sanity
    if (!rawNotes.length) return;

    // Sort notes by tick -> velocity -> note
    rawNotes.sort((a, b) => {
        const tickDiff = a.tick - b.tick;
        if (tickDiff !== 0) return tickDiff;
        const velDiff = a.velocity - b.velocity;
        if (velDiff !== 0) return velDiff;
        const noteDiff = a.note - b.note;
        if (noteDiff !== 0) return noteDiff;
    });

    // Tick division
    for (const note of rawNotes) {
        tickInaccuracy += Math.abs((note.tick / midiTickDivisor) - Math.round(note.tick / midiTickDivisor));
        note.tick = Math.round(note.tick / midiTickDivisor);
        note.length = Math.round(note.length / midiTickDivisor);
    }

    // Get instrument for track
    const instr = instrSpec[trackName]
    if (!instr) {
        console.warn(`No instrument specified for ${trackName}, skipping`);
        return;
    }

    // Data buffer
    const trackDataBuf = [];
    trackDataBufs.push(trackDataBuf);

    // Required instrument values
    if (CHANNEL_NAMES[instr.channel] === undefined) throw `Invalid channel ${instr.channel}`;
    instr.channel = CHANNEL_NAMES[instr.channel];
    if (instr.pulseMode && PULSE_MODES[instr.pulseMode] === undefined) throw `Invalid pulseMode ${instr.pulseMode}`;
    instr.pulseMode = PULSE_MODES[instr.pulseMode] ?? 0;
    instr.amplitude ??= 1;

    // Write instrument data
    trackDataBuf.push((instr.pulseMode << 2) | instr.channel);
    if (instr.a) {
        trackDataBuf.push(W4ON_MSG_ID_SET_A_EX);
        trackDataBuf.push(instr.a);
    }
    if (instr.d) {
        trackDataBuf.push(W4ON_MSG_ID_SET_D_EX);
        trackDataBuf.push(instr.d);
    }
    if (instr.s && instr.s !== 1) {
        trackDataBuf.push(W4ON_MSG_ID_SET_S_EX);
        trackDataBuf.push(Math.round(instr.s * 255));
    }
    if (instr.r) {
        trackDataBuf.push(W4ON_MSG_ID_SET_R_EX);
        trackDataBuf.push(instr.r);
    }
    if (instr.arpSpeed && instr.arpSpeed > 1) {
        trackDataBuf.push(W4ON_MSG_ID_SET_ARP_SPEED_EX);
        trackDataBuf.push(instr.arpSpeed);
    }

    // Convert notes to events (notes/slides/arps)
    // TODO: instrument formula support
    const chEvents = [];
    for (let noteI = 0; noteI < rawNotes.length; noteI++) {
        const note = rawNotes[noteI];
        const lastEvent = chEvents.length > 0 ? chEvents[chEvents.length - 1] : null;
        let arpedCount = 0;
        while (instr.arpSpeed && rawNotes[noteI + arpedCount + 1]?.tick === note.tick) arpedCount++;
        if (arpedCount > 0) { // Arp
            const arpNotes = [note.note];
            for (let arpI = 0; arpI < arpedCount; arpI++) {
                arpNotes.push(rawNotes[noteI + arpI + 1].note);
            }
            chEvents.push({
                tick: note.tick,
                length: note.length,

                velocity: Math.max(0, Math.min(100, note.velocity)),
                pan: note.pan,

                arp: arpNotes,
            });
            noteI += arpedCount; // Skip
        } else if (lastEvent && lastEvent.tick + lastEvent.length > note.tick) { // Slide
            const overlapTicks = (lastEvent.tick + lastEvent.length) - note.tick;
            const aLen = lastEvent.length - lastEvent.segLength - overlapTicks;
            if (aLen < 0) throw `Invalid slide note for ${trackName}`;
            lastEvent.segments.push({
                note: lastEvent.note,
                length: aLen,
            });
            lastEvent.segments.push({
                note: note.note,
                length: overlapTicks,
            });
            lastEvent.segLength += aLen + overlapTicks; // Total length of all segments (excluding tail)
            lastEvent.note = note.note; // Tail segment note
            lastEvent.length += note.length - overlapTicks; // Full length
        } else { // Note
            chEvents.push({
                tick: note.tick,
                length: note.length,

                note: note.note,
                velocity: Math.max(0, Math.min(100, note.velocity)),
                pan: note.pan,

                segments: [],
                segLength: 0,
            });
        }
    }

    // Track info
    const trkInfo = {
        name: trackName,
        notes: 0,
        segs: 0,
        arps: 0,
        arpNotes: 0,
        waits: 0,
        firstTick: chEvents[0].tick,
        lastTick: chEvents[chEvents.length - 1].tick + chEvents[chEvents.length - 1].length,
        changes: {
            velocity: 0,
            pan: 0,
        },
    };
    trackInfos.push(trkInfo);

    // Convert events to w4on data
    let curVel = instr.amplitude === 1 ? 1 : -1; // NOTE: velocity needs to be multipled with amplitude
    let curPan = 0;
    let lastEndTick = 0;
    for (let evtI = 0; evtI < chEvents.length; evtI++) {
        const note = chEvents[evtI];

        // Handle overlap (trim earlier note length)
        const nextNote = chEvents[evtI + 1];
        if (nextNote && note.tick + note.length > nextNote.tick) {
            note.length = nextNote.tick - note.tick;
            console.warn(`Corrected overlapping note for ${trackName}`);
        }

        // Convert to w4on binary
        // - Insert wait
        let waitTicks = note.tick - lastEndTick;
        while (waitTicks > 0) {
            trkInfo.waits++;
            if (waitTicks - 1 < W4ON_MSG_COUNT_SHORT_WAIT) {
                trackDataBuf.push(W4ON_MSG_SPAN_SHORT_WAIT + waitTicks - 1);
                waitTicks = 0;
            } else {
                trackDataBuf.push(W4ON_MSG_ID_LONG_WAIT_EX);
                pushExtLength(trackDataBuf, waitTicks - W4ON_MSG_COUNT_SHORT_WAIT - 1);
                waitTicks = 0; // TODO: handle super long waits
            }
        }
        lastEndTick = note.tick + note.length;

        // - Pan change
        if (note.pan !== curPan) {
            trkInfo.changes.pan++;
            curPan = note.pan;
            trackDataBuf.push(W4ON_MSG_SPAN_SET_PAN + curPan);
        }

        // - Velocity change
        if (note.velocity !== curVel) {
            trkInfo.changes.velocity++;
            curVel = note.velocity;
            trackDataBuf.push(W4ON_MSG_ID_SET_VOLUME_EX);
            trackDataBuf.push(Math.round(curVel * instr.amplitude));
        }

        // - Insert note
        if (note.arp && note.arp.length > 0) { // Arp
            trkInfo.arps++;
            trkInfo.arpNotes += note.arp.length;
            if (note.arp.length - 2 >= W4ON_MSG_COUNT_ARP) throw 'Too many arp notes';
            trackDataBuf.push(W4ON_MSG_SPAN_ARP_EX + note.arp.length - 2);
            pushExtLength(trackDataBuf, note.length);
            for (const arpNote of note.arp) {
                trackDataBuf.push(arpNote);
            }
        } else if (note.segments && note.segments.length > 0) { // Slide
            const hasTail = note.length - note.segLength > 0;
            const segCount = note.segments.length - (hasTail ? 0 : 1);
            if (segCount > W4ON_MSG_COUNT_SEGMENTS) throw 'Too many segments';
            trkInfo.notes++;
            trkInfo.segs += segCount;
            if (segCount === 0) throw 'Sanity lost';
            trackDataBuf.push(W4ON_MSG_SPAN_SEGMENTS + segCount - 1 /*-1 because format says so*/);
            for (let i = 0; i < note.segments.length; i++) {
                const seg = note.segments[i];
                trackDataBuf.push(W4ON_MSG_SPAN_NOTE_EX + seg.note);
                pushExtLength(trackDataBuf, seg.length);
            }
            if (hasTail) {
                trackDataBuf.push(W4ON_MSG_SPAN_NOTE_EX + note.note);
                pushExtLength(trackDataBuf, note.length - note.segLength);
            }
        } else { // Note
            trkInfo.notes++;
            trackDataBuf.push(W4ON_MSG_SPAN_NOTE_EX + note.note);
            pushExtLength(trackDataBuf, note.length);
        }
    }
});

// Concat track data
const fileDataBuf = [trackDataBufs.length];
for (const trkData of trackDataBufs) {
    if (trkData.length > 0xffff) throw `Too much track data`;
    fileDataBuf.push(trkData.length >> 8);
    fileDataBuf.push(trkData.length & 0xff);
    fileDataBuf.push(...trkData);
}
const fileData = new Uint8Array(fileDataBuf);

// Print stats
for (let i = 0; i < trackDataBufs.length; i++) {
    console.log(`
Track #${i + 1}/${trackDataBufs.length} (${trackInfos[i].name})
    data size: ${trackDataBufs[i].length}
    channel flags: ${trackDataBufs[i][0]}
    notes: ${trackInfos[i].notes}
    segments: ${trackInfos[i].segs}
    arps: ${trackInfos[i].arps} (${trackInfos[i].arpNotes} notes)
    waits: ${trackInfos[i].waits}
    first tick: ${trackInfos[i].firstTick}, last tick: ${trackInfos[i].lastTick}
    changes:
        velocity: ${trackInfos[i].changes.velocity}
        pan: ${trackInfos[i].changes.pan}`);
}
console.log(`
Total data size: ${fileData.length}
Tick inaccuracy: ${tickInaccuracy}
`)

if (w4onOutFile) {
    fs.writeFileSync(w4onOutFile, Buffer.from(fileData), 'binary');
    console.log('File written');
} else {
    console.warn('No output file given, skipping');
}