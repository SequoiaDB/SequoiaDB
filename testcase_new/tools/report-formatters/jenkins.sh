#!/usr/bin/env bash
# CI/Jenkins JUnit XML formatter. Arguments: source-dir publish-dir testcase-type.
#
# Optional report decoration from caller:
#   REPORTINFO  k=v;k=v. Executor does not parse this.
#     execute_host, test_type, deploy_mode, build_number  -> EXECUTE HOST / DOWNLOAD DIAGLOG URL
#     cluster_hosts                         -> CLUSTER HOST
#     diaglog_url / diaglog_base            -> optional override
#   DIAGLOG_URL / TESTHOST_LIST kept as fallback if REPORTINFO is empty.
set -euo pipefail
source_dir=${1:?missing source report directory}
publish_dir=${2:-$source_dir}
testcase_type=${3:-java}
[[ -d "$source_dir" ]] || { echo "report source not found: $source_dir" >&2; exit 2; }
if [[ "$publish_dir" != "$source_dir" ]]; then
    mkdir -p -- "$publish_dir"
    cp -a -- "$source_dir/." "$publish_dir/"
fi
diaglog_url=${DIAGLOG_URL:-}
diaglog_base=http://192.168.28.27:8081
report_testhost_list=${TESTHOST_LIST:-}
report_test_type=
report_deploy_mode=
report_build_number=
report_execute_host=

if [[ -n "${REPORTINFO:-}" ]]; then
    IFS=';' read -r -a reportinfo_pairs <<< "$REPORTINFO"
    for reportinfo_pair in "${reportinfo_pairs[@]}"; do
        [[ -n "$reportinfo_pair" ]] || continue
        case "${reportinfo_pair%%=*}" in
            execute_host) report_execute_host=${reportinfo_pair#*=} ;;
            diaglog_url) diaglog_url=${reportinfo_pair#*=} ;;
            diaglog_base) diaglog_base=${reportinfo_pair#*=} ;;
            cluster_hosts) report_testhost_list=${reportinfo_pair#*=} ;;
            test_type) report_test_type=${reportinfo_pair#*=} ;;
            deploy_mode) report_deploy_mode=${reportinfo_pair#*=} ;;
            build_number) report_build_number=${reportinfo_pair#*=} ;;
        esac
    done
fi
if [[ -z "$diaglog_url" && -n "$report_test_type" && -n "$report_deploy_mode" && -n "$report_build_number" ]]; then
    diaglog_url="${diaglog_base%/}/${report_test_type}_${report_deploy_mode}_${report_build_number}.tar.gz"
fi

is_local_host() {
    local host=$1 local_short local_fqdn
    local_short=$(hostname 2>/dev/null || true)
    local_fqdn=$(hostname -f 2>/dev/null || true)
    [[ "$host" == localhost || "$host" == 127.0.0.1 || "$host" == ::1 || "$host" == "$local_short" || "$host" == "$local_fqdn" ]]
}

resolve_host_ipv4() {
    local host=$1 ip=
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
    [[ -n "$ip" ]] && printf '%s\n' "$ip" || printf '%s\n' "$host"
}

escape_sed_replacement() {
    local value=$1
    value=${value//\\/\\\\}
    value=${value//&/\\&}
    value=${value//|/\\|}
    printf '%s' "$value"
}

# 按 CI 原 modifytestngreport 的顺序重写 JUnit XML，并补充失败诊断上下文。
rewrite_java_report() {
    local dir=$1 layer=$2 f execute_host execute_ip cluster_hosts host host_ip
    local layer_esc execute_line cluster_line
    layer_esc=$(escape_sed_replacement "$layer")
    if [[ -n "$report_execute_host" ]]; then
        execute_line="EXECUTE HOST: ${report_execute_host}"
    else
        execute_host=$(hostname -f 2>/dev/null || hostname)
        execute_ip=$(resolve_host_ipv4 "$execute_host")
        execute_line="EXECUTE HOST: ${execute_host} (${execute_ip})"
    fi
    execute_line=$(escape_sed_replacement "$execute_line")
    cluster_hosts=
    if [[ -n "$report_testhost_list" ]]; then
        IFS=',' read -r -a report_hosts <<< "$report_testhost_list"
        for host in "${report_hosts[@]}"; do
            host=$(printf '%s' "$host" | sed 's/^[[:space:]]*//;s/[[:space:]]*$//')
            [[ -n "$host" ]] || continue
            host_ip=$(resolve_host_ipv4 "$host")
            cluster_hosts+="${cluster_hosts:+; }${host}(${host_ip})"
        done
    fi
    if [[ -n "$cluster_hosts" ]]; then
        cluster_line=$(escape_sed_replacement "CLUSTER HOST: ${cluster_hosts}")
    fi

    while IFS= read -r -d '' f; do
        sed -i -E 's|(<testsuite[^>]*name=")com\.sequoiadb\.([^.\"]+)\.[^\"]+(\")|\1\2\3|g' "$f"
        sed -i -E "s|classname=\"com\\.sequoiadb\\.([^\"]*)\"|classname=\"${layer_esc}.\\1\"|g" "$f"
        sed -i -E "s|(<testcase[^>]*name=\")([^\"]*)\"([^>]*classname=\")${layer_esc}\\.([^\\.\"]+)\\.([^\\.\"]+)\"|\\1\\5.\\2\"\\3${layer_esc}.\\4\"|g" "$f"
        sed -i -E "s/name=\"([^\"]*)\"(.*) classname=\"${layer_esc}\\.([^.\"]+)\\.([^\"]+)\"/name=\"\\4.\\1\"\\2 classname=\"${layer_esc}.\\3\"/g" "$f"
        if grep -q '<failure\|<error' "$f"; then
            if [[ -n "$cluster_hosts" ]]; then
                sed -i -E "s#(<failure|<error)([^>]*)>#\\1\\2>\\n    ${execute_line}\\n${cluster_line}#g" "$f"
            else
                sed -i -E "s#(<failure|<error)([^>]*)>#\\1\\2>\\n    ${execute_line}#g" "$f"
            fi
            if [[ -n "$diaglog_url" ]]; then
                local url_line
                url_line=$(escape_sed_replacement "DOWNLOAD DIAGLOG: ${diaglog_url}")
                sed -i -E "s#(</failure>|</error>)#\\n${url_line}\\n\\1#g" "$f"
            fi
        fi
    done < <(find "$dir" -type f -name '*.xml' -print0)
}

rewrite_java_report "$publish_dir" "$testcase_type"
