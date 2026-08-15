#!/bin/sh

PLATFORM="m17"
SDCARD_PATH="/sdcard"
SYSTEM_PATH="$SDCARD_PATH/.system/$PLATFORM"
PAK_PATH="$(dirname "$0")"

export LD_LIBRARY_PATH="$SYSTEM_PATH/lib:/usr/lib:$LD_LIBRARY_PATH"
export PATH="$SYSTEM_PATH/bin:/usr/bin:$PATH"

# The ARM binary is intentionally not shipped until built against the M17 sysroot.
exec "$PAK_PATH/pocket-signer.elf"