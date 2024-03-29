const proto = [
	// Note
	['LONG_WAIT', 1, 'ExLength + W4ON_MSG_COUNT_SHORT_WAIT'],
	['SHORT_WAIT', 120],
	['NOTE', 88, 'ExLength'],
	// next n+1 notes after the first one will be segments
	// note that the first must come immediately after in stream
	['SEGMENTS', 16],
	['ARP', 16, 'ExLength + 2, [notes...]'],
	// Instrument adjustments
	['SET_VOLUME', 1, '[8:volume]'],
	['SET_A', 1, '[8:A]'],
	['SET_D', 1, '[8:D]'],
	['SET_S', 1, '[8:S]'],
	['SET_R', 1, '[8:A]'],
	['SET_PAN', 3],
	['SET_ARP_SPEED', 1, '[8:ArpSpeed]'],
];

const formats = {
	c: (name, value, comment) => `#define ${name} ${value}${comment ? ' // ' + comment : ''}`,
	js: (name, value, comment) => `const ${name} = ${value};${comment ? ' // ' + comment : ''}`,
};
const format = formats[process.argv[2]];
if (!format) {
	throw `node protospan.js <c/js>`
}

let b = 0;
console.log('// Generated by protospan.js');
for (const p of proto) {
	console.log(format(`W4ON_MSG_${p[1] > 1 ? 'SPAN' : 'ID'}_${p[0]}${p[2] ? '_EX' : ''}`, '0x' + b.toString(16).padStart(2, '0'), p[2]));
	if (p[1] > 1) {
		console.log(format(`W4ON_MSG_COUNT_${p[0]}`, p[1]));
	}
	b += p[1];
}
console.log(format('W4ON_MSG_RESERVED', '0x' + b.toString(16).padStart(2, '0')));
console.log(`// Unused values: ${255 - b}\n`);
