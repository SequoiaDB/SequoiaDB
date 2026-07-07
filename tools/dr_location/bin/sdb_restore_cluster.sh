#!/bin/bash
# ========================================
# sdb_restore_cluster.sh - 恢复集群到正常状态
# ========================================

# 脚本所在目录
SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
PROJECT_ROOT="$(dirname "$SCRIPT_DIR")"

# 显示帮助
function show_help() {
    cat << EOF
Usage:
    sdb_restore_cluster.sh [options]

Options:
    -h, --help          Show help message
    -c, --conf <file>   Specify config file, default config/config.js

Example:
    sdb_restore_cluster.sh -c config/config.js

Notes:
    This script will:
    1. Check if all nodes are healthy
    2. Disable all MaintenanceMode
    3. Disable all CriticalMode
    4. Restore the cluster to normal operation
    Use this only after a DR failover or when cluster is stable
EOF
}

# 主函数
function main() {
    local configJs=""

    # 解析命令行参数
    TEMP=$(getopt -o hc: \
        --long help,conf: \
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

    test ! -f "$PROJECT_ROOT/lib/common.sh" && echo "[ERROR] Failed to load $PROJECT_ROOT/lib/common.sh" && return 1
    source "$PROJECT_ROOT/lib/common.sh"

    # 构建参数列表，用 ; 分隔
    local cmd="var scriptDir=\"$SCRIPT_DIR\"; var mode=\"restore\""
    if [ -n "$configJs" ]; then
        # 拼入绝对路径
        if [[ "$configJs" == /* ]]; then
            cmd="$cmd; var configJs=\"$configJs\""
        else
            cmd="$cmd; var configJs=\"$SCRIPT_DIR/$configJs\""
        fi
    fi

    $sdb_shell -f "$PROJECT_ROOT/bin/main.js" -e "$cmd"
    return $?
}

main "$@"
exit $?
