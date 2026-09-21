#!/bin/bash
# ========================================
# generate_copy_cluster.sh - Generate JS script for copying Sequoiadb cluster metadata
# ========================================

SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
JS_FILE="$SCRIPT_DIR/generatJs.js"

function show_help() {
    cat << 'EOF'
Usage:
    generate_copy_cluster.sh [options]

Options:
    -h, --help                      Show help message
    --coord-host <hostname>         Coord hostname, default: localhost
    --coord-port <port>             Coord port, default: 11810
    --cata-host <hostname>          Catalog hostname, default: localhost
    --cata-port <port>              Catalog port, default: 11820
    -u, --user <username>           SequoiaDB username, default: empty
    -p, --password <password>       SequoiaDB password, default: empty
    -o, --output <file>             Output JS file path, default: ./copyCluster.js
    --domains <n1,n2,...>           Domain name whitelist (comma separated)
    --ds <n1,n2,...>                Data source name whitelist (comma separated)
    --cs <n1,n2,...>                Collection space name whitelist (comma separated)
    --cl <n1,n2,...>                Collection name whitelist (comma separated)
    --sdb <path>                    Path to sdb binary
                                    (default: INSTALL_DIR/bin/sdb from /etc/default/sequoiadb,
                                     then <tool>/../../bin/sdb)

Examples:
    ./generate_copy_cluster.sh
    ./generate_copy_cluster.sh --coord-host 192.168.1.10 --coord-port 11810 \
        --cata-host 192.168.1.10 --cata-port 11820 -u sdbadmin -p sdbadmin
    ./generate_copy_cluster.sh --domains domain1,domain2 --cs cs1 -o /tmp/copyCluster.js
EOF
}

# Escape a string for embedding into a double-quoted JS string literal.
function js_escape() {
    local s="$1"
    s="${s//\\/\\\\}"
    s="${s//\"/\\\"}"
    printf '%s' "$s"
}

# Convert comma-separated names to a JS array literal: ["a","b"]
function to_js_array() {
    local input="$1"
    local result="["
    local first=1
    local IFS=','
    local item
    for item in $input; do
        item="$(echo "$item" | sed 's/^[[:space:]]*//;s/[[:space:]]*$//')"
        [ -z "$item" ] && continue
        if [ $first -eq 1 ]; then
            first=0
        else
            result="$result,"
        fi
        result="$result\"$(js_escape "$item")\""
    done
    result="$result]"
    printf '%s' "$result"
}

function find_sdb() {
    local sdb_path="$1"
    local candidate=""

    if [ -n "$sdb_path" ]; then
        if [ ! -f "$sdb_path" ]; then
            echo "[ERROR] sdb binary not found: $sdb_path" >&2
            return 1
        fi
        printf '%s' "$sdb_path"
        return 0
    fi

    # 1) /etc/default/sequoiadb -> INSTALL_DIR/bin/sdb
    if [ -f /etc/default/sequoiadb ]; then
        # shellcheck source=/dev/null
        . /etc/default/sequoiadb
        if [ -n "$INSTALL_DIR" ]; then
            candidate="$INSTALL_DIR/bin/sdb"
            if [ -f "$candidate" ]; then
                printf '%s' "$candidate"
                return 0
            fi
        fi
    fi

    # 2) tools/<name>/../../bin/sdb (source tree layout)
    candidate="$SCRIPT_DIR/../../bin/sdb"
    if [ -f "$candidate" ]; then
        printf '%s' "$candidate"
        return 0
    fi

    echo "[ERROR] Cannot find sdb from /etc/default/sequoiadb or $SCRIPT_DIR/../../bin/sdb. Please specify --sdb <path>" >&2
    return 1
}

function main() {
    local coordHost="localhost"
    local coordPort="11810"
    local cataHost="localhost"
    local cataPort="11820"
    local username=""
    local password=""
    local output="./copyCluster.js"
    local domains=""
    local dsNames=""
    local csNames=""
    local clNames=""
    local sdbPath=""

    if ! TEMP=$(getopt -o hu:p:o: \
        --long help,coord-host:,coord-port:,cata-host:,cata-port:,user:,password:,output:,domains:,ds:,cs:,cl:,sdb: \
        -n "$0" -- "$@"); then
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
            --coord-host)
                coordHost="$2"
                shift 2
                ;;
            --coord-port)
                coordPort="$2"
                shift 2
                ;;
            --cata-host)
                cataHost="$2"
                shift 2
                ;;
            --cata-port)
                cataPort="$2"
                shift 2
                ;;
            -u|--user)
                username="$2"
                shift 2
                ;;
            -p|--password)
                password="$2"
                shift 2
                ;;
            -o|--output)
                output="$2"
                shift 2
                ;;
            --domains)
                domains="$2"
                shift 2
                ;;
            --ds)
                dsNames="$2"
                shift 2
                ;;
            --cs)
                csNames="$2"
                shift 2
                ;;
            --cl)
                clNames="$2"
                shift 2
                ;;
            --sdb)
                sdbPath="$2"
                shift 2
                ;;
            --)
                shift
                break
                ;;
            *)
                echo "[ERROR] Unknown option: $1" >&2
                show_help
                exit 1
                ;;
        esac
    done

    if [ ! -f "$JS_FILE" ]; then
        echo "[ERROR] JS script not found: $JS_FILE" >&2
        exit 1
    fi

    local sdb
    sdb=$(find_sdb "$sdbPath") || exit 1

    local cmd=""
    cmd="var coordHostName=\"$(js_escape "$coordHost")\""
    cmd="$cmd; var coordPort=$coordPort"
    cmd="$cmd; var catalogHostName=\"$(js_escape "$cataHost")\""
    cmd="$cmd; var catalogPort=$cataPort"
    cmd="$cmd; var username=\"$(js_escape "$username")\""
    cmd="$cmd; var password=\"$(js_escape "$password")\""
    cmd="$cmd; var generateFilePath=\"$(js_escape "$output")\""

    if [ -n "$domains" ]; then
        cmd="$cmd; var inDomainNames=$(to_js_array "$domains")"
    fi
    if [ -n "$dsNames" ]; then
        cmd="$cmd; var inDSNames=$(to_js_array "$dsNames")"
    fi
    if [ -n "$csNames" ]; then
        cmd="$cmd; var inCSNames=$(to_js_array "$csNames")"
    fi
    if [ -n "$clNames" ]; then
        cmd="$cmd; var inCLNames=$(to_js_array "$clNames")"
    fi

    "$sdb" -e "$cmd" -f "$JS_FILE"
    return $?
}

main "$@"
exit $?
