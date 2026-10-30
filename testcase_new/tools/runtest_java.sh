#!/usr/bin/env bash
# 本地与 CI 共用的 Java TestNG 执行器。
#
# 输入：用例 Maven 工程、TestNG suite（或单个类）、数据库连接与可选运行环境参数。
# 输出：JUnit 在 <case-dir>/output/junitreports；本地可读报告在
# 仓库根 local_test_report/java/<类型>/<模式>/。
#
# 退出码：
#   0    全部通过；或用例全部 skipped（环境不满足）。后者只打印 WARN，
#        由调用方按自己的策略判定——旧 CI 的 count_skiped_case.xml 会区分
#        DEPLOY_MODE：非 STANDALONE 转 unstable，STANDALONE 视为通过。
#   1    JUnit 报告中存在 failure 或 error。
#   2    参数、环境或前置依赖错误（含 CONFIGURETEST 前后置失败）。
#   3    报告缺失或一个 testcase 都没有，即用例未真正执行。
#   130  收到 SIGINT；143 收到 SIGTERM（128+信号号的通用约定）。
#   Maven 非零且报告中没有 failure/error 时返回 2。
#
# 调用方约定：0 与 1 之外的值都表示「未得到可信的测试结论」，应判为执行器
# 或环境失败，而不是产品缺陷。

set -euo pipefail

usage() {
    cat <<'HELP'
Usage:
  bash testcase_new/tools/runtest_java.sh [-p path] [-f file] [-t type] [-m mode] [options]

 -p path                  : 运行指定包路径，相对 src/test/java，如 com.sequoiadb.crud；与 -f 互斥
 -f file                  : 运行指定测试类源文件，相对 src/test/java，如 com/sequoiadb/crud/CrudTest.java；与 -p 互斥
 -t type                  : 选择 Maven 工程，可取 story|story_at|sdv|driver|reliability|s3|flink
                             - 未指定 -t/-p/-f 时默认 story/normal
                             - 未指定 -t 但指定 -p/-f 时默认 story 工程
                             - 与 --case-dir 互斥
 -m mode                  : 运行模式，可取 normal|smoke|sync|largedata|configure|mvcc|reliability，默认 normal
                             - reliability 工程且不传 -m 时，按安装包 Debug/Release 选择 TestNG XML
                             - CI 可传 NORMALTEST 等别名，脚本会映射为上述 mode
 -s stopFlag              : 发生用例错误是否停止，0 表示继续，1 表示停止，默认为 0
 -h hostname              : 指定 COORD 节点 HostName 或 IP，默认 localhost
 -n svcname               : 指定 COORD 节点服务名，默认 11810
 -print [failed|maven]    : 屏幕打印方式，不指定时只打印摘要与逐方法一行结果
                             - failed：额外打印失败用例详情（单独 -print 等价于 failed）
                             - maven：同步输出 Maven/TestNG 原始日志
 --dry-run                : 只检查参数并打印将执行的命令，不编译、不执行
 --conf file              : 环境配置文件；存在 testcase_new/tools/conf/test.conf 时默认加载
 --user user              : 远程 SSH 用户名；未指定时用当前 Linux 用户
 --password passwd        : --user 对应 SSH 密码
 --rootpwd passwd         : 需要 root 的用例使用的 root 密码
 --install-dir dir        : SequoiaDB 安装目录，默认 /opt/sequoiadb
 --java-home dir          : JAVA_HOME，默认自动检测
 --timeout seconds        : 单 TestNG 方法超时（秒），0 表示不限制
 --log-file file          : 完整 Maven 日志路径，默认 <caseDir>/output/runtest_java.log
 --suite file             : 指定 TestNG XML；未指定时由 -t/-m 选择
 --case-dir dir           : Maven 工程根（含 pom.xml），与 -t 互斥
 --mongo-driver-jar jar   : Mongo 驱动 JAR；传入即 Mongo 工程，suite 固定 testng.xml
 --reportFormat jenkins   : 调用 report-formatters/jenkins.sh 改写 JUnit
 --reportinfo kv          : CI 报告装饰，格式 k=v;k=v，可取 execute_host|cluster_hosts|diaglog_url|
                            diaglog_base|test_type|deploy_mode|build_number
                             - 未指定 diaglog_url 时按 <diaglog_base>/<test_type>_<deploy_mode>_<build_number>.tar.gz 拼接
 --help                   : 显示帮助（单独 -h 且无 hostname 参数时同 --help）
HELP
}

# 下列参数仍可用，但不列入 --help（CI 调用时使用）：
#   --testcase-type name   JUnit classname 前缀，如 story_java；CI 走 --case-dir 时必须显式传
#   --report-dir dir       将 <caseDir>/output/junitreports 额外复制到 CI 收集目录；不是 local_test_report 路径


die() {
    echo "[runtest_java][ERROR] $*" >&2
    exit 2
}

abs_path() {
    realpath -m -- "$1"
}

trim_conf_value() {
    local value=$1
    value="${value#"${value%%[![:space:]]*}"}"
    value="${value%"${value##*[![:space:]]}"}"
    printf '%s' "$value"
}

# 从前往后取第一个非空值。空字符串表示这一层没设。
first_set() {
    local dest=$1 val
    shift
    for val in "$@"; do
        [[ -n "$val" ]] || continue
        printf -v "$dest" '%s' "$val"
        return
    done
    printf -v "$dest" '%s' ""
}

load_conf() {
    local file=$1 line key value trimmed first=1
    [[ -f "$file" ]] || die "configuration file not found: $file"
    [[ -r "$file" ]] || die "configuration file is not readable: $file"

    while IFS= read -r line || [[ -n "$line" ]]; do
        if [[ -n "$first" ]]; then
            line=${line#$'\xEF\xBB\xBF'}
            first=
        fi
        line=${line%$'\r'}
        trimmed=$(trim_conf_value "$line")
        [[ -n "$trimmed" && "$trimmed" != \#* && "$line" == *=* ]] || continue
        key=$(trim_conf_value "${line%%=*}")
        value=$(trim_conf_value "${line#*=}")
        case "$key" in
            DB_HOSTNAME|SVCNAME|INSTALL_DIR|JAVA_HOME|ES_IP|ES_PORT|ES_PORT_RELIABILITY|\
            S3HOSTNAME|S3SVCNAME|S3CLIENTDIR|ROOT_USER|ROOT_PWD|SDBDRIVERDIR|\
            THREADEXECUTOR_DIR|RSRVNODEDIR|SCRIPTDIR|FULLTEXTPREFIX|\
            COORDS|USERNAME|PASSWORD|MYSQLURL|MYSQLUSERNAME|MYSQLPASSWORD)
                printf -v "conf_$key" '%s' "$value"
                ;;
        esac
    done <"$file"
}

find_versioned_jar() {
    local dir=$1 prefix=$2 jar
    for jar in "$dir"/"$prefix"-*.jar; do
        [[ -f "$jar" ]] || continue
        case "$jar" in
            *-javadoc.jar|*-sources.jar|*-tests.jar) continue ;;
        esac
        printf '%s\n' "$jar"
        return 0
    done
    return 1
}

format_command() {
    local arg redacted
    for arg in "$@"; do
        case "$arg" in
            -DREMOTEPASSWD=*|-DROOTPASSWD=*|-DPASSWORD=*|\
            -DMYSQLPASSWORD=*)
                redacted="${arg%%=*}=<redacted>"
                printf ' %q' "$redacted"
                ;;
            *) printf ' %q' "$arg" ;;
        esac
    done
}

print_command() {
    printf '[runtest_java] command:'
    format_command "$@"
    printf '\n'
}

millis_to_seconds() {
    awk -v millis="$1" 'BEGIN { printf "%.3f", millis / 1000 }'
}

term_cols() {
    local n=${COLUMNS-}
    if [[ "$n" =~ ^[1-9][0-9]*$ ]]; then
        printf '%s' "$n"
        return
    fi
    n=$(tput cols 2>/dev/null) || n=
    if [[ "$n" =~ ^[1-9][0-9]*$ ]]; then
        printf '%s' "$n"
        return
    fi
    if [[ -r /dev/tty ]]; then
        n=$(stty size </dev/tty 2>/dev/null | awk '{print $2}') || n=
    fi
    [[ "$n" =~ ^[1-9][0-9]*$ ]] || n=80
    printf '%s' "$n"
}

