#!/usr/bin/env -S bash
## SEE https://github.com/lehmannro/assert.sh for usage

#Exit immediately if a command exits with a non-zero status.
set -e

NAME=test-replace

VALGRIND_CMD='valgrind -v --log-file=${NAME}_vallog --tool=memcheck --track-origins=yes --leak-check=full --show-leak-kinds=all --show-reachable=no'
. assert.sh

CMD='
label A0 = "aaa:bb"\n
label A1 = @replace(a0,":","c")\n
label A2 = @replace(a0,"","c")\n
label A3 = @replace(a0,"z","c")\n
label A4 = @replace(a0,":","")\n
label A5 = @replace(a5,":","c")\n
recalc\n
label A5 = @replace(a5,":","c")\n
recalc\n
getstring a0\n
getstring a1\n
getstring a2\n
getstring a3\n
getstring a4\n
getstring a5\n
'
CMD2='
let C1 = 3\n
let D1 = 4\n
let E1 = 1\n
let F1 = 2\n
let B54 = 5\n
rightstring B55 = @coltoa(B54)#"1"\n
rightstring B56 = @replace("@sum(d1:0)","0",B55)\n
let B57 = @evaluate(B56)\n
recalc\n
getnum b57
'
export IFS=$'\n'
EXP="\
Circular reference in seval\n\
Illegal string expression\n\
Circular reference in seval\n\
Illegal string expression\n\
Illegal string expression\n\
aaa:bb\n\
aaacbb\n\
\n\
aaa:bb\n\
aaabb
"
assert "echo -e '${CMD}' | $VALGRIND_CMD ../src/sc-im --nocurses --nodebug --quit_afterload 2>&1" $EXP
#we check valgrind log
assert_iffound_notcond ${NAME}_vallog "definitely lost.*bytes" "0 bytes"
assert_iffound_notcond ${NAME}_vallog "indirectly lost.*bytes" "0 bytes"
assert_iffound_notcond ${NAME}_vallog "possibly lost.*bytes" "0 bytes"
assert_iffound_notcond ${NAME}_vallog "Uninitialised value was created by a heap allocation"
assert_iffound_notcond ${NAME}_vallog "Conditional jump or move depends on uninitialised value"
assert_iffound_notcond ${NAME}_vallog "Invalid read of size"
assert_iffound_notcond ${NAME}_vallog "Invalid write of size"
assert_iffound_notcond ${NAME}_vallog "Invalid free() / delete"
assert "echo -e '${CMD2}' |../src/sc-im --nocurses --nodebug --quit_afterload 2>&1" 7
if [ "$1" != "keep-vallog" ];then
   rm ${NAME}_vallog
fi

assert_end ${NAME}
