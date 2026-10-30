#!/usr/bin/env bash
# C++ 入口。公共逻辑在 lib/c_cpp_runner.sh，这里只保留差异。
SCRIPT_DIR=$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd -P)
TESTCASE_ROOT=$(cd "$SCRIPT_DIR/.." && pwd -P)
PROJECT_ROOT=$(cd "$SCRIPT_DIR/../.." && pwd -P)
SRC_DIR="$TESTCASE_ROOT/driver/cpp"

RUNNER_TAG=runCPP
RUNNER_KIND=C++
CASE_TYPE=cpp
CLIENT_LIB=libsdbcpp.so
svcname=50000
stopFlag=0
dshostname=localhost
dssvcname=50000
sslFile=("sslTrue9648" "sslTrue9648.static")
transactionOn=0
runLastFailed=0
dshostnameSet=0
dssvcnameSet=0
failedTestcaseFile="$SRC_DIR/failedTestcase.txt"
transFile=("getListTransaction12526" "getListTransaction12526.static"
           "getSnapshotTransaction12662" "getSnapshotTransaction12662.static"
           "trans12520_12675" "trans12520_12675.static")

if ! source "$SCRIPT_DIR/lib/c_cpp_runner.sh"; then
   echo "[runCPP][ERROR] cannot load $SCRIPT_DIR/lib/c_cpp_runner.sh" >&2
   exit 2
fi

function display()
{
   cat <<'HELP'
Usage:
  bash testcase_new/tools/runC.sh [-p path]|[-f file] [-m mode] [options]
  bash testcase_new/tools/runCPP.sh [-p path]|[-f file] [-m mode] [options]

 -p path                  : 运行指定目录下的用例，相对用例根目录，如 crud；与 -f 互斥
 -f file                  : 运行指定的用例源文件，相对用例根目录，可重复，如 crud/upsert25409.cpp；与 -p 互斥
 -m mode                  : 运行指定模式的用例，默认 normal
                             - 可取 normal|smoke|sync|configure|all
                             - normal：基本功能用例；smoke：冒烟名单
                             - sync：集群拓扑用例，会增删节点
                             - configure：配置类用例，会修改集群配置（含 SSL）
                             - all：全部用例
 -s stopFlag              : 发生用例错误是否停止，0 表示继续，1 表示停止，默认为 0
 -h hostname              : 指定 COORD 节点 HostName 或 IP，默认 localhost
 -n svcname               : 指定 COORD 节点服务名，C 默认 11810，C++ 默认 50000
 -print [failed]          : 屏幕打印方式，不指定时每个用例只打印一行结果
                             - failed：额外打印失败用例的标准输出、标准错误和失败信息
                             - 不带参数：打印测试程序原始输出
 --dry-run                : 只打印将执行的编译和运行命令，不更新、不编译、不执行
 --report-dir dir         : 指定报告目录，默认 local_test_report/<c|cpp>/driver/<mode>/

 --conf file              : 环境配置文件；不传则用内置默认，不自动读 test.conf
 --install-dir dir        : 指定 SequoiaDB 安装目录，默认 /opt/sequoiadb
 --db-path dir            : 指定用例新建节点的数据目录，默认 <install-dir>/database/
 --case-dir dir           : 指定已编译用例目录，默认 <用例根>/build_test
 --test                   : 执行测试程序；不加则只更新、编译
 --no-up                  : 跳过源码更新，默认执行更新
 --no-compile             : 跳过编译，直接使用 build_test
 --release                : 使用 release 编译，默认 debug
 --use-ssl                : 允许执行 SSL 用例；仅 -m normal 时有意义，默认关闭
 --enable-transaction     : 允许执行事务专用用例；仅 runCPP、-m normal 时有意义，默认关闭
 --ds-hostname host       : 指定数据源主机，默认 localhost（仅 C++）
 --ds-svcname port        : 指定数据源服务端口，默认 50000（仅 C++）
 --run-last-failed        : 重跑上次失败程序，不更新、不编译（仅 C++）
 --reportFormat jenkins   : 将报告改写为 Jenkins 格式，CI 使用
 --reportinfo kv          : CI 报告信息，格式 k=v;k=v，可取 execute_host|cluster_hosts|diaglog_url|
                            diaglog_base|test_type|deploy_mode|build_number
                             - 未指定 diaglog_url 时按 <diaglog_base>/<test_type>_<deploy_mode>_<build_number>.tar.gz 拼接
 --help                   : 显示帮助
HELP
}

function hookParseOption()
{
   hookConsume=-1
   case $1 in
      --ds-hostname | -dshostname )
         [ -n "${2:-}" ] || reportError "missing value for $1"
         dshostname=$2
         dshostnameSet=1
         hookConsume=1
         ;;
      --ds-svcname | -dssvcname )
         [ -n "${2:-}" ] || reportError "missing value for $1"
         dssvcname=$2
         dssvcnameSet=1
         hookConsume=1
         ;;
      --enable-transaction | -transactionOn )
         transactionOn=1
         hookConsume=0
         ;;
      --run-last-failed | -runLastFailed )
         runLastFailed=1
         runTests=1
         needUpdate=0
         needCompile=0
         hookConsume=0
         ;;
   esac
}

function hookSkipCase()
{
   if [ $transactionOn -eq 0 ] && contains "$1" "${transFile[@]}"; then
      return 0
   fi
   return 1
}

function hookCaseArgs()
{
   if [ $ciMode -eq 0 ] || [ $dshostnameSet -eq 1 ]; then
      command+=(-o "$dshostname")
   fi
   if [ $ciMode -eq 0 ] || [ $dssvcnameSet -eq 1 ]; then
      command+=(-p "$dssvcname")
   fi
}

function noteFailedCase()
{
   printf '%s|failure|%s|%s|%s|%s\n' \
      "$testcase" "$elapsed" "$xmlFile" "$stdoutFile" "$stderrFile" \
      >>"$failedTestcaseFile"
}

function loadFailedTestcases()
{
   local testcase

   if [ ! -s "$failedTestcaseFile" ]; then
      reportError "no failed test cases: $failedTestcaseFile"
   fi
   while IFS='|' read -r testcase _
   do
      if [ -n "$testcase" ]; then
         addSelected "exe:$testcase"
      fi
   done <"$failedTestcaseFile"
}

function hookCaseFailed()
{
   noteFailedCase
}

function hookBeforeRun()
{
   : >"$failedTestcaseFile"
}

function hookSelect()
{
   if [ $runLastFailed -eq 1 ]; then
      loadFailedTestcases
   fi
}

function hookValidate()
{
   if [ $runLastFailed -eq 1 ] && [ ${#selectArgs[@]} -gt 0 ]; then
      reportError "--run-last-failed cannot be combined with -f/-p"
   fi
}

function hookReproArgs()
{
   printf ' --ds-hostname %s --ds-svcname %s' "$dshostname" "$dssvcname"
}

runnerMain "$@"