format_case_status_line() {
    local name=$1 color=$2 status=$3 seconds=$4
    local suffix_plain cols pad
    suffix_plain=$(printf ' [%s] %8ss' "$status" "$seconds")
    cols=$(term_cols)
    pad=$((cols - ${#suffix_plain}))
    ((pad < 1)) && pad=1
    if ((${#name} >= pad)); then
        printf '%s [%s%s\033[0m] %8ss' "$name" "$color" "$status" "$seconds"
    else
        printf '%-*s [%s%s\033[0m] %8ss' "$pad" "$name" "$color" "$status" "$seconds"
    fi
}

write_case_detail_header() {
    local case_name=$1 case_status=$2 case_seconds=$3
    local class_name package_name package_path class_simple method_name safe_method
    local case_index detail_dir

    class_name=${case_name%%#*}
    method_name=${case_name#*#}
    package_name=${class_name%.*}
    package_path=${package_name//./\/}
    package_path=${package_path#com/sequoiadb/}
    class_simple=${class_name##*.}
    safe_method=$(printf '%s' "$method_name" | tr -c '[:alnum:]_$-' '_')
    case_index=$(cat "$case_sequence_file" 2>/dev/null || printf '0')
    case_index=$((case_index + 1))
    printf '%s\n' "$case_index" >"$case_sequence_file"
    detail_dir="$local_report_dir/$package_path/$class_simple"
    mkdir -p -- "$detail_dir"
    printf -v detail_file '%s/%s_%03d_out.txt' "$detail_dir" "$safe_method" "$case_index"
    {
        printf '# Maven suite command for this run; its temporary TestNG XML is deleted after the run.\n'
        printf 'CMD: %s\n' "$maven_command"
        printf '# Re-run command from the repository root; executes this entire test class.\n'
        printf 'CMD: %s\n' "$(format_command \
            "./testcase_new/tools/runtest_java.sh" \
            -t "$display_project" -m "$display_mode" -f "${class_name//./\/}.java" \
            -h "$db_hostname" -n "$svc_name" -print failed)"
        printf 'case   : %s\n' "$case_name"
        printf 'status : %s\n' "$case_status"
        printf 'time   : %ss\n' "$case_seconds"
        printf 'report : %s\n' "$report_display"
        printf 'log    : %s\n' "$log_display"
    } >"$detail_file"
}

stream_maven_output() {
    local line name status millis seconds color color_line result_status failure_type failure_detail
    local detail_file failure_index
    while IFS= read -r line || [[ -n "$line" ]]; do
        printf '%s\n' "$line" >>"$log_file"
        printf '%s\n' "$line" >>"$result_file"
        case "$line" in
            TEST_RESULT\|*)
                IFS='|' read -r _ name status millis <<<"$line"
                seconds=$(millis_to_seconds "$millis")
                case "$status" in
                    pass) color=$'\033[32m'; result_status=Done ;;
                    fail) color=$'\033[31m'; result_status=Failed ;;
                    skip) color=$'\033[33m'; result_status=Skip ;;
                    *) color=; result_status="$status" ;;
                esac
                if [[ "$print_mode" == maven ]]; then
                    printf '%s\n' "$line"
                else
                    printf '%s\n' "$(format_case_status_line "$name" "$color" "$status" "$seconds")"
                fi
                printf '===> %s [%s] %ss\n' "$name" "$result_status" "$seconds" >>"$result_file"
                write_case_detail_header "$name" "$result_status" "$seconds"
                if [[ "$status" == fail ]]; then
                    printf '%s\t%s\n' "$name" "$detail_file" >>"$failed_result_files_file"
                fi
                ;;
            TEST_FAILURE\|*)
                IFS='|' read -r _ failure_type name millis failure_detail <<<"$line"
                seconds=$(millis_to_seconds "$millis")
                failure_index=$(awk -F '\t' -v name="$name" '$1 == name { count++ } END { print count }' "$failed_result_files_file")
                detail_file=$(awk -F '\t' -v name="$name" -v match_index="$failure_index" \
                    '$1 == name && ++count == match_index { print $2; exit }' "$failed_result_files_file")
                if [[ -z "$detail_file" ]]; then
                    write_case_detail_header "$name" Failed "$seconds"
                    printf '%s\t%s\n' "$name" "$detail_file" >>"$failed_result_files_file"
                fi
                {
                    printf 'type   : %s\n' "$failure_type"
                    printf '\n'
                    printf '%s' "$failure_detail" | base64 -d
                    printf '\n'
                } >>"$detail_file"
                printf '%s|%s|%s|%s\n' "$failure_type" "$name" "$seconds" "$detail_file" >>"$failed_cases_file"
                if [[ "$failure_type" == configuration ]]; then
                    if [[ "$print_mode" == maven ]]; then
                        printf '%s\n' "$line"
                    else
                        printf '%s\n' "$(format_case_status_line "$name" $'\033[31m' fail "$seconds")"
                    fi
                    printf '===> %s [Failed] %ss\n' "$name" "$seconds" >>"$result_file"
                fi
                if [[ "$print_mode" == failed ]]; then
                    printf '    detail : %s\n' "${detail_file#"$case_dir"/}"
                    printf '%s' "$failure_detail" | base64 -d
                    printf '\n'
                fi
                ;;
            *)
                [[ "$print_mode" != maven ]] || printf '%s\n' "$line"
                ;;
        esac
    done
}
# 统计分散 XML 中的标签，仅用于每阶段验收，不输出逐用例状态。
count_xml_tag() {
    { grep -REoh "<$2[[:space:]/>]" "$1" --include='*.xml' 2>/dev/null || true; } | wc -l
}


# -p/-f 只改代理文件：保留所有登记了目标包/类的 <test>，因此原 suite 的
# groups、parameter、parallel 等运行上下文不会被错误套用。未登记目标时，只有
# 单 <test>（或无 <test>）suite 才能作为无歧义模板。class 内的 methods/parameter
# 无法在重建选择器时无损保留，因此明确拒绝。
filter_suite_by_level() {
    local file=$1 level=$2 name=$3 tmp_file rc=0
    tmp_file="${file}.filtered"
    awk -v level="$level" -v selected="$name" '
        function without_comments(line, start, finish) {
            while (xml_comment || (start = index(line, "<!--"))) {
                if (xml_comment) start = 1
                finish = index(substr(line, start), "-->")
                if (!finish) {
                    xml_comment = 1
                    return substr(line, 1, start - 1)
                }
                line = substr(line, 1, start - 1) substr(line, start + finish + 2)
                xml_comment = 0
            }
            return line
        }
        function attr_value(line, value) {
            if (match(line, /name[[:space:]]*=[[:space:]]*"[^"]*"/) == 0) return ""
            value = substr(line, RSTART, RLENGTH)
            sub(/^[^"]*"/, "", value)
            sub(/"$/, "", value)
            return value
        }
        function package_name(value, result) {
            result = value
            sub(/\.\*$/, "", result)
            return result
        }
        function package_of(class_name, result) {
            result = class_name
            sub(/\.[^.]+$/, "", result)
            return result
        }
        function is_under(child, parent) {
            return child == parent || index(child, parent ".") == 1
        }
        function selector_matches(line, value, package) {
            value = attr_value(line)
            if (value == "") return 0
            if (line ~ /<package[[:space:]]/) {
                package = package_name(value)
                if (level == "CLASS") return is_under(package_of(selected), package)
                return is_under(selected, package) || is_under(package, selected)
            }
            if (line ~ /<class[[:space:]]/) {
                if (level == "CLASS") return value == selected
                return is_under(package_of(value), selected)
            }
            return 0
        }
        function add_package(value) {
            if (index(package_seen, SUBSEP value SUBSEP) != 0) return
            package_seen = package_seen SUBSEP value SUBSEP
            package_values = package_values value "\n"
        }
        function add_class(value) {
            if (index(class_seen, SUBSEP value SUBSEP) != 0) return
            class_seen = class_seen SUBSEP value SUBSEP
            class_values = class_values value "\n"
        }
        function collect_intersection(block, generic, lines, count, i, line, value, package, in_classes) {
            package_seen = class_seen = package_values = class_values = ""
            count = split(block, lines, "\n")
            for (i = 1; i <= count; i++) {
                line = lines[i]
                if (line ~ /<classes([[:space:]]|>)/) in_classes = 1
                if (in_classes && line ~ /<(methods|parameter)([[:space:]]|>)/)
                    unsupported_selector = 1
                if (line ~ /<(package|class)[[:space:]]/) {
                    value = attr_value(line)
                    if (line ~ /<package[[:space:]]/) {
                        package = package_name(value)
                        if (is_under(selected, package)) add_package(selected)
                        else if (is_under(package, selected)) add_package(value)
                    } else if (value != "" && is_under(package_of(value), selected)) {
                        add_class(value)
                    }
                }
                if (line ~ /<\/classes>/) in_classes = 0
            }
            if (level == "CLASS") {
                package_values = class_values = ""
                add_class(selected)
                return
            }
            if (generic) {
                package_values = class_values = ""
                add_package(selected)
                return
            }
            if (package_values == "" && class_values == "") add_package(selected)
        }
        function emit_selectors(indent, values, count, i) {
            if (package_values != "") {
                print indent "<packages>"
                count = split(package_values, values, "\n")
                for (i = 1; i <= count; i++) if (values[i] != "")
                    printf "%s    <package name=\"%s\"/>\n", indent, values[i]
                print indent "</packages>"
            }
            if (class_values != "") {
                print indent "<classes>"
                count = split(class_values, values, "\n")
                for (i = 1; i <= count; i++) if (values[i] != "")
                    printf "%s    <class name=\"%s\"/>\n", indent, values[i]
                print indent "</classes>"
            }
        }
        function emit_selected_test(block, generic, lines, count, i, line, indent, skip_selector) {
            collect_intersection(block, generic)
            count = split(block, lines, "\n")
            if (lines[count] == "") count--
            skip_selector = 0
            for (i = 1; i <= count; i++) {
                line = lines[i]
                if (skip_selector) {
                    if (line ~ /<\/(packages|classes)>/) skip_selector = 0
                    continue
                }
                if (line ~ /<(packages|classes)([[:space:]]|>)/) {
                    skip_selector = line !~ /<\/(packages|classes)>/
                    continue
                }
                if (line ~ /<\/test>/) {
                    match(line, /^[ \t]*/)
                    indent = substr(line, 1, RLENGTH) "    "
                    emit_selectors(indent)
                }
                print line
            }
        }
        function emit_local_test() {
            print "    <test name=\"local-selection\">"
            package_values = class_values = ""
            if (level == "PACKAGE") add_package(selected); else add_class(selected)
            emit_selectors("        ")
            print "    </test>"
        }
        function flush_suite(i, generic) {
            generic = matched_tests == 0 && test_count <= 1
            for (i = 1; i <= segment_count; i++) {
                if (segment_kind[i] == "text") {
                    printf "%s", segment_content[i]
                } else if ((matched_tests > 0 && segment_match[i]) ||
                           (generic && test_count == 1)) {
                    emit_selected_test(segment_content[i], generic)
                }
            }
            if (test_count == 0) emit_local_test()
            if (matched_tests > 0) status = 0
            else if (test_count <= 1) status = 10
            else status = 11
        }
        {
            $0 = without_comments($0)
            if ($0 ~ /^[ \t]*$/) next
        }
        /^[ \t]*<test([[:space:]]|>)/ {
            in_test = 1
            test_block = $0 "\n"
            test_matches = 0
            next
        }
        in_test {
            test_block = test_block $0 "\n"
            if ($0 ~ /<(package|class)[[:space:]]/ && selector_matches($0)) test_matches = 1
            if ($0 ~ /<\/test>/) {
                in_test = 0
                segment_count++
                segment_kind[segment_count] = "test"
                segment_content[segment_count] = test_block
                segment_match[segment_count] = test_matches
                test_count++
                if (test_matches) matched_tests++
            }
            next
        }
        /<\/suite>/ {
            flush_suite()
            print $0
            suite_closed = 1
            next
        }
        {
            segment_count++
            segment_kind[segment_count] = "text"
            segment_content[segment_count] = $0 "\n"
        }
        END {
            if (!suite_closed) status = 12
            if (unsupported_selector) status = 13
            exit status
        }
    ' "$file" >"$tmp_file" || rc=$?
    case "$rc" in
        0|10) mv -- "$tmp_file" "$file" ;;
        *) rm -f -- "$tmp_file" ;;
    esac
    return "$rc"
}

set_suite_parameter() {
    local file=$1 parameter_name=$2 parameter_value=$3 tmp_file
    tmp_file="${file}.parameter"
    if grep -q "name=\"$parameter_name\"" "$file"; then
        awk -v target="$parameter_name" -v replacement="$parameter_value" '
            index($0, "name=\"" target "\"") {
                sub(/value="[^"]*"/, "value=\"" replacement "\"")
            }
            { print }
        ' "$file" >"$tmp_file"
    else
        awk -v target="$parameter_name" -v replacement="$parameter_value" '
            /<\/suite>/ {
                print "    <parameter name=\"" target "\" value=\"" replacement "\"/>"
            }
            { print }
        ' "$file" >"$tmp_file"
    fi
    mv -- "$tmp_file" "$file"
}

set_suite_timeout() {
    local file=$1 timeout_millis=$2 tmp_file
    tmp_file="${file}.timeout"
    awk -v timeout="$timeout_millis" '
        !updated && /<suite[[:space:]>]/ {
            if ($0 ~ /time-out="[^"]*"/) {
                sub(/time-out="[^"]*"/, "time-out=\"" timeout "\"")
            } else {
                sub(/>/, " time-out=\"" timeout "\">")
            }
            updated = 1
        }
        { print }
    ' "$file" >"$tmp_file"
    mv -- "$tmp_file" "$file"
}
prepare_proxy_suite() {
    local proxy_file=$1
    if [[ "$test_type" == CONFIGURETEST || "$test_type" == MVCCTEST ]]; then
        set_suite_parameter "$proxy_file" CONFTOOL "$test_basedir/tools/changeConf.js"
    fi
    if [[ -n "$test_level_type" ]]; then
        selection_rc=0
        filter_suite_by_level "$proxy_file" "$test_level_type" "$exec_testcase_name" || selection_rc=$?
        case "$selection_rc" in
            0|10) ;;
            13) die "cannot filter suite with class-level <methods> or <parameter>: $proxy_file" ;;
            *) die "cannot select $exec_testcase_name from suite: $proxy_file" ;;
        esac
    fi
    if ((timeout_seconds > 0)); then
        set_suite_timeout "$proxy_file" "$((timeout_seconds * 1000))"
    fi
    if is_s3_testcase_type "$testcase_type" || grep -q 'com\.sequoias3\.' "$proxy_file"; then
        set_suite_parameter "$proxy_file" S3HOSTNAME "$s3_hostname"
        set_suite_parameter "$proxy_file" S3PORT "$s3_service"
    fi
}

