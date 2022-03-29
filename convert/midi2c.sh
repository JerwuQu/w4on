#!/bin/sh
set -e
bin2c() { #array_name, in_file, out_file
	printf "unsigned char %s[] = {\n%s\n};\n" "$1" "$(xxd -i < "$2")" > "$3"
}
node "$(dirname "$0")/convert.js" "$2" "$3" "$4.w4on"
bin2c "$1" "$4.w4on" "$4"