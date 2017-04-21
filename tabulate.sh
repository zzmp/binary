#!/bin/bash

get_stats() {
    cat solutions | grep ^$1 | awk -f tabulate.awk | xargs echo $1
}

echo N MIN MAX AVG
for i in {2..10}
do
    get_stats $i
done
