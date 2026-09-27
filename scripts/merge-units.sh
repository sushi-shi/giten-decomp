#!/bin/sh
# git merge driver for config/units.toml (see scripts/giten/tool/merge_units.py).
# Outside `nix develop` there is no python3: fall back to git's own text merge,
# which leaves ordinary conflict markers.
if command -v python3 >/dev/null 2>&1; then
    exec python3 scripts/giten/tool/merge_units.py "$1" "$2" "$3"
fi
exec git merge-file -L ours -L base -L theirs "$2" "$1" "$3"
