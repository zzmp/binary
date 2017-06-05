#!/bin/bash

check() {
    let k=$1
    let n=$2
    vector=""
    for i in $(seq 0 $n); do
        if [[ "$i" -eq "$k" ]]; then
            vector+=1
        else
            vector+=0
        fi
    done
    let soln=`./solver -s $vector | awk '{print $NF}'`
    echo $k $n $vector $soln
}

for k in $(seq 0 $1); do
    check $k $1
done
