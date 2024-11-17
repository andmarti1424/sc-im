
#!/usr/bin/env -S bash
## SEE https://github.com/lehmannro/assert.sh for usage

#Exit immediately if a command exits with a non-zero status.
set -e

NAME=test-getent-range
#SCIM=sc-im
SCIM=../src/sc-im

VALGRIND_CMD='valgrind -v --log-file=${NAME}_vallog --tool=memcheck --track-origins=yes --leak-check=full --show-leak-kinds=all --show-reachable=no'
. assert.sh

CMD='
LET B0 = @sum(A0:@getent(D0,0))\n
LET B1 = @sum(A0:@getent(@lastrow,0))\n
RECALC\n
GETNUM B0\n
GETNUM B1\n
LET D0=2\n
LET A3=4\n
RECALC\n
GETNUM B0\n
GETNUM B1\n
YANKAREA {"Sheet1"}!B0:B0 "a"\n
GOTO B2\n
PASTEYANKED {"Sheet1"} 0 "c"\n
GETNUM B2
'
EXP="
3\n\
6\n\
6\n\
10\n\
6
"
check_log(){
#we check valgrind log
assert_iffound_notcond ${NAME}_vallog "definitely lost.*bytes" "0 bytes"
assert_iffound_notcond ${NAME}_vallog "indirectly lost.*bytes" "0 bytes"
assert_iffound_notcond ${NAME}_vallog "possibly lost.*bytes" "0 bytes"
assert_iffound_notcond ${NAME}_vallog "Uninitialised value was created by a heap allocation"
assert_iffound_notcond ${NAME}_vallog "Conditional jump or move depends on uninitialised value"
assert_iffound_notcond ${NAME}_vallog "Invalid read of size"
assert_iffound_notcond ${NAME}_vallog "Invalid write of size"
assert_iffound_notcond ${NAME}_vallog "Invalid free() / delete"
}
#echo -e "${CMD}" | ../src/sc-im ${NAME}.sc --nocurses --nodebug --quit_afterload 2>&1 |grep -v '^$\|Interp\|Change\|wider'
#exit
assert "echo -e '${CMD}' | $VALGRIND_CMD ${SCIM} ${NAME}.sc  --nocurses --nodebug --quit_afterload 2>&1" $EXP
check_log

if [ "$1" != "keep-vallog" ];then
   rm ${NAME}_vallog
fi

assert_end ${NAME}
