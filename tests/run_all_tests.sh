#!/bin/bash
for i in test*.sh
do
    echo "Testing $i"
    /bin/bash $i
done
