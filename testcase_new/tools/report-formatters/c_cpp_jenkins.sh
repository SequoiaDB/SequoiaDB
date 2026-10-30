#!/usr/bin/env bash
# C/C++ Jenkins JUnit XML formatter. Arguments: source-dir publish-dir [casetype].
# Failure text matches exec_c_case.xml, not the Java TestNG formatter.
#
# Optional env from caller (CI sets these; local usually omits them):
#   TESTHOST_LIST   comma-separated cluster hosts, same as testhost_list
#   DIAGLOG_URL     full diagnostic package URL; overrides composed URL
#   DIAGLOG_BASE    default http://192.168.28.27:8081
#   TEST_TYPE, DEPLOY_MODE, BUILD_NUMBER
#   TESTCASE_TYPE   preferred classname prefix, e.g. driver_c
set -euo pipefail
source_dir=${1:?missing source report directory}
publish_dir=${2:-$source_dir}
casetype=${TESTCASE_TYPE:-${3:-}}
[[ -d "$source_dir" ]] || { echo "report source not found: $source_dir" >&2; exit 2; }
mkdir -p -- "$publish_dir"

# 本机用 hostname -I；其它主机 getent。解析失败退回主机名本身
is_local_host()
{
    local host=$1
    local local_short local_fqdn

    local_short=$(hostname 2>/dev/null || true)
    local_fqdn=$(hostname -f 2>/dev/null || true)
    [[ "$host" == localhost || "$host" == 127.0.0.1 || "$host" == ::1 ||
       "$host" == "$local_short" || "$host" == "$local_fqdn" ]]
}

resolve_host_ipv4()
{
    local host=$1
    local ip=

    if is_local_host "$host"; then
        if command -v hostname >/dev/null 2>&1; then
            ip=$(hostname -I 2>/dev/null | awk '{print $1}') || true
        fi
        printf '%s\n' "${ip:-127.0.0.1}"
        return
    fi
    if command -v getent >/dev/null 2>&1; then
        ip=$(getent ahostsv4 "$host" 2>/dev/null | awk 'NR==1 {print $1}') || true
    fi
    if [[ -n "$ip" ]]; then
        printf '%s\n' "$ip"
    else
        printf '%s\n' "$host"
    fi
}

escape_sed_replacement()
{
    local value=$1
    value=${value//\\/\\\\}
    value=${value//&/\\&}
    value=${value//|/\\|}
    printf '%s' "$value"
}

# 与 exec_c_case.xml 一致：DIAGLOG_URL 优先，否则 BASE/TYPE_MODE_NUM.tar.gz
build_diaglog_url()
{
    if [[ -n "${DIAGLOG_URL:-}" ]]; then
        printf '%s' "$DIAGLOG_URL"
        return
    fi
    if [[ -n "${TEST_TYPE:-}" && -n "${DEPLOY_MODE:-}" && -n "${BUILD_NUMBER:-}" ]]; then
        printf '%s/%s_%s_%s.tar.gz' \
            "${DIAGLOG_BASE:-http://192.168.28.27:8081}" \
            "$TEST_TYPE" "$DEPLOY_MODE" "$BUILD_NUMBER"
    fi
}

# CLUSTER HOST: 冒号后直接拼 " host/ip"（每项前有空格），不是 Java 的分号括号
format_cluster_hosts()
{
    local report_testhost_list=${TESTHOST_LIST:-}
    local host host_ip cluster_hosts=

    [[ -n "$report_testhost_list" ]] || return 0
    IFS=',' read -r -a report_hosts <<< "$report_testhost_list"
    for host in "${report_hosts[@]}"
    do
        host=$(printf '%s' "$host" | sed 's/^[[:space:]]*//;s/[[:space:]]*$//')
        [[ -n "$host" ]] || continue
        host_ip=$(resolve_host_ipv4 "$host")
        cluster_hosts+=" ${host}/${host_ip}"
    done
    printf '%s' "$cluster_hosts"
}

# 侧车里是测试程序名；CI classname 要把 '.' 换成 '_'
read_casename()
{
    local xml=$1
    local side="${xml}.case"

    if [[ -f "$side" ]]; then
        tr '.' '_' <"$side" | tr -d '\r\n'
    fi
}

if [[ -n "${EXECUTE_HOST_REPORT:-}" ]]; then
    execute_line=$(escape_sed_replacement "EXECUTE HOST: ${EXECUTE_HOST_REPORT}")
else
    execute_host=$(hostname 2>/dev/null || echo host)
    execute_ip=$(resolve_host_ipv4 "$execute_host")
    execute_line=$(escape_sed_replacement "EXECUTE HOST: ${execute_host}/${execute_ip}")
fi
cluster_hosts=$(format_cluster_hosts)
if [[ -n "$cluster_hosts" ]]; then
    cluster_line=$(escape_sed_replacement "CLUSTER HOST:${cluster_hosts}")
else
    cluster_line=""
fi
diaglog_url=$(build_diaglog_url)

# 只处理源目录本层文件；.case 不发布。文案对齐 exec_c_case.xml，只改 <failure>
while IFS= read -r -d '' src
do
    [[ "$src" == *.case ]] && continue
    dest="$publish_dir/$(basename -- "$src")"
    if [[ "$src" != "$dest" ]]; then
        cp -p -- "$src" "$dest"
    fi
    [[ "$src" == *.xml ]] || continue

    casename=$(read_casename "$src")
    if [[ -n "$casetype" && -n "$casename" ]]; then
        type_esc=$(escape_sed_replacement "$casetype")
        name_esc=$(escape_sed_replacement "$casename")
        sed -i -E "s|classname=\"[^\"]*\"|classname=\"${type_esc}.${name_esc}\"|g" "$dest"
    fi

    grep -Eq '<(failure|error)' "$dest" || continue
    if [[ -n "$cluster_line" ]]; then
        sed -i -E "s@(<failure|<error)([^>]*)>@\\1\\2>\\n${execute_line}\\n\\n${cluster_line}\\n\\n@g" "$dest"
    else
        sed -i -E "s@(<failure|<error)([^>]*)>@\\1\\2>\\n${execute_line}\\n\\n@g" "$dest"
    fi
    if [[ -n "$diaglog_url" ]]; then
        url_label=$(escape_sed_replacement "DOWNLOAD DIAGLOG :")
        url_value=$(escape_sed_replacement "$diaglog_url")
        sed -i -E "s@(</failure>|</error>)@\\n${url_label}\\n${url_value}\\n\\1@g" "$dest"
    fi
done < <(find "$source_dir" -maxdepth 1 -type f -print0)
