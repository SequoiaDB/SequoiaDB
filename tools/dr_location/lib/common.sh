#!/bin/bash

# 检查配置文件
if [[ "" == "$configJs" && ! -f "$PROJECT_ROOT/config/config.js" ]]; then
    echo "[ERROR] Config file not found: $PROJECT_ROOT/config/config.js, please run \"cp $PROJECT_ROOT/config/config.js.sample $PROJECT_ROOT/config/config.js\""
    exit 1
fi

# 不能同时指定 -l,--location 和 -H,--hostname
if [[ "" != "$locations" && "" != "$hostnames" ]]; then
    echo "[ERROR] Parameter \"-l,--location\" and \"-H,--hostname\" cannot be used at the same time"
    exit 1
fi

# 指定 domain 时必须指定 -l,--location 或 -H,--hostname
if [[ -n "$domains" && ! -n "$file" ]]; then
    if [[ ! -n "$locations" && ! -n "$hostnames" ]]; then
        echo '[ERROR] Cannot specify only "-d,--domain" without specifying "-H,--hostname" or "-l,--location"'
        exit 1
    fi
fi

# 获取 sdb shell
if [ -f /etc/default/sequoiadb ]; then
    # 从 default 文件获取
    . /etc/default/sequoiadb
    sdb_shell="$INSTALL_DIR/bin/sdb"
elif [ -f "$PROJECT_ROOT/../../bin/sdb" ]; then
    # 从上层目录获取
    sdb_shell="$PROJECT_ROOT/../../bin/sdb"
else
    echo "[ERROR] Cannot find \"sdb\" shell from file /etc/default/sequoiadb or the parent directory"
    exit 1
fi

if [ ! -f "$sdb_shell" ]; then
    echo "[ERROR] \"$sdb_shell\" does not exist"
    exit 1
fi

SCRIPT_DIR=$(cd "$(dirname $0)" && pwd)/../
test $? -ne 0 && echo "[ERROR] failed to get script dir from $0" && exit 1