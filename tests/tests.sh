#!/bin/bash

## SEE https://github.com/lehmannro/assert.sh for usage


set -e

ROOTDIR=`pwd`
. tests/assert.sh

rm -f str.md
cd examples/csv
assert_raises "cat csv2markdown.sc | ../../src/sc-im --nocurses" 0 ""
assert "cat str.md|wc -l|awk '{\$1=\$1};1'" "23"
rm -f str.md
cd $ROOTDIR

assert_end csv

#rm -f str.md
#cd examples/xlsx
#assert_raises "cat xlsx2markdown.sc | ../../src/sc-im --nocurses" 0 ""
#assert "cat str.md|wc -l|awk '{\$1=\$1};1'" "114"
#rm -f str.md
#cd $ROOTDIR
#
#assert_end xlsx
