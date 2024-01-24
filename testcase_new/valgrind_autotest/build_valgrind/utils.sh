#!/bin/bash

# new output with \n, while echo need echo "$output"
function get_new_output()
{
    srcstr=$1
    if [ -z "${srcstr}" ];then
        return
    fi
    # echo up and below lines
    newstr_len=$[${#srcstr}+2]
    columns=$(stty size | awk '{print $2}')
    if [ ${newstr_len} -gt ${columns} ];then
        newstr_len=${columns}
    fi
    output_line="+"
    for (( i=1; i<$[${newstr_len}-1]; i++ ))
    do
        output_line=${output_line}"-"
    done
    output_line=${output_line}"+"
    echo "${output_line}"

    # echo content with "|"
    new_columns=$[${columns}-2]
    t1=$[${#srcstr}/${new_columns}]
    t2=$[${#srcstr}%${new_columns}]
    for (( i=0; i<${t1}; i++ ))
    do
        split_start=$[i*${new_columns}]
        offset=${new_columns}
        output="|${srcstr:${split_start}:${offset}}|"
        echo "${output}"
    done
    split_start=$[${new_columns}*${t1}]
    offset=$[${#srcstr}-${split_start}]
    remainstr=${srcstr:${split_start}:${offset}}
    if [ $[${#output_line}-2] -gt ${#remainstr} ];then
        t3=$[${#output_line}-2-${#remainstr}]
        for (( i=0; i<${t3}; i++ ))
        do
            remainstr=${remainstr}" "
        done
    fi
    echo "|${remainstr}|"
    echo "${output_line}"
}

