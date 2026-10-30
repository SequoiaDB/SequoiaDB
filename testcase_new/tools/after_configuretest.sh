#!/bin/bash

COORDHOSTNAME="${1:-localhost}"
COORDSVCNAME="${2:-11810}"
currentHostname=$(hostname)

source /etc/profile
source /etc/default/sequoiadb
INSTALL_DIR="${3:-$INSTALL_DIR}"

# stop sdb-schedule tools
${INSTALL_DIR}/tools/sdb-schedule/sdb-schedule/bin/schctl.sh stop -t all -f

rm -rf ${INSTALL_DIR}/tools/sdb-schedule/sdb-schedule

${INSTALL_DIR}/bin/sdb "var tmpDB = new Sdb(\"$COORDHOSTNAME\", $COORDSVCNAME)"

${INSTALL_DIR}/bin/sdb "tmpDB.dropCS(\"SDB_SCHEDULE_SYSTEM\")"

${INSTALL_DIR}/bin/sdb "tmpDB.dropDataSource(\"sdbscheduledatasource\")"

${INSTALL_DIR}/bin/sdb 'tmpDB.close()'

SCRIPT_DIR=$(cd "$(dirname "$0")" && pwd)
REMOVE_JS="$SCRIPT_DIR/removeDSCluster.js"

echo "cleaning datasource cluster..."
${INSTALL_DIR}/bin/sdb -f $REMOVE_JS -e "var DSHOSTNAME=\"$currentHostname\"; var DSSVCNAME=32010; var COORDHOSTNAME=\"$COORDHOSTNAME\"; var RSRVNODEDIR=\"${INSTALL_DIR}/database\";"
