#! /bin/sh
#
# List all Yorick sources (including documentation, scripts, graphic styles).
#
# Arguments can be be an arbitrary list of files or directories.
# Without arguments, "." is assumed.  GIT files ".git*" are omitted.
#

export LANG="C"

pgm=$(basename "$0" ".sh")

tmp=$(mktemp "/tmp/$pgm-XXXXXX.tmp")

on_exit() {
    rm -f "$tmp"
}

trap on_exit 0

if test $# -eq 0; then
    find . -type f ! -path '*/.git*' >"$tmp"
else
    find "$@" -type f ! -path '*/.git*' >"$tmp"
fi

sed -n <"$tmp" \
    -e '/\.\([hc]\|cpp\|gs\|sh\|md\|txt\)$/{b print}' \
    -e '/\(\/\|^\)Make[^/]*$/{b print}' \
    -e '/\(\/\|^\)[A-Z][A-Z0-9]*\(\|\.[^/]*\)$/{b print}' \
    -e '/\(\/\|^\)configure$/{b print}' \
    -e 'd; b' \
    -e ':print; p' \
   | sort
