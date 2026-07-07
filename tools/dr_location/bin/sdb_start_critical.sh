#!/bin/bash
# ========================================
# sdb_start_critical.sh - 开启 CriticalMode
# ========================================

# 脚本所在目录
SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
PROJECT_ROOT="$(dirname "$SCRIPT_DIR")"

# 显示帮助
function show_help() {
    cat << EOF
Usage:
    sdb_start_critical.sh [command] [options]

Commands:
    start_critical    Start CriticalMode

Options:
    -h, --help                 Show help message
    -c, --conf <file>          Specify config file, default config/config.js
    -l, --location <location>  Location(s) with the specified CriticalMode enabled, cannot be used with -H,--hostname
    -H, --hostname <host>      Hostname(s) with the specified CriticalMode enabled, cannot be used with -l,--location
    -d, --domain <domain>      Filter the data groups within the specify domain(s)
    -f, --file <file>          Specify node information file, ignore other command-line node args

Examples:
    sdb_start_critical.sh -l GuangZhou
    sdb_start_critical.sh -H host1,host2
    sdb_start_critical.sh -l GuangZhou -d domain1
    sdb_start_critical.sh -f target

Notes:
    CriticalMode will set these nodes to be primary during re-election
    This may cause data rollback if not used properly
    Use with caution and follow proper DR procedures

Others:
    -f, --file <file> content is as follows:
        [location]
        location1
        location2

        [hostname]
        host1
        host2

        [domain]
        domain1
        domain2
EOF
}

# 主函数
function main() {
    local configJs=""
    local locations=""
    local hostnames=""
    local domains=""
    local file=""

    # 解析命令行参数
    TEMP=$(getopt -o hc:l:H:d:f: \
        --long help,conf:,location:,hostname:,domain:,file: \
        -n "$0" -- "$@")
    if [ $? != 0 ]; then
        show_help
        exit 1
    fi
    eval set -- "$TEMP"
    while true; do
        case "$1" in
            -h|--help)
                show_help
                exit 0
                ;;
            -c|--conf)
                configJs="$2"
                shift 2
                ;;
            -l|--location)
                locations="$2"
                shift 2
                ;;
            -H|--hostname)
                hostnames="$2"
                shift 2
                ;;
            -d|--domain)
                domains="$2"
                shift 2
                ;;
            -f|--file)
                file="$2"
                shift 2
                ;;
            --)
                shift
                break
                ;;
            *)
                echo "Unknown option: $1"
                show_help
                exit 1
                ;;
        esac
    done

    if [[ ! -n "$locations" && ! -n "$hostnames" && ! -n "$file" ]]; then
        echo "[ERROR] No target to start CriticalMode"
        exit 1
    fi

    test ! -f "$PROJECT_ROOT/lib/common.sh" && echo "[ERROR] Failed to load $PROJECT_ROOT/lib/common.sh" && return 1
    source "$PROJECT_ROOT/lib/common.sh"

    # 构建参数列表，用 ; 分隔
    local cmd="var scriptDir=\"$SCRIPT_DIR\"; var mode=\"start_critical\""

    if [ -n "$configJs" ]; then
        # 拼入绝对路径
        if [[ "$configJs" == /* ]]; then
            cmd="$cmd; var configJs=\"$configJs\""
        else
            cmd="$cmd; var configJs=\"$SCRIPT_DIR/$configJs\""
        fi
    fi

    if [ -n "$file" ]; then
        # 忽略其他命令行配置
        # 拼入绝对路径
        if [[ "$file" == /* ]]; then
            cmd="$cmd; var targetFile=\"$file\""
        else
            cmd="$cmd; var targetFile=\"$SCRIPT_DIR/$file\""
        fi
    else
        if [ -n "$locations" ]; then
            locationArray=(`echo "$locations" | sed 's/,/ /g'`)
            locationStr="\"${locationArray[0]}\""
            for((i=1; i<${#locationArray}; i++))
            do
                test "${locationArray[i]}" != "" && locationStr="$locationStr,\"${locationArray[i]}\""
            done
            cmd="$cmd; var locations=[$locationStr]"
        fi
        if [ -n "$hostnames" ]; then
            hostnameArray=(`echo "$hostnames" | sed 's/,/ /g'`)
            hostnameStr="\"${hostnameArray[0]}\""
            for((i=1; i<${#hostnameArray}; i++))
            do
                test "${hostnameArray[i]}" != "" && hostnameStr="$hostnameStr,\"${hostnameArray[i]}\""
            done
            cmd="$cmd; var hostnames=[$hostnameStr]"
        fi
        if [ -n "$domains" ]; then
            domainArray=(`echo "$domains" | sed 's/,/ /g'`)
            domainStr="\"${domainArray[0]}\""
            for((i=1; i<${#domainArray}; i++))
            do
                test "${domainArray[i]}" != "" && domainStr="$domainStr,\"${domainArray[i]}\""
            done
            cmd="$cmd; var domains=[$domainStr]"
        fi
    fi

    $sdb_shell -f "$PROJECT_ROOT/bin/main.js" -e "$cmd"
    return $?
}

main "$@"
exit $?
