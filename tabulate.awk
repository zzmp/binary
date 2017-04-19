BEGIN {
    MIN=1000
    MAX=   0
    SUM=   0
    COUNT= 0
}

{
    if ($4<0+MIN) MIN=$4
    if ($4>0+MAX) MAX=$4
    SUM+=$4
    COUNT+=1
}

END {
    print MIN, MAX, SUM/COUNT
}
