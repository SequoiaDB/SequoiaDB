#bin/bash

SEQPATH="/opt/sequoiadb"
SDB=${SEQPATH}"/bin/sdb"
CLUSTER_OPR_PATH=${SEQPATH}"/tools/dr_ha/cluster_opr.js"

# run command
$SDB -e "var SEQPATH = \"${SEQPATH}\" ; var CUROPR = 'detachGroupNode'; " -f ${CLUSTER_OPR_PATH}
