#!/usr/bin/env bash
# 旧入口，参数为 driver/php 下的一个文件或目录，实际执行交给 testcase_new/tools/run_php_case.sh。
script_dir=$(cd -- "$(dirname -- "${BASH_SOURCE[0]}")" && pwd -P) || exit 2
targets=()
if [[ -n ${1:-} ]]; then
    [[ -d $script_dir/$1 ]] && targets=(-p "$1") || targets=(-f "$1")
fi
exec bash "$script_dir/../../tools/run_php_case.sh" "${targets[@]}"
