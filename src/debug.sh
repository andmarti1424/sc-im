#!/usr/bin/env bash
tmux splitw -h -p 35 "gdbserver :12345 ./sc-im"
tmux selectp -t 0
cgdb -x gdb.gdb
