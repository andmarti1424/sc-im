#!/bin/bash
## SEE https://github.com/lehmannro/assert.sh for usage

#Exit immediately if a command exits with a non-zero status.
set -e

NAME=test4

VALGRIND_CMD='valgrind -v --log-file=${NAME}_vallog --tool=memcheck --track-origins=yes --leak-check=full --show-leak-kinds=all --show-reachable=no'
. assert.sh

CMD='LET A2 = 1\nUNDO\nGETNUM A0\nGETNUM {"dos"}!B3'
assert "echo -e '${CMD}' | $VALGRIND_CMD ../src/sc-im ${NAME}.sc --nocurses --nodebug --quit_afterload 2>&1 |grep -v '^$\|Interp\|left\|Change'" "2.3\n0"

#TODO: check valgrind log here
#"in use at exit: 0 bytes in 0 blocks"
#"All heap blocks were freed -- no leaks are possible"
#rm ${NAME}_vallog

assert_end ${NAME}
