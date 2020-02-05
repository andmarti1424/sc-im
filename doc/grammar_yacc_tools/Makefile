all: parse_yacc command_list setitem_list term_list

parse_yacc:
	./y2l ../../src/gram.y | pandoc -f latex -t markdown | \
		sed 's/^[[:space:]][[:space:]]//g' |\
		sed 's/[S\_|K\_]\\_//g' | \
		sed 's/\\_/_/g' | \
		sed 's/[\*\*|\$$|"]//g' |\
		sed 's/[[:space:]]e[[:space:]]/\{expr\}/g' | \
		sed 's/[[:space:]]e$$/\{expr\}/g'|\
		sed 's/@[[:space:]]/@/g'|\
		sed 's/::=[[:space:]]+//g'|\
		sed 's/::=[[:space:]]!//g'|\
		sed 's/[[:space:]]var_or_range/\ {var_or_range}/g' |\
		sed 's/[[:space:]]var/\ {var}/g' |\
		sed 's/[[:space:]]expr_list/\ {expr_list}/g' |\
		sed 's/[[:space:]]range/\ {range}/g' |\
		sed 's/[[:space:]]COL/\ {COL}/g' |\
		sed 's/[[:space:]]NUMBER/\ {NUMBER}/g' |\
		sed 's/[[:space:]]STRING/\ {STRING}/g' |\
		sed 's/[[:space:]]strarg/\ {strarg}/g' |\
		sed 's/[[:space:]]WORD/\ {WORD}/g' |\
		sed 's/[[:space:]]setitem/\ {setitem}/g' |\
		sed 's/[[:space:]]setlist/\ {setlist}/g' |\
		sed 's/[[:space:]]num/\ {num}/g' |\
		sed 's/{COL}OR/COLOR/g' |\
		sed 's/[[:space:]]$$//g'|\
		sed 's/xxxxx/yyyyy/g' > yacc-format-stage1

command_list:
	grep -i "^command" ./yacc-format-stage1 | cut -d"=" -f2| sed 's/^[[:space:]]//g' > commands.txt

setitem_list:
	grep -i "^setitem" ./yacc-format-stage1 | cut -d"=" -f2,3| sed 's/^[[:space:]]//g' > setitems.txt

term_list:
	grep -i "^term" ./yacc-format-stage1 | cut -d"=" -f2| sed 's/^[[:space:]]//g' > term_list.txt