is_local_host() {
    case "$1" in
        localhost|127.0.0.1|::1|"$(hostname)"|"$(hostname -f 2>/dev/null)") return 0 ;;
        *) return 1 ;;
    esac
}

# normal_test 使用 s3_java，旧 runtest_java.xml/configure_test 使用
# thirdparty_s3_java。二者指向同一个 S3 Maven 工程，运行语义必须一致。
is_s3_testcase_type() {
    case "$1" in
        s3_java|thirdparty_s3_java) return 0 ;;
        *) return 1 ;;
    esac
}

is_flink_testcase_type() {
    case "$1" in
        flink_java|thirdparty_flink_java) return 0 ;;
        *) return 1 ;;
    esac
}

# 读已安装二进制的 --version。CI 执行机未必有引擎，优先 SSH 到 -h 对应节点。
read_installed_sdb_version() {
    local bin=$1 text=""
    if is_local_host "$db_hostname"; then
        [[ -x "$bin" ]] || return 1
        text=$("$bin" --version 2>/dev/null || true)
    else
        text=$(run_configure_command "$bin" --version 2>/dev/null || true)
    fi
    [[ -n "$text" ]] || return 1
    printf '%s' "$text"
}

select_reliability_suite_files() {
    local version_text=""
    version_text=$(read_installed_sdb_version "$install_dir/bin/sequoiadb" || true)
    if [[ -z "$version_text" && -f "$install_dir/sequoiadb.version" ]]; then
        version_text=$(cat -- "$install_dir/sequoiadb.version" || true)
    fi
    [[ -n "$version_text" ]] || \
        die "cannot detect SequoiaDB Debug/Release from $install_dir (host $db_hostname)"
    suite_files+=("$case_dir/s3testng.xml" "$case_dir/testng.xml")
    if [[ "$version_text" != *Debug* ]]; then
        suite_files+=("$case_dir/testng_release.xml")
    fi
}

