#!/usr/bin/env bash

hexdump -X $1 | sed 's/\( \)\{3\}\+//g' | sed 's-  -, 0x-g' | sed 's/[0-9a-fA-F]\{7\}/\t.byte/g' | sed 's/byte,/byte/g'> $2
