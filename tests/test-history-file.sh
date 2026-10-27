#!/usr/bin/env bash
set -e

# Python's standard-library pty module drives the actual curses UI. The
# existing --nocurses tests do not exercise normal startup/shutdown history.
python3 "$(dirname "$0")/test-history-file.py"
