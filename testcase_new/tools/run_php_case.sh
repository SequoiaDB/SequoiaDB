#!/usr/bin/env bash
set -u

runner_dir=$(cd -- "$(dirname -- "${BASH_SOURCE[0]}")" && pwd -P) || exit 2

show_help() {
    cat <<'HELP'
Usage:
  bash testcase_new/tools/run_php_case.sh [-p path]|[-f file] [-t type] [-m mode] [options]

 -p path                  : 运行指定目录下的用例，相对用例根目录，如 crud；与 -f 互斥
 -f file                  : 运行指定的用例文件，相对用例根目录，可重复，如 crud/xxx_Test.php；与 -p 互斥
 -t type                  : 运行指定类型的用例，可取 driver，默认 driver
 -m mode                  : 运行指定模式的用例，默认 normal
                             - driver 可取 normal|sync|largedata|configure|all
                             - normal：基本功能用例；sync：集群拓扑用例，会增删节点
                             - largedata：大数据量用例；configure：配置类用例，会修改集群配置
                             - all：该类型下的全部用例
 -s stopFlag              : 发生用例错误是否停止，0 表示继续，1 表示停止，默认为 0
 -h hostname              : 指定 COORD 节点 HostName 或 IP，默认 localhost
 -n svcname               : 指定 COORD 节点服务名，默认 11810
 -print [failed]          : 屏幕打印方式，不指定时每个用例只打印一行结果
                             - failed：额外打印失败用例的错误信息和调用栈
                             - 不带参数：打印 PHPUnit 原始输出
 --dry-run                : 只打印将执行的用例，不执行
 --report-dir dir         : 指定报告目录，默认 local_test_report/php/<type>/<mode>/<php版本>/

 --conf file              : 环境配置文件，默认 testcase_new/tools/conf/test.conf
 --install-dir dir        : 指定 SequoiaDB 安装目录，默认 /opt/sequoiadb
 --db-path dir            : 指定用例新建节点的数据目录，默认 <install-dir>/database/
 --case-dir dir           : 指定用例根目录，默认 testcase_new/driver/php
 --changed-prefix name    : 指定用例集合空间名前缀，默认 sdv_php_test
 --reserve-port-begin n   : 指定用例新建节点的起始端口，默认 2600
 --reserve-port-end n     : 指定用例新建节点的结束端口，默认 2700
 --php file               : 指定 PHP 可执行文件，默认 <install-dir>/tools/server/php/bin/php-bin
 --php-ini file           : 指定 php.ini，默认不指定
 --extension file         : 指定 SequoiaDB PHP 扩展，默认 <install-dir>/lib/phplib/libsdbphp-5.4.6.so
 --phpunit file           : 指定 PHPUnit，默认 <case-dir>/tools/phpunit.phar
 --workdir dir            : 指定临时文件目录，默认 <report-dir>/.work
 --reportFormat jenkins   : 将报告改写为 Jenkins 格式，CI 使用
 --reportinfo kv          : CI 报告信息，格式 k=v;k=v，可取 execute_host|cluster_hosts|diaglog_url|
                            diaglog_base|test_type|deploy_mode|build_number
                             - 未指定 diaglog_url 时按 <diaglog_base>/<test_type>_<deploy_mode>_<build_number>.tar.gz 拼接
 --help                   : 显示帮助
HELP
}
runner_args=("$@")
php_exe=
php_ini=
extension=
conf_file="$runner_dir/conf/test.conf"
install_dir=
list_only=0

while (($#)); do
    case "$1" in
        --help)
            show_help
            exit 0
            ;;
        --dry-run)
            list_only=1
            shift
            ;;
        --php|--php-ini|--extension|--conf|--install-dir|--deploy-mode)
            option=$1
            if (($# < 2)) || [[ -z $2 ]]; then
                printf 'PHP runner：%s 缺少参数值\n' "$option" >&2
                exit 2
            fi
            case "$option" in
                --php) php_exe=$2 ;;
                --php-ini) php_ini=$2 ;;
                --extension) extension=$2 ;;
                --conf) conf_file=$2 ;;
                --install-dir) install_dir=$2 ;;
                --deploy-mode) ;;
            esac
            shift 2
            ;;
        *)
            shift
            ;;
    esac
done

# 安装目录：--install-dir > 环境变量 INSTALL_DIR > 配置文件 INSTALL_DIR > /opt/sequoiadb。
if [[ -z $install_dir ]]; then
    install_dir=${INSTALL_DIR:-}
    if [[ -z $install_dir && -f $conf_file ]]; then
        install_dir=$(sed -n 's/^[[:space:]]*INSTALL_DIR[[:space:]]*=[[:space:]]*//p' "$conf_file" | tail -n 1)
        install_dir=${install_dir%$'\r'}
    fi
fi
install_dir=${install_dir:-/opt/sequoiadb}
runner_args+=(--install-dir "$install_dir")

if [[ -z $php_exe ]]; then
    for candidate in "$install_dir/tools/server/php/bin/php-bin" "$install_dir/tools/server/php/bin/php"; do
        if [[ -x $candidate ]]; then
            php_exe=$candidate
            break
        fi
    done
    if [[ -n $php_exe && -z $php_ini && -z $extension &&
          -f $install_dir/lib/phplib/libsdbphp-5.4.6.so ]]; then
        extension=$install_dir/lib/phplib/libsdbphp-5.4.6.so
        runner_args+=(--extension "$extension")
    fi
    php_exe=${php_exe:-php}
    resolved_php=$(command -v -- "$php_exe") || {
        printf 'PHP runner：找不到 PHP 可执行文件：%s\n' "$php_exe" >&2
        exit 2
    }
    runner_args+=(--php "$resolved_php")
else
    resolved_php=$(command -v -- "$php_exe") || {
        printf 'PHP runner：找不到 PHP 可执行文件：%s\n' "$php_exe" >&2
        exit 2
    }
fi

php_command=("$resolved_php")
if ((list_only)); then
    php_command+=(-n)
else
    if [[ -n $php_ini ]]; then
        php_command+=(-c "$php_ini")
    elif [[ -n $extension ]]; then
        php_command+=(-n -d date.timezone=UTC)
    fi
    [[ -z $extension ]] || php_command+=(-d "extension=$extension")
fi

if [[ -z ${PHP_RUNNER_COLOR+x} ]]; then
    if [[ -t 1 && ${TERM:-dumb} != dumb && -z ${NO_COLOR:-} ]]; then
        export PHP_RUNNER_COLOR=1
    else
        export PHP_RUNNER_COLOR=0
    fi
fi

child_pid=
forward_signal() {
    local signal=$1 status=$2
    trap - INT TERM
    if [[ -n $child_pid ]] && kill -0 "$child_pid" 2>/dev/null; then
        kill -"$signal" -- "-$child_pid" 2>/dev/null || kill -"$signal" "$child_pid" 2>/dev/null || true
        wait "$child_pid" 2>/dev/null || true
    fi
    exit "$status"
}

trap 'forward_signal INT 130' INT
trap 'forward_signal TERM 143' TERM
set -m
"${php_command[@]}" "$runner_dir/lib/php_runner.php" "${runner_args[@]}" &
child_pid=$!
wait "$child_pid"
exit $?
