#!/usr/bin/env bash
for i in test*.sh
do
    echo "Testing $i"
    /usr/bin/env -S bash $i keep-vallog
done
