#! /bin/sh
#
# List all symbol names in text files. Members fo C structures, matching
# '.name' or '->name', are printed as '.name', i.e. with a '.' prefix.
#
# Arguments are an arbitrary list of files.
#

export LANG="C"
pgm=$(basename "$0" ".sh")

usage() {
    echo >&2 "usage: $pgm FILE [...]"
    exit 1
}

if test $# -lt 1; then
    usage
fi

#yorick="no"
#while test $# -ge 1; do
#    arg="$1"
#    if test "$arg" = "--"; then
#        shift
#        break
#    elif test "$arg" = "--yorick"; then
#        yorick="yes"
#        shift
#    else
#        break
#    fi
#done
#echo >&2 "args: $*"

tmp=$(mktemp -p "$TMPDIR" "$pgm-XXXXXX")

on_exit() {
    rm -f "$tmp"
}

trap on_exit 0

append_names1() {
    # Apply the following:
    #  - suppress floating-point constants
    #  - suppress integer constants
    #  - replace ':' and tabs by a space
    #  - replace '(\.|->)([_A-Za-z])' by ':\1'
    #  - replace all non [:_A-Za-z0-9] by a space
    #  - delete non words
    #  - suppress leading and trailing spaces
    #  - delete empty lines
    #  - replace spaces by a newline
    #  - uniquely sort the result
    sed <"$1" \
        -e 's/\(^\|[^_A-Za-z0-9]\)\(\(\([1-9][0-9]*\|[0-9]\)\(\.[0-9]*\)\?\)\|\.[0-9]\+\)\([eEdD][-+]\?[0-9]\+\)\?\([^_A-Za-z0-9]\|$\)/\1 \7/g' \
        -e 's/\(^\|[^_A-Za-z0-9]\)\(0[Xx][0-9A-Fa-f]\+\|0[0-7]*\|[1-9][0-9]*\)[uUlL]*\([^_A-Za-z0-9]\|$\)/\1 \3/g' \
        -e 's/[:\t]\+/ /g' \
        -e 's/\(->\|\.\)\([_A-Za-z]\)/ :\2/g' \
        -e 's/[^:_A-Za-z0-9]\+/ /g' \
        -e 's/\(^\| \):\?[^_A-Za-z][^ ]*\( \|$\)/ /g' \
        -e 's/^ \+//;s/ \+$//' \
        -e '/^ *$/d' \
        -e 's/ \+/\n/g' \
        | sort -u >>"$tmp"
}

suppress_floating_point_constants() {
    sed -e 's/\(^\|[^_A-Za-z0-9]\)\(\(\([1-9][0-9]*\|[0-9]\)\(\.[0-9]*\)\?\)\|\.[0-9]\+\)\([eEdD][-+]\?[0-9]\+\)\?\([^_A-Za-z0-9]\|$\)/\1 \7/g'
}

suppress_integer_constants() {
    sed -e 's/\(^\|[^_A-Za-z0-9]\)\(0[Xx][0-9A-Fa-f]\+\|0[0-7]*\|[1-9][0-9]*\)[uUlL]*\([^_A-Za-z0-9]\|$\)/\1 \3/g'
}

append_names() {
    # Apply the following:
    #  - replace all non [-+.>_A-Za-z0-9] by a space
    #  - delete empty lines and restart with next line
    #  - suppress floating-point constants (because exponent can be confused
    #    with a name)
    #  - replace '(\.|->)([_A-Za-z])' by ' :\2'
    #  - replace all non [:_A-Za-z0-9] by a space
    #  - delete non words
    #  - suppress leading and trailing spaces
    #  - delete empty lines
    #  - replace ':' by '.' (could also be '->' or whatever)
    #  - replace spaces by a newline
    #  - uniquely sort the result
    sed -r <"$1" \
        -e 's/[^-+.>_A-Za-z0-9]+/ /g' \
        -e '/^ *$/d' \
        -e 's/(^|[^_A-Za-z0-9])((([1-9][0-9]*|[0-9])(\.[0-9]*)?)|\.[0-9]+)([eEdD][-+]?[0-9]+)?([^_A-Za-z0-9]|$)/\1 \7/g' \
        -e 's/(\.|->)([_A-Za-z])/ :\2/g' \
        -e 's/[^:_A-Za-z0-9]+/ /g' \
        -e 's/(^| )[^:_A-Za-z][^ ]*/\1 /g' \
        -e '/^ *$/d' \
        -e 's/^ +//;s/ \+$//' \
        -e 's/:+/./g' \
        -e 's/ +/\n/g' \
        | sort -u >>"$tmp"
}

while test $# -ge 1; do
    append_names "$1"
    shift
done

sort -u <"$tmp"
