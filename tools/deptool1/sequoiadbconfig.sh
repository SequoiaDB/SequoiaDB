#!/bin/bash

#是否输出调试信息[1:输出调试信息,2:输出普通信息,3:不输出]
IS_PRINGT_DEBUG=1

#安装文件的路径
INSTALL_PATH="/home/sequoiadb/www/http/shell"

#安装文件的文件名
INSTALL_NAME="sequoiadb-1.5-linux_x86_64-installer.run"

#sdbcm端口
SDBCM_PORT="50010"

#分区组列表
LIST_GROUP=("g1")

#主机列表
LIST_HOST=(\
"ARRAY_HOST_1" \
"ARRAY_HOST_2" \
"ARRAY_HOST_3" \
)

ARRAY_HOST_1=("ubuntu-test-01" "/opt/sequoiadb" "sdbadmin_group" "sdbadmin" "sequoiadb")
ARRAY_HOST_2=("ubuntu-test-02" "/opt/sequoiadb" "sdbadmin_group" "sdbadmin" "sequoiadb")
ARRAY_HOST_3=("ubuntu-test-03" "/opt/sequoiadb" "sdbadmin_group" "sdbadmin" "sequoiadb")

#节点列表
LIST_NODE=(\
"ARRAY_NODE_1" \
"ARRAY_NODE_2" \
"ARRAY_NODE_3" \
"ARRAY_NODE_4" \
"ARRAY_NODE_5" \
"ARRAY_NODE_6" \
"ARRAY_NODE_7" \
"ARRAY_NODE_8" \
"ARRAY_NODE_9" \
)

ARRAY_NODE_1=("coord" "" "ARRAY_HOST_1" "SDBCONF_1")
ARRAY_NODE_2=("coord" "" "ARRAY_HOST_2" "SDBCONF_2")
ARRAY_NODE_3=("coord" "" "ARRAY_HOST_3" "SDBCONF_3")
ARRAY_NODE_4=("cata" "" "ARRAY_HOST_1" "SDBCONF_4")
ARRAY_NODE_5=("cata" "" "ARRAY_HOST_2" "SDBCONF_5")
ARRAY_NODE_6=("cata" "" "ARRAY_HOST_3" "SDBCONF_6")
ARRAY_NODE_7=("data" "g1" "ARRAY_HOST_1" "SDBCONF_7")
ARRAY_NODE_8=("data" "g1" "ARRAY_HOST_2" "SDBCONF_8")
ARRAY_NODE_9=("data" "g1" "ARRAY_HOST_3" "SDBCONF_9")

#配置文件参数表
SDB_CONFIG=(confpath logpath diagpath dbpath indexpath bkuppath maxpool svcname replname shardname catalogname httpname diaglevel role catalogaddr logfilesz logfilenum transactionon numpreload maxprefpool maxsubquery logbuffsize)
SDBCONF_1=("" "" "" "/opt/sequoiadb/database/coord" "" "" "0" "50000" "" "" "" "" "3" "" "" "64" "20" "false" "0" "200" "10" "1024")
SDBCONF_2=("" "" "" "/opt/sequoiadb/database/coord" "" "" "0" "50000" "" "" "" "" "3" "" "" "64" "20" "false" "0" "200" "10" "1024")
SDBCONF_3=("" "" "" "/opt/sequoiadb/database/coord" "" "" "0" "50000" "" "" "" "" "3" "" "" "64" "20" "false" "0" "200" "10" "1024")
SDBCONF_4=("" "" "" "/opt/sequoiadb/database/cata" "" "" "0" "40000" "" "" "" "" "3" "" "" "64" "20" "false" "0" "200" "10" "1024")
SDBCONF_5=("" "" "" "/opt/sequoiadb/database/cata" "" "" "0" "40000" "" "" "" "" "3" "" "" "64" "20" "false" "0" "200" "10" "1024")
SDBCONF_6=("" "" "" "/opt/sequoiadb/database/cata" "" "" "0" "40000" "" "" "" "" "3" "" "" "64" "20" "false" "0" "200" "10" "1024")
SDBCONF_7=("" "" "" "/opt/sequoiadb/database/data" "" "" "0" "30000" "" "" "" "" "3" "" "" "64" "20" "false" "0" "200" "10" "1024")
SDBCONF_8=("" "" "" "/opt/sequoiadb/database/data" "" "" "0" "30000" "" "" "" "" "3" "" "" "64" "20" "false" "0" "200" "10" "1024")
SDBCONF_9=("" "" "" "/opt/sequoiadb/database/data" "" "" "0" "30000" "" "" "" "" "3" "" "" "64" "20" "false" "0" "200" "10" "1024")