# 本地执行时优先使用当前身份或免密 sudo；否则保持 SSH 登录能力。
run_configure_command() {
    local current_user command_text arg
    current_user=$(id -un)

    if is_local_host "$db_hostname"; then
        if [[ "$remote_user" == "$current_user" || $(id -u) -eq 0 ]]; then
            "$@"
            return
        fi
        if [[ "$remote_user" == root ]] && command -v sudo >/dev/null 2>&1 && sudo -n true 2>/dev/null; then
            sudo -n "$@"
            return
        fi
    fi

    command_text=
    for arg in "$@"; do
        printf -v arg '%q' "$arg"
        command_text+="${command_text:+ }$arg"
    done
    if [[ -n "$remote_password" ]]; then
        if ! command -v sshpass >/dev/null 2>&1; then
            echo "[runtest_java][ERROR] CONFIGURETEST requires sshpass for password-based SSH to $remote_user@$db_hostname" >&2
            return 2
        fi
        sshpass -p "$remote_password" ssh -o StrictHostKeyChecking=no \
            "$remote_user@$db_hostname" "$command_text"
    else
        ssh -o BatchMode=yes -o StrictHostKeyChecking=no \
            "$remote_user@$db_hostname" "$command_text"
    fi
}

script_dir=$(cd -- "$(dirname -- "${BASH_SOURCE[0]}")" && pwd -P)
testcase_root=$(cd -- "$script_dir/.." && pwd -P)

case_dir="${CASE_DIR:-$testcase_root/story/java}"
test_basedir=$testcase_root
report_dir="${REPORT_DIR:-}"
log_file="${LOG_FILE:-}"
timeout_seconds="${JAVA_TIMEOUT:-0}"
test_type="${TEST_TYPE:-NORMALTEST}"
testcase_type="${TESTCASE_TYPE:-}"
install_dir_arg=
java_home_arg=
db_hostname_arg=
svc_name_arg=
project_type=
test_mode=
package_name=
test_file=
custom_case_dir=false
stop_flag=0
print_mode=compact
remote_user_arg=
remote_password_arg=
root_password_arg=
conf_file=
test_level_type=
exec_testcase_name=
suite=
dry_run="${JAVA_DRY_RUN:-false}"
report_format="${REPORT_FORMAT:-}"
reportinfo="${REPORTINFO:-}"
# Mongo Java 工程用 mongo-java-driver 替代 SequoiaDB driver，但仍引用工程内的
# jars/threadexecutor-1.0.0-SNAPSHOT.jar。
# 传入该 JAR 即进入 Mongo 模式；多版本由调用方对每个 JAR 分别调用本脚本。
mongo_driver_jar="${MONGO_DRIVER_JAR:-}"

parse_args() {
while (($#)); do
    case "$1" in
        -p) [[ $# -ge 2 ]] || die "missing value for -p"; package_name=$2; shift 2 ;;
        -f) [[ $# -ge 2 ]] || die "missing value for -f"; test_file=$2; shift 2 ;;
        -t) [[ $# -ge 2 ]] || die "missing value for -t"; project_type=$2; shift 2 ;;
        -m) [[ $# -ge 2 ]] || die "missing value for -m"; test_mode=$2; shift 2 ;;
        -s) [[ $# -ge 2 ]] || die "missing value for -s"; stop_flag=$2; shift 2 ;;
        --case-dir) [[ $# -ge 2 ]] || die "missing value for --case-dir"; case_dir=$2; custom_case_dir=true; shift 2 ;;
        --suite) [[ $# -ge 2 ]] || die "missing value for --suite"; suite=$2; shift 2 ;;
        --testcase-type) [[ $# -ge 2 ]] || die "missing value for --testcase-type"; testcase_type=$2; shift 2 ;;
        --report-dir) [[ $# -ge 2 ]] || die "missing value for --report-dir"; report_dir=$2; shift 2 ;;
        --install-dir) [[ $# -ge 2 ]] || die "missing value for --install-dir"; install_dir_arg=$2; shift 2 ;;
        -n) [[ $# -ge 2 ]] || die "missing value for -n"; svc_name_arg=$2; shift 2 ;;
        -h)
            if [[ $# -lt 2 || "$2" == -* ]]; then
                usage
                exit 0
            fi
            db_hostname_arg=$2
            shift 2
            ;;
        --user) [[ $# -ge 2 ]] || die "missing value for --user"; remote_user_arg=$2; shift 2 ;;
        --password) [[ $# -ge 2 ]] || die "missing value for --password"; remote_password_arg=$2; shift 2 ;;
        --rootpwd) [[ $# -ge 2 ]] || die "missing value for --rootpwd"; root_password_arg=$2; shift 2 ;;
        --java-home) [[ $# -ge 2 ]] || die "missing value for --java-home"; java_home_arg=$2; shift 2 ;;
        --conf)
            [[ $# -ge 2 ]] || die "missing value for --conf"
            [[ -n "$2" ]] || die "missing value for --conf"
            conf_file=$2
            shift 2
            ;;
        --timeout) [[ $# -ge 2 ]] || die "missing value for --timeout"; timeout_seconds=$2; shift 2 ;;
        --log-file) [[ $# -ge 2 ]] || die "missing value for --log-file"; log_file=$2; shift 2 ;;
        --mongo-driver-jar) [[ $# -ge 2 ]] || die "missing value for --mongo-driver-jar"; mongo_driver_jar=$2; shift 2 ;;
        -print)
            print_mode=failed
            if [[ $# -ge 2 && "$2" != -* ]]; then
                case "$2" in
                    failed|maven) print_mode=$2 ;;
                    *) die "unsupported -print mode: $2 (use failed or maven)" ;;
                esac
                shift 2
            else
                shift
            fi
            ;;
        --dry-run) dry_run=true; shift ;;
        --reportFormat)
            [[ $# -ge 2 ]] || die "missing value for --reportFormat (jenkins)"
            report_format=$2
            shift 2
            ;;
        # 不在本脚本解析，原样交给 formatter。
        --reportinfo)
            [[ $# -ge 2 ]] || die "missing value for --reportinfo"
            [[ -n "$2" ]] || die "missing value for --reportinfo"
            reportinfo=$2
            shift 2
            ;;
        --help) usage; exit 0 ;;
        *) die "unknown option: $1" ;;
    esac
done
}

parse_args "$@"

command -v realpath >/dev/null 2>&1 || die "realpath not found in PATH"
[[ "$stop_flag" == 0 || "$stop_flag" == 1 ]] || die "-s only supports 0 or 1"
[[ "$timeout_seconds" =~ ^[0-9]+$ ]] || die "--timeout must be a non-negative integer"
if [[ -n "$report_format" ]]; then
    report_format=$(printf '%s' "$report_format" | tr '[:upper:]' '[:lower:]')
    [[ "$report_format" == jenkins ]] || \
        die "unsupported report format: $report_format (only jenkins is supported)"
    formatter="$script_dir/report-formatters/${report_format}.sh"
    [[ -f "$formatter" ]] || die "report formatter not found: $formatter"
fi

# 丢掉环境里残留的 conf_*，只让 --conf 文件写入这些变量。
unset -v conf_DB_HOSTNAME conf_SVCNAME conf_INSTALL_DIR conf_JAVA_HOME \
    conf_ES_IP conf_ES_PORT conf_ES_PORT_RELIABILITY conf_S3HOSTNAME conf_S3SVCNAME \
    conf_S3CLIENTDIR conf_ROOT_USER conf_ROOT_PWD conf_SDBDRIVERDIR \
    conf_THREADEXECUTOR_DIR conf_RSRVNODEDIR conf_SCRIPTDIR conf_FULLTEXTPREFIX \
    conf_COORDS conf_USERNAME conf_PASSWORD \
    conf_MYSQLURL conf_MYSQLUSERNAME conf_MYSQLPASSWORD
if [[ -z "$conf_file" && -f "$script_dir/conf/test.conf" ]]; then
    conf_file="$script_dir/conf/test.conf"
fi
if [[ -n "$conf_file" ]]; then
    load_conf "$conf_file"
fi

if [[ -n "$project_type" ]]; then
    project_type=$(printf '%s' "$project_type" | tr '[:upper:]' '[:lower:]')
    case "$project_type" in
        story) case_dir="$testcase_root/story/java"; : "${testcase_type:=story_java}" ;;
        story_at) case_dir="$testcase_root/story_at/java"; : "${testcase_type:=story_at_java}" ;;
        sdv) case_dir="$testcase_root/sdv/java"; : "${testcase_type:=sdv_java}" ;;
        driver) case_dir="$testcase_root/driver/java"; : "${testcase_type:=driver_java}" ;;
        reliability) case_dir="$testcase_root/reliability/java"; : "${testcase_type:=reliability}" ;;
        s3) case_dir="$testcase_root/thirdparty/s3/java"; : "${testcase_type:=thirdparty_s3_java}" ;;
        flink) case_dir="$testcase_root/thirdparty/flink/java"; : "${testcase_type:=thirdparty_flink_java}" ;;
        *) die "unsupported -t: $project_type" ;;
    esac
fi

if [[ -n "$test_mode" ]]; then
    test_mode=$(printf '%s' "$test_mode" | tr '[:upper:]' '[:lower:]')
    case "$test_mode" in
        normal|normaltest) test_type=NORMALTEST ;;
        smoke|smoketest) test_type=SMOKETEST ;;
        sync|synctest) test_type=SYNCTEST ;;
        largedata|largedatatest) test_type=LARGEDATATEST ;;
        configure|configuretest) test_type=CONFIGURETEST ;;
        reliability|reliabilitytest) test_type=RELIABILITYTEST ;;
        mvcc|mvcctest) test_type=MVCCTEST ;;
        *) die "unsupported -m: $test_mode" ;;
    esac
fi

[[ -z "$package_name" || -z "$test_file" ]] || die "-p and -f are mutually exclusive"

if [[ -n "$package_name" ]]; then
    test_level_type=PACKAGE
    exec_testcase_name=$package_name
fi

[[ -z "$project_type" || "$custom_case_dir" == false ]] || die "-t and --case-dir are mutually exclusive"
case_dir=$(abs_path "$case_dir")
test_basedir=$(abs_path "$test_basedir")

[[ -d "$case_dir" ]] || die "case directory not found: $case_dir"
[[ -f "$case_dir/pom.xml" ]] || die "pom.xml not found: $case_dir/pom.xml"
[[ -d "$test_basedir/tools" ]] || die "test tools directory not found: $test_basedir/tools"

test_source_dir="$case_dir/src/test/java"
if [[ -n "$package_name" ]]; then
    package_dir="$test_source_dir/${package_name//./\/}"
    [[ -d "$package_dir" ]] || die "Java package not found: $package_dir"
    package_name=${package_dir#"$test_source_dir"/}
    package_name=${package_name//\//.}
fi

if [[ -n "$test_file" ]]; then
    [[ "$test_file" == *.java ]] || test_file+=.java
    test_file_path="$test_source_dir/${test_file#./}"
    [[ -f "$test_file_path" ]] || die "Java test file not found: $test_file_path"
    exec_testcase_name=${test_file_path#"$test_source_dir"/}
    exec_testcase_name=${exec_testcase_name%.java}
    exec_testcase_name=${exec_testcase_name//\//.}
    test_level_type=CLASS
fi

test_type=$(printf '%s' "$test_type" | tr '[:lower:]' '[:upper:]')
if [[ -z "$testcase_type" ]]; then
    case "$case_dir" in
        */story/java) testcase_type=story_java ;;
        */story_at/java) testcase_type=story_at_java ;;
        */sdv/java) testcase_type=sdv_java ;;
        */driver/java) testcase_type=driver_java ;;
        */reliability/java) testcase_type=reliability ;;
        */thirdparty/s3/java) testcase_type=thirdparty_s3_java ;;
        */thirdparty/flink/java) testcase_type=thirdparty_flink_java ;;
        *) testcase_type=$(basename "$(dirname "$case_dir")")_java ;;
    esac
