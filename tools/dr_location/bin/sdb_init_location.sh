#!/bin/bash
# ========================================
# sdb_init_location.sh - 初始化和展示 Location 信息
# ========================================

# 脚本所在目录
SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
PROJECT_ROOT="$(dirname "$SCRIPT_DIR")"

# 显示帮助
function show_help() {
    cat << EOF
Usage:
    sdb_init_location.sh [command] [options]

Commands:
    show          Show current cluster location information
    check         Check location configuration against expected
    init          Initialize location configuration

Options:
    -h, --help          Show help message
    -c, --conf <file>   Specify config file, default config/config.js
    -f, --file <file>   Specify location file in 'check' or 'init' mode
    -q, --quiet         Do not output the result file from 'show' mode

Examples:
    sdb_init_location.sh
    sdb_init_location.sh show
    sdb_init_location.sh check
    sdb_init_location.sh init

Others:
    -f, --file <file> content is as follows:
        [GuangZhou(active)]
        host1
        host2

        [ShenZhen]
        host3

        [domains(1)]
        domain1
        domain2

    domains section:
        [domains(1)]  select mode, only listed domains
        [domains(0)]  exclude mode, all domains except listed
EOF
}

# 主函数
function main() {
    local command=""
    local configJs=""
    local file=""
    local params=""
    local quiet="false"

    # 解析命令行参数
    TEMP=$(getopt -o hc:f:q \
        --long help,conf:,file:,quiet \
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
            -f|--file)
                file="$2"
                shift 2
                ;;
            -q|--quiet)
                quiet=true
                shift
                ;;
            --)
                shift
                break
                ;;
            *)
                echo "Unknown option: $1"
                show_help
                return 1
                ;;
        esac
    done

    # 解析子命令（剩余位置参数）
    while [[ $# -gt 0 ]]; do
        case "$1" in
            show)
                command="show"
                shift
                ;;
            check)
                command="check"
                shift
                ;;
            init)
                command="init"
                shift
                ;;
            *)
                echo "Unknown command: $1"
                show_help
                return 1
                ;;
        esac
    done

    test ! -f "$PROJECT_ROOT/lib/common.sh" && echo "[ERROR] Failed to load $PROJECT_ROOT/lib/common.sh" && return 1
    source "$PROJECT_ROOT/lib/common.sh"

    # 构建参数列表，用 ; 分隔
    local cmd="var scriptDir=\"$SCRIPT_DIR\""
    if [ -n "$command" ]; then
        cmd="$cmd; var mode=\"$command\""
    else
        cmd="$cmd; var mode=\"show\""
    fi
    
    cmd="$cmd; var quiet=$quiet"
    if [ -n "$configJs" ]; then
        # 拼入绝对路径
        if [[ "$configJs" == /* ]]; then
            cmd="$cmd; var configJs=\"$configJs\""
        else
            cmd="$cmd; var configJs=\"$SCRIPT_DIR/$configJs\""
        fi
    fi
    if [ -n "$file" ]; then
        if [ "$command" == "show" ]; then
            echo "[ERROR] -f, --file can only use in check or init mode"
            return 1
        fi
        # 拼入绝对路径
        if [[ "$file" == /* ]]; then
            cmd="$cmd; var locationFile=\"$file\""
        else
            cmd="$cmd; var locationFile=\"$SCRIPT_DIR/$file\""
        fi
    fi

    $sdb_shell -f "$PROJECT_ROOT/bin/main.js" -e "$cmd"
    return $?
}

main "$@"
exit $?
