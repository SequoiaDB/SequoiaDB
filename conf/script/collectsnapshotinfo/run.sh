#!/bin/bash

script_path=$(cd `dirname $0`; pwd)
comm_script_path=${script_path}/..
js_file="${comm_script_path}/define.js"
js_file="${js_file};${comm_script_path}/common.js"
js_file="${js_file};${comm_script_path}/log.js"
js_file="${js_file};${comm_script_path}/func.js"
js_file="${js_file};${script_path}/collectSnapshotInfo.js"

#user define
coordList="[{hostName:'localhost', svcName:11810}]"
userName=""
password=""

${script_path}/../../../bin/sdb -f ${js_file} -e "var COORD_LIST=${coordList};var USER_NAME='${userName}';var PASSWORD='${password}';"
