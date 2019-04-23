#! /bin/sh
#
# List all file extensions.
#
# Arguments can be be an arbitrary list of files or directories.
# Without arguments, "." is assumed.  GIT files ".git*" are omitted.
#

pgm=$(basename "$0" ".sh")

if test $# -eq 0; then
    find . -type f ! -path '*/.git*' \
        | sed '/\.[^/]*$/!d;s/.*\(\.[^/]*\)$/\1/' \
        | sort -u
else
    find "$@" -type f ! -path '*/.git*' \
        | sed '/\.[^/]*$/!d;s/.*\(\.[^/]*\)$/\1/' \
        | sort -u
fi
