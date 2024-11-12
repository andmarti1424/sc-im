#!/usr/bin/env bash
#Exit immediately if a command exits with a non-zero status.
set -e

for i in test*.sh
do
    echo "Testing $i"
    /usr/bin/env -S bash $i keep-vallog
done
