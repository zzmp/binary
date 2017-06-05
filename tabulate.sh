#!/bin/bash

echo -e 'N\tMAX\tAVG'
cat solutions | grep ^$1 | awk -v OFS=$'\t' -f tabulate.awk
