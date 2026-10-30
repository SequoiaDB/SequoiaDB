#!/bin/bash

COORDHOSTNAME="${1:-localhost}"
COORDSVCNAME="${2:-11810}"

CURRENT_HOSTNAME="$(hostname)"

source /etc/profile >/dev/null 2>&1 || true
source /etc/default/sequoiadb
INSTALL_DIR="${3:-$INSTALL_DIR}"

log() {
    echo "[INFO] $*"
}

err() {
    echo "[ERROR] $*" >&2
}

fail_exit() {
    err "$1"
    cleanup
    exit 1
}

cleanup() {
    log "cleaning env"
    ${INSTALL_DIR}/tools/sdb-schedule/sdb-schedule/bin/schctl.sh stop -t all -f >/dev/null 2>&1
    rm -rf "${INSTALL_DIR}/tools/sdb-schedule/sdb-schedule"

    ${INSTALL_DIR}/bin/sdb "var tmpDB = new Sdb(\"$COORDHOSTNAME\", $COORDSVCNAME)" >/dev/null 2>&1
    ${INSTALL_DIR}/bin/sdb "tmpDB.dropCS(\"SDB_SCHEDULE_SYSTEM\")" >/dev/null 2>&1
    ${INSTALL_DIR}/bin/sdb "tmpDB.dropDataSource(\"sdbscheduledatasource\")" >/dev/null 2>&1
    ${INSTALL_DIR}/bin/sdb 'tmpDB.close()' >/dev/null 2>&1

    SCRIPT_DIR="$(cd "$(dirname "$0")" && pwd)"
    REMOVE_JS="$SCRIPT_DIR/removeDSCluster.js"

    ${INSTALL_DIR}/bin/sdb -f "$REMOVE_JS" \
        -e "var DSHOSTNAME=\"$CURRENT_HOSTNAME\"; \
            var DSSVCNAME=32010; \
            var COORDHOSTNAME=\"$COORDHOSTNAME\"; \
            var RSRVNODEDIR=\"${INSTALL_DIR}/database\";" \
        >/dev/null 2>&1

    # wait for the port to be fully released
    sleep 60
    log "done"
}

run_or_fail() {
    "$@" || return 1
}

deploy_datasource_cluster() {

    ${INSTALL_DIR}/bin/sdblist -p 32010 >/dev/null 2>&1
    if [ $? -eq 0 ]; then
        return 0
    fi

    log "Deploying datasource cluster..."

    ${INSTALL_DIR}/bin/sdblist -p 18800 -m local >/dev/null 2>&1
    if [ $? -eq 0 ]; then
        run_or_fail ${INSTALL_DIR}/bin/sdb "var tmpOma = new Oma(\"$COORDHOSTNAME\", 11790)" || return 1
        run_or_fail ${INSTALL_DIR}/bin/sdb "tmpOma.removeCoord(18800)" || return 1
        run_or_fail ${INSTALL_DIR}/bin/sdb "tmpOma.close()" || return 1
        sleep 60
    fi

    cd "${INSTALL_DIR}/tools/deploy/" || return 1

    cat > sequoiadb.conf <<EOF
role,groupName,hostName,serviceName,dbPath
catalog,SYSCatalogGroup,$CURRENT_HOSTNAME,32000,[installPath]/database/catalog/32000
coord,SYSCoord,$CURRENT_HOSTNAME,32010,[installPath]/database/coord/32010
data,group1,$CURRENT_HOSTNAME,32020,[installPath]/database/data/32020
data,group2,$CURRENT_HOSTNAME,32030,[installPath]/database/data/32030
data,group3,$CURRENT_HOSTNAME,32040,[installPath]/database/data/32040
EOF

    bash quickDeploy.sh --sdb || return 1

    return 0
}

create_datasource() {

    log "Creating datasource..."

    run_or_fail ${INSTALL_DIR}/bin/sdb "var tmpDB = new Sdb(\"$COORDHOSTNAME\", $COORDSVCNAME)" || return 1

    ${INSTALL_DIR}/bin/sdb 'tmpDB.getDataSource("sdbscheduledatasource")' >/dev/null 2>&1
    if [ $? -ne 0 ]; then
        run_or_fail ${INSTALL_DIR}/bin/sdb \
            "tmpDB.createDataSource(\"sdbscheduledatasource\", \"$CURRENT_HOSTNAME:32010\", \"sdbadmin\", \"sdbadmin\")" || return 1
    fi

    run_or_fail ${INSTALL_DIR}/bin/sdb 'tmpDB.close()' || return 1

    return 0
}

deploy_schedule_tools() {

    log "Deploying schedule tools..."

    toolsPath="${INSTALL_DIR}/tools/sdb-schedule/"
    cd "$toolsPath" || return 1

    run_or_fail tar -zxf sdb-schedule-*-release.tar.gz || return 1
    run_or_fail chmod +x sdb-schedule/bin -R || return 1

    cd sdb-schedule/script/ || return 1

    cat > quickDeploy.conf <<EOF
rootsite_url="$COORDHOSTNAME:$COORDSVCNAME"
rootsite_user="sdbadmin"
rootsite_password="sdbadmin1"
node_list="9001,9002"
datasource_list="sdbscheduledatasource"
system_cs_name="SDB_SCHEDULE_SYSTEM"
system_cs_domain=""
EOF

    echo 'y' | bash quickDeploy.sh deploy || return 1

    return 0
}

main() {
    log "Start deploy configure test env"

    cleanup

    deploy_datasource_cluster || fail_exit "deploy datasource cluster failed"
    create_datasource || fail_exit "create datasource failed"
    deploy_schedule_tools || fail_exit "deploy schedule tools failed"

    log "Done"
}

main