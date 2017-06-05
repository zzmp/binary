#!/bin/bash

get_stats() {
    cat solutions | grep ^$1 | awk -v OFS=$'\t' -f tabulate.awk
}

echo -e 'N\tMAX\tAVG'
for i in {2..10}
do
    get_stats $i
done