fi
# 与 runtest.sh 的 -t 对齐：选了可靠性工程且未指定 -m 时，按安装包选择 TestNG XML。
if [[ "$testcase_type" == reliability && -z "$test_mode" ]]; then
    test_type=RELIABILITYTEST
fi

# --suite 优先；否则按工程/模式选文件。reliability 按已安装包 Debug/Release 选 XML。
suite_files=()
if [[ -n "$suite" ]]; then
    [[ "$suite" == /* ]] || suite="$case_dir/$suite"
    suite_files+=("$(abs_path "$suite")")
elif [[ -n "$mongo_driver_jar" ]] || is_s3_testcase_type "$testcase_type"; then
    suite_files+=("$case_dir/testng.xml")
elif is_flink_testcase_type "$testcase_type"; then
    suite_files+=("$case_dir/src/test/resources/testng.xml")
else
    case "$test_type" in
        NORMALTEST) suite_files+=("$case_dir/testng.xml") ;;
        SMOKETEST) suite_files+=("$case_dir/smoke.xml") ;;
        LARGEDATATEST) suite_files+=("$case_dir/largedata.xml") ;;
        SYNCTEST) suite_files+=("$case_dir/sync.xml") ;;
        CONFIGURETEST|MVCCTEST) suite_files+=("$case_dir/configure.xml") ;;
        RELIABILITYTEST)
            first_set install_dir "$install_dir_arg" "${INSTALL_DIR-}" "${conf_INSTALL_DIR-}" "/opt/sequoiadb"
            first_set db_hostname "$db_hostname_arg" "${DB_HOSTNAME-}" "${conf_DB_HOSTNAME-}" "localhost"
            first_set remote_user "$remote_user_arg" "${REMOTEUSER-}" "${conf_ROOT_USER-}" "$(id -un)"
            first_set remote_password "$remote_password_arg" "${REMOTEPASSWD-}" "${conf_ROOT_PWD-}"
            install_dir=$(abs_path "$install_dir")
            select_reliability_suite_files
            ;;
        *) die "unsupported test mode: $test_type" ;;
    esac
fi

for suite_file in "${suite_files[@]}"; do
    [[ -f "$suite_file" ]] || die "suite XML not found: $suite_file"
done

if [[ -n "$test_level_type" ]]; then
    candidate_count=${#suite_files[@]}
    matched_suites=()
    for suite_file in "${suite_files[@]}"; do
        suite_probe=$(mktemp "${TMPDIR:-/tmp}/runtest-java-suite.XXXXXX")
        cp -- "$suite_file" "$suite_probe"
        selection_rc=0
        filter_suite_by_level "$suite_probe" "$test_level_type" "$exec_testcase_name" || selection_rc=$?
        rm -f -- "$suite_probe"
        case "$selection_rc" in
            0) matched_suites+=("$suite_file") ;;
            10)
                if ((candidate_count == 1)); then
                    matched_suites+=("$suite_file")
                fi
                ;;
            11) ;;
            13) die "cannot filter suite with class-level <methods> or <parameter>: $suite_file" ;;
            *) die "cannot inspect suite XML: $suite_file" ;;
        esac
    done
    ((${#matched_suites[@]} > 0)) || \
        die "$exec_testcase_name is not registered in an unambiguous <test>; pass a dedicated --suite"
    suite_files=("${matched_suites[@]}")
fi

# 默认 < conf < env < CLI。空字符串视为该层未设置。
# ES 端口跟最终 mode 走，所以等 test_type 定了再落地。
first_set install_dir "$install_dir_arg" "${INSTALL_DIR-}" "${conf_INSTALL_DIR-}" "/opt/sequoiadb"
first_set java_home "$java_home_arg" "${JAVA_HOME-}" "${conf_JAVA_HOME-}"
first_set db_hostname "$db_hostname_arg" "${DB_HOSTNAME-}" "${conf_DB_HOSTNAME-}" "localhost"
first_set svc_name "$svc_name_arg" "${SVCNAME-}" "${conf_SVCNAME-}" "11810"
first_set s3_hostname "${S3HOSTNAME-}" "${conf_S3HOSTNAME-}" "$db_hostname"
# CI 对外使用 S3SVCNAME；生成代理 suite 时映射为 S3TestBase 读取的 S3PORT。
first_set s3_service "${S3SVCNAME-}" "${conf_S3SVCNAME-}" "8002"
first_set es_hostname "${ESHOSTNAME-}" "${conf_ES_IP-}"
if [[ "$test_type" == RELIABILITYTEST ]]; then
    first_set es_service "${ESSVCNAME-}" "${conf_ES_PORT_RELIABILITY-}"
else
    first_set es_service "${ESSVCNAME-}" "${conf_ES_PORT-}"
fi
# 与 exec_java_case 原 IDXPREFIX 规则一致：取 COORD（-h）并把第一个 '-' 换成 '_'。
first_set fulltext_prefix "${FULLTEXTPREFIX-}" "${conf_FULLTEXTPREFIX-}" "${db_hostname/-/_}"
first_set remote_user "$remote_user_arg" "${REMOTEUSER-}" "${conf_ROOT_USER-}" "$(id -un)"
first_set remote_password "$remote_password_arg" "${REMOTEPASSWD-}" "${conf_ROOT_PWD-}"
first_set root_password "$root_password_arg" "${ROOTPASSWD-}" "${conf_ROOT_PWD-}"
install_dir=$(abs_path "$install_dir")
first_set rsrv_node_dir "${RSRVNODEDIR-}" "${conf_RSRVNODEDIR-}" "$install_dir/database/"
first_set reliability_script_dir "${SCRIPTDIR-}" "${conf_SCRIPTDIR-}" "$test_basedir/reliability/java/script"
reliability_script_dir=$(abs_path "$reliability_script_dir")
first_set driver_dir_input "${SDBDRIVERDIR-}" "${conf_SDBDRIVERDIR-}"
first_set threadexecutor_dir "${THREADEXECUTOR_DIR-}" "${conf_THREADEXECUTOR_DIR-}" "$test_basedir/tools"
first_set s3client_dir "${S3CLIENTDIR-}" "${conf_S3CLIENTDIR-}" "$install_dir/tools/sequoias3/java"

# Mongo 模式：用调用方指定的 mongo-java-driver，版本号从文件名剥取。
# pom 通过 jars.dir 引用工程内 threadexecutor，执行前只确认该 JAR 存在。
if [[ -n "$mongo_driver_jar" ]]; then
    mongo_driver_jar=$(abs_path "$mongo_driver_jar")
    [[ -f "$mongo_driver_jar" ]] || die "mongo driver JAR not found: $mongo_driver_jar"
    mongo_driver_name=${mongo_driver_jar##*/}
    mongo_driver_version=${mongo_driver_name#mongo-java-driver-}
    mongo_driver_version=${mongo_driver_version%.jar}
    [[ -f "$case_dir/jars/threadexecutor-1.0.0-SNAPSHOT.jar" ]] || \
        die "Mongo threadexecutor JAR not found: $case_dir/jars/threadexecutor-1.0.0-SNAPSHOT.jar"
else
    repo_driver_dir=$(abs_path "$testcase_root/../driver/java/target")
    driver_jar=
    if [[ -n "$driver_dir_input" ]]; then
        driver_dir=$(abs_path "$driver_dir_input")
    elif driver_jar=$(find_versioned_jar "$repo_driver_dir" sequoiadb-driver); then
        driver_dir=$repo_driver_dir
    else
        driver_dir=$(abs_path "$install_dir/java")
    fi
    threadexecutor_dir=$(abs_path "$threadexecutor_dir")

    if [[ -z "$driver_jar" ]]; then
        driver_jar=$(find_versioned_jar "$driver_dir" sequoiadb-driver) || \
            die "SequoiaDB Java Driver not found: $driver_dir/sequoiadb-driver-*.jar"
    fi
    driver_name=${driver_jar##*/}
    driver_version=${driver_name#sequoiadb-driver-}
    driver_version=${driver_version%.jar}

    threadexecutor_jar="$threadexecutor_dir/threadexecutor-1.0.0-SNAPSHOT.jar"
    [[ -f "$threadexecutor_jar" ]] || die "threadexecutor JAR not found: $threadexecutor_jar"

    if is_s3_testcase_type "$testcase_type"; then
        s3client_dir=$(abs_path "$s3client_dir")
        s3client_jar=$(find_versioned_jar "$s3client_dir" sequoias3-client) || \
            die "SequoiaS3 client not found: $s3client_dir/sequoias3-client-*.jar"
        s3client_name=${s3client_jar##*/}
        s3client_version=${s3client_name#sequoias3-client-}
        s3client_version=${s3client_version%.jar}
    fi
fi

if [[ -z "$java_home" ]]; then
    for candidate in "$HOME"/.local/jdks/jdk8* /opt/jdk1.8* /usr/lib/jvm/java-8*; do
        if [[ -x "$candidate/bin/java" ]]; then
            java_home=$candidate
            break
        fi
    done
fi
[[ -n "$java_home" ]] || die "JAVA_HOME not found; install it or pass --java-home"
java_home=$(abs_path "$java_home")
[[ -x "$java_home/bin/java" ]] || die "java not found: $java_home/bin/java"
command -v mvn >/dev/null 2>&1 || die "mvn not found in PATH"

# Maven/Surefire 的 reportsDirectory 读取 -DreportDir，因此原始报告必须落在 case-dir/output。
# report_dir 只兼容旧 CI 的显式副本目录；本地未指定时不再创建 local-report。
run_dir="$case_dir/output"
report_source="$run_dir/junitreports"
[[ -n "$log_file" ]] || log_file="$run_dir/runtest_java.log"
if [[ -n "$report_dir" ]]; then
    report_dir=$(abs_path "$report_dir")
fi
log_file=$(abs_path "$log_file")

# 不传 -DBACKUPTMPNODELOGPATH 是有意的：reliability 与 story configure suite 把该参数声明为
# value="${BACKUPTMPNODELOGPATH}"，SdbTestBase 用「值仍等于这个字面量」判断调用方未提供备份路径
# （见 reliability SdbTestBase 对 "${BACKUPTMPNODELOGPATH}" 的比较）。一旦传入真实值，
# 该哨兵判断失效。需要指定备份路径时应改用例侧约定，不要在此补参数。

maven_common_args=(surefire-report:report)
if [[ -n "$mongo_driver_jar" ]]; then
    maven_common_args+=(
        "-Dmongo-java-driver=$mongo_driver_version"
    )
else
    maven_common_args+=(
        "-Dsdbdriver=$driver_version"
        "-DthreadexecutorDir=$threadexecutor_dir"
        "-DsdbdriverDir=$driver_dir"
    )
    if is_s3_testcase_type "$testcase_type"; then
        maven_common_args+=(
            "-Ds3client=$s3client_version"
            "-Ds3clientDir=$s3client_dir"
        )
    fi
fi
if [[ "$stop_flag" == 1 ]]; then
    maven_common_args+=("-Dsurefire.skipAfterFailureCount=1")
fi
maven_common_args+=(
    "-DreportDir=$run_dir"
    "-DHOSTNAME=$db_hostname"
    "-DSVCNAME=$svc_name"
    "-DS3HOSTNAME=$s3_hostname"
    "-DS3SVCNAME=$s3_service"
    "-DESHOSTNAME=$es_hostname"
    "-DESSVCNAME=$es_service"
    "-DFULLTEXTPREFIX=$fulltext_prefix"
    "-DREMOTEUSER=$remote_user"
    "-DREMOTEPASSWD=$remote_password"
    "-DROOTPASSWD=$root_password"
    "-DRSRVNODEDIR=$rsrv_node_dir"
    "-DSCRIPTDIR=$reliability_script_dir"
)

# Flink suite 使用独立的连接参数名。COORDS 可由通用 COORD 自动推导，
# 数据库/MySQL 用户和密码由环境变量传入，避免再扩展低频公共 CLI 参数。
if is_flink_testcase_type "$testcase_type"; then
    first_set coords "${COORDS-}" "${conf_COORDS-}" "$db_hostname:$svc_name"
    first_set flink_username "${USERNAME-}" "${conf_USERNAME-}"
    first_set flink_password "${PASSWORD-}" "${conf_PASSWORD-}"
    first_set mysql_url "${MYSQLURL-}" "${MYSQLADDRESS-}" "${conf_MYSQLURL-}"
    first_set mysql_username "${MYSQLUSERNAME-}" "${conf_MYSQLUSERNAME-}"
    first_set mysql_password "${MYSQLPASSWORD-}" "${conf_MYSQLPASSWORD-}"
    maven_common_args+=(
        "-DCOORDS=$coords"
        "-DUSERNAME=$flink_username"
        "-DPASSWORD=$flink_password"
        "-DMYSQLURL=$mysql_url"
        "-DMYSQLUSERNAME=$mysql_username"
        "-DMYSQLPASSWORD=$mysql_password"
    )
fi

case "$testcase_type" in
    story_java|story_at_java|sdv_java|driver_java|flink_java)
        display_project=${testcase_type%_java}
        ;;
    thirdparty_s3_java) display_project=s3 ;;
    thirdparty_flink_java) display_project=flink ;;
    reliability) display_project=reliability ;;
    *) display_project="${project_type:-$testcase_type}" ;;
esac
display_mode=$(printf '%s' "${test_type%TEST}" | tr '[:upper:]' '[:lower:]')
if [[ -n "$package_name" ]]; then
    display_selection="package $package_name"
elif [[ -n "$test_file" ]]; then
    display_selection="file $exec_testcase_name"
elif [[ -n "$suite" ]]; then
    display_selection="suite $(basename "$suite")"
else
    display_selection="all cases"
fi

# 不直接修改用户维护的 suite：每个阶段使用进程唯一的代理文件。
proxy_files=()
for ((suite_index = 0; suite_index < ${#suite_files[@]}; suite_index++)); do
    proxy_files+=("$case_dir/.runtest_java.$$.$((suite_index + 1)).xml")
done

if [[ "$dry_run" == true ]]; then
    echo "[runtest_java] project=$display_project mode=$display_mode case_dir=$case_dir"
    for proxy_file in "${proxy_files[@]}"; do
        print_command mvn "${maven_common_args[@]}" "-DxmlFileName=$(basename "$proxy_file")"
    done
    exit 0
fi

# 同一 Maven 工程固定使用 case-dir/output。并发执行会互相删除日志和报告，
# 因此实际运行必须持有进程级互斥锁；进程退出后 flock 会由内核自动释放。
command -v flock >/dev/null 2>&1 || \
    die "flock command not found; run this script in WSL or Linux with util-linux installed"
exec 9<"$case_dir/pom.xml"
if ! flock -n 9; then
    die "another Java test is already running for case directory: $case_dir"
fi

# CONFIGURETEST 的前后置处理只属于旧 CI 中的 story_java。
configure_cleanup_needed=false
configure_helper_dir=
configure_remote_helper_dir=
active_maven_pid=
active_tee_pid=
output_fifo=
output_pipe_dir=

terminate_process_tree() {
    local parent_pid=$1 child_pid
    while read -r child_pid; do
        [[ -n "$child_pid" ]] || continue
        terminate_process_tree "$child_pid"
    done < <(pgrep -P "$parent_pid" 2>/dev/null || true)
    kill -TERM "$parent_pid" 2>/dev/null || true
}

# removeDSCluster.js 在部分工作树中是 CRLF，复制后统一转为 LF。
prepare_configure_helpers() {
    local source_dir="$test_basedir/tools" helper
    [[ -n "$configure_helper_dir" ]] && return 0
    for helper in before_configuretest.sh after_configuretest.sh removeDSCluster.js; do
        [[ -f "$source_dir/$helper" ]] || {
            echo "[runtest_java][ERROR] CONFIGURETEST helper not found: $source_dir/$helper" >&2
            return 1
        }
    done
    configure_helper_dir=$(mktemp -d "${TMPDIR:-/tmp}/runtest-java-configure.XXXXXX") || return 1
    for helper in before_configuretest.sh after_configuretest.sh removeDSCluster.js; do
        sed 's/\r$//' "$source_dir/$helper" >"$configure_helper_dir/$helper" || return 1
    done
    # localhost 可能通过 SSH 切换到 CI 配置的 remote_user，临时 helper 需可读。
    chmod 0755 "$configure_helper_dir" || return 1
    chmod 0644 "$configure_helper_dir"/* || return 1
    if ! is_local_host "$db_hostname"; then
        configure_remote_helper_dir=$(run_configure_command mktemp -d /tmp/runtest-java-configure.XXXXXX) || return 1
        tar -C "$configure_helper_dir" -cf - before_configuretest.sh after_configuretest.sh removeDSCluster.js |
            run_configure_command tar -C "$configure_remote_helper_dir" -xf - || return 1
    fi
}

run_configure_before() {
    local before_script
    prepare_configure_helpers || return 1
    before_script="${configure_remote_helper_dir:-$configure_helper_dir}/before_configuretest.sh"

    configure_cleanup_needed=true
    echo "[runtest_java] CONFIGURETEST before: $before_script"
    run_configure_command bash "$before_script" "$db_hostname" "$svc_name" "$install_dir"
}

run_configure_after() {
    local after_script
    local schedule_log="$install_dir/tools/sdb-schedule/sdb-schedule/log"
    local backup_dir="$run_dir/toolslog/sdbschedule"
    prepare_configure_helpers || return 1
    after_script="${configure_remote_helper_dir:-$configure_helper_dir}/after_configuretest.sh"

    echo "[runtest_java] CONFIGURETEST after: $after_script"
    if [[ -d "$schedule_log" ]]; then
        mkdir -p -- "$(dirname "$backup_dir")"
        cp -a -- "$schedule_log" "$backup_dir" || true
    fi
    run_configure_command bash "$after_script" "$db_hostname" "$svc_name" "$install_dir"
}

# 无论 Maven 成功、失败或被中断，都清理代理 suite，并尽力恢复配置测试环境。
cleanup() {
    local original_rc=$?
    trap - EXIT
    if [[ -n "$active_maven_pid" ]]; then
        terminate_process_tree "$active_maven_pid"
    fi
    if [[ -n "$active_tee_pid" ]]; then
        kill -TERM "$active_tee_pid" 2>/dev/null || true
    fi
    [[ -z "$output_fifo" ]] || rm -f -- "$output_fifo"
    [[ -z "$output_pipe_dir" ]] || rmdir -- "$output_pipe_dir" 2>/dev/null || true
    rm -f -- "${proxy_files[@]}"
    if [[ "$configure_cleanup_needed" == true ]]; then
        if ! run_configure_after; then
            echo "[runtest_java][ERROR] CONFIGURETEST cleanup failed" >&2
            # 保留中断信号；普通测试结论遇到清理失败转为环境失败。
            if ((original_rc == 0 || original_rc == 1)); then original_rc=2; fi
        fi
    fi
    if [[ -n "$configure_remote_helper_dir" ]]; then
        if ! run_configure_command rm -f -- "$configure_remote_helper_dir/before_configuretest.sh" \
            "$configure_remote_helper_dir/after_configuretest.sh" \
            "$configure_remote_helper_dir/removeDSCluster.js" ||
            ! run_configure_command rmdir -- "$configure_remote_helper_dir"; then
            echo "[runtest_java][ERROR] CONFIGURETEST remote helper cleanup failed" >&2
            if ((original_rc == 0 || original_rc == 1)); then original_rc=2; fi
        fi
    fi
    if [[ -n "$configure_helper_dir" ]]; then
        rm -f -- "$configure_helper_dir/before_configuretest.sh" \
            "$configure_helper_dir/after_configuretest.sh" \
            "$configure_helper_dir/removeDSCluster.js"
        rmdir -- "$configure_helper_dir" 2>/dev/null || true
    fi
    exit "$original_rc"
}
trap cleanup EXIT
trap 'exit 130' INT
trap 'exit 143' TERM

for ((suite_index = 0; suite_index < ${#suite_files[@]}; suite_index++)); do
    suite_file=${suite_files[$suite_index]}
    proxy_file=${proxy_files[$suite_index]}
    cp -- "$suite_file" "$proxy_file"
    prepare_proxy_suite "$proxy_file"
done

# JUnit 仍写在工程 output/（Maven -DreportDir）。可读报告：
# local_test_report/java/<类型>/<模式>/。全量清空该目录；-p/-f 只清对应包或类。
rm -rf -- "$run_dir"
repo_root=$(cd -- "$script_dir/../.." && pwd -P)
local_report_dir="$repo_root/local_test_report/java/$display_project/$display_mode"
local_report_clear=$local_report_dir
if [[ -n "$package_name" ]]; then
    local_report_clear="$local_report_dir/${package_name//./\/}"
    local_report_clear=${local_report_clear/#$local_report_dir\/com\/sequoiadb\//$local_report_dir/}
elif [[ -n "$exec_testcase_name" ]]; then
    local_report_clear="$local_report_dir/${exec_testcase_name//./\/}"
    local_report_clear=${local_report_clear/#$local_report_dir\/com\/sequoiadb\//$local_report_dir/}
fi
rm -rf -- "$local_report_clear"
mkdir -p -- "$run_dir" "$local_report_dir" "$(dirname -- "$log_file")"
failed_cases_file="$run_dir/.runtest_java.failed"
result_file="$local_report_dir/result.txt"
case_sequence_file="$run_dir/.runtest_java.case-sequence"
failed_result_files_file="$run_dir/.runtest_java.failed-result-files"
export JAVA_HOME="$java_home"
run_begin_epoch=$(date +%s.%N)
run_begin_time=$(date '+%Y-%m-%d %H:%M:%S')
log_display=${log_file#"$case_dir"/}
report_display=${report_source#"$case_dir"/}
echo "======> Begin Java test: $display_project/$display_mode =====>"
echo " Test case : $display_selection"
echo " Case dir  : $case_dir"
echo " COORD     : $db_hostname:$svc_name"
echo " Log       : $log_display"
echo

# CONFIGURETEST 前置成功后由 EXIT trap 保证后置清理，即使 Maven 中途失败也不会跳过。
if [[ "$test_type" == CONFIGURETEST && "$testcase_type" == story_java ]]; then
    if ! run_configure_before; then
        echo "[runtest_java][ERROR] CONFIGURETEST preparation failed" >&2
        exit 2
    fi
fi

: >"$log_file"
: >"$failed_cases_file"
: >"$result_file"
: >"$case_sequence_file"
: >"$failed_result_files_file"
echo " [ Compile ] Compiling Java test project..."
echo
runner_rc=0
previous_testcase_count=0
previous_failure_count=0
previous_error_count=0
xml_count=0
testcase_count=0
failure_count=0
error_count=0
skipped_count=0

for ((suite_index = 0; suite_index < ${#proxy_files[@]}; suite_index++)); do
    proxy_file=${proxy_files[$suite_index]}
    command=(mvn "${maven_common_args[@]}" "-DxmlFileName=$(basename "$proxy_file")")
    # 多 suite 时为每个报告文件添加后缀，避免 TEST-TestSuite.xml 互相覆盖
    if ((${#proxy_files[@]} > 1)); then
        command+=("-Dsurefire.reportNameSuffix=.suite-$((suite_index + 1))")
    fi
    maven_command=$(format_command "${command[@]}")
    maven_command=${maven_command#' '}

    echo " [ Suite $((suite_index + 1))/${#proxy_files[@]} ] $(basename "$proxy_file")" | tee -a "$log_file"
    printf 'CMD: %s\n' "$maven_command" >>"$result_file"
    if [[ "$print_mode" == maven ]]; then
        print_command "${command[@]}"
    fi

    # 使用 FIFO 分离 Maven 与 tee，既保留 Maven 的真实返回码，也便于信号到达时清理完整子进程树。
    output_pipe_dir=$(mktemp -d /tmp/runtest-java-output.XXXXXX)
    output_fifo="$output_pipe_dir/maven.fifo"
    mkfifo -- "$output_fifo"
    stream_maven_output <"$output_fifo" &

    active_tee_pid=$!
    set +e
    (
        cd -- "$case_dir" || exit 1
        "${command[@]}"
    ) >"$output_fifo" 2>&1 &
    active_maven_pid=$!
    wait "$active_maven_pid"
    maven_rc=$?
    active_maven_pid=
    wait "$active_tee_pid"
    active_tee_pid=
    rm -f -- "$output_fifo"
    rmdir -- "$output_pipe_dir"
    output_fifo=
    output_pipe_dir=
    set -e

    if [[ ! -d "$report_source" ]]; then
        echo "[runtest_java][ERROR] JUnit report directory is missing: $report_source" >&2
        runner_rc=3
        break
    fi

    xml_count=$(find "$report_source" -type f -name '*.xml' -print | wc -l)
    testcase_count=$(count_xml_tag "$report_source" testcase)
    failure_count=$(count_xml_tag "$report_source" failure)
    error_count=$(count_xml_tag "$report_source" error)
    skipped_count=$(count_xml_tag "$report_source" skipped)

    stage_testcase_count=$((testcase_count - previous_testcase_count))
    ((stage_testcase_count >= 0)) || stage_testcase_count=$testcase_count
    stage_failure_count=$((failure_count - previous_failure_count))
    stage_error_count=$((error_count - previous_error_count))
    ((stage_failure_count >= 0)) || stage_failure_count=$failure_count
    previous_testcase_count=$testcase_count
    ((stage_error_count >= 0)) || stage_error_count=$error_count
    previous_failure_count=$failure_count
    previous_error_count=$error_count

    echo "[runtest_java] reports=$report_source xml=$xml_count tests=$testcase_count failures=$failure_count errors=$error_count skipped=$skipped_count" >>"$log_file"

    # 报告缺失或一个 testcase 都没有，说明用例根本没跑起来，必须拒绝假成功。

    if ((xml_count == 0 || stage_testcase_count == 0)); then
        echo "[runtest_java][ERROR] current suite completed no test case; refusing a false success" >&2
        runner_rc=3
        break
    fi

    # 「全部 skipped」是环境不满足，不是执行器故障，也不是产品失败。
    # 旧 CI 用 count_skiped_case.xml 判定：非 STANDALONE 才转 unstable，STANDALONE 视为通过。
    # 该策略含 DEPLOY_MODE 分支，属平台职责；此处只报告事实并保持退出码 0，交由 CI 判定。
    # 逐阶段重算：报告在多阶段间累积，所以该标记始终反映最后一个阶段的累积结果。
    if ((testcase_count <= skipped_count)); then
        echo "[runtest_java][WARN] all $testcase_count test case(s) skipped;" \
             "environment likely unsatisfied — leaving the verdict to the caller"
    fi
    if ((maven_rc == 130 || maven_rc == 143)); then
        echo "[runtest_java][ERROR] Maven process was interrupted: exit=$maven_rc; log=$log_file" >&2
        runner_rc=$maven_rc
        break
    fi
    if ((stage_failure_count > 0 || stage_error_count > 0)); then
        echo "[runtest_java][ERROR] current suite contains failures or errors" >&2
        runner_rc=1
        if [[ "$stop_flag" == 1 ]]; then
            echo "[runtest_java] stop remaining suites because -s is 1"
            break
        fi
        continue
    fi
    if ((maven_rc != 0)); then
        echo "[runtest_java][ERROR] Maven failed without a reported test failure:" \
             "exit=$maven_rc; log=$log_file" >&2
        runner_rc=2
        break
    fi
    done

# 最终汇总只读取 Maven 聚合报告，避免扫描分散 XML 重复统计。
summary_xml="$run_dir/TEST-TestSuite.xml"
if [[ -f "$summary_xml" ]]; then
    summary_line=$(grep -m1 "<testsuite" "$summary_xml")
    testcase_count=$(sed -n "s/.* tests=\"\([0-9]*\)\".*/\1/p" <<<"$summary_line")
    failure_count=$(sed -n "s/.* failures=\"\([0-9]*\)\".*/\1/p" <<<"$summary_line")
    error_count=$(sed -n "s/.* errors=\"\([0-9]*\)\".*/\1/p" <<<"$summary_line")
    skipped_count=$(sed -n "s/.* skipped=\"\([0-9]*\)\".*/\1/p" <<<"$summary_line")
fi

# 正常路径主动执行配置测试清理；EXIT trap 只作为异常路径的兜底。
if [[ "$configure_cleanup_needed" == true ]]; then
    if ! run_configure_after; then
        echo "[runtest_java][ERROR] CONFIGURETEST cleanup failed" >&2
        # 已得到的测试结果仍保留在报告中，但最终应按执行环境失败处理。
        if ((runner_rc == 0 || runner_rc == 1)); then runner_rc=2; fi
    fi
    configure_cleanup_needed=false
fi

# 仅在本次确实生成报告时复制并重写，避免发布上一次运行的残留 XML。
# --reportinfo 不在本脚本解析，原样交给 formatter。
if ((xml_count > 0)); then
    if [[ -n "$report_format" ]]; then
        if ! REPORTINFO="$reportinfo" bash "$formatter" "$report_source" "${report_dir:-$report_source}" "$testcase_type"; then
            echo "[runtest_java][ERROR] report formatter failed: $formatter" >&2
            runner_rc=2
        fi
    elif [[ -n "$report_dir" && "$report_dir" != "$report_source" ]]; then
        if ! { mkdir -p -- "$report_dir" && cp -a -- "$report_source/." "$report_dir/"; }; then
            echo "[runtest_java][ERROR] report copy failed: $report_dir" >&2
            runner_rc=2
        fi
    fi
    if [[ -z "$package_name" && -z "$exec_testcase_name" ]]; then
        mkdir -p -- "$local_report_dir/junitreports"
        cp -a -- "$report_source/." "$local_report_dir/junitreports/" 2>/dev/null || true
    fi
fi
run_end_epoch=$(date +%s.%N)
run_end_time=$(date '+%Y-%m-%d %H:%M:%S')
use_time=$(awk -v begin="$run_begin_epoch" -v end="$run_end_epoch" 'BEGIN { printf "%.3f", end - begin }')
failed_count=$((failure_count + error_count))
succeed_count=$((testcase_count - failed_count - skipped_count))
((succeed_count >= 0)) || succeed_count=0

echo
if [[ "$print_mode" == failed && -s "$failed_cases_file" ]]; then
    echo "================ Failed test cases ================"
    failed_index=0
    while IFS='|' read -r failure_type failed_name failed_seconds detail_file; do
        failed_index=$((failed_index + 1))
        printf '[%s] %s [%s] %ss\n' \
            "$failed_index" "$failed_name" "$failure_type" "$failed_seconds"
        echo "    report : $report_display"
        echo "    detail : ${detail_file#"$repo_root"/}"
        echo "    log    : $log_display"
    done <"$failed_cases_file"
    echo "==================================================="
    echo
fi
echo "<====== Finish Java test ======"
echo
echo "***********************************************************"
echo "                    ***test result***"
echo " begin time : $run_begin_time"
echo " end time   : $run_end_time"
echo " use time   : ${use_time}(s)"
echo " total      : $testcase_count"
echo " succeed    : $succeed_count"
echo " failed     : $failed_count"
echo " skipped    : $skipped_count"
echo " log        : $log_display"
echo " report     : $report_display"
echo " local      : local_test_report/java/$display_project/$display_mode"
echo "***********************************************************"
exit "$runner_rc"
