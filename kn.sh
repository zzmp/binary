#!/bin/bash

check() {
    let k=$1
    let n=$2
    vector=""
    for i in $(seq 0 $n); do
        if [[ "$i" -lt "$k" ]]; then
            vector+=0
        else
            vector+=1
        fi
    done
    let soln=`./solver -s $vector | awk '{print $NF}'`
    echo $k $n $vector $soln
    if [[ $(($k * ($n - $k + 1))) -ne "$soln" ]]; then
        echo Failed check, should be $(($k * ($n - $k + 1)))
        exit 1
    fi
}

for k in $(seq 1 $1); do
    check $k $1
done
